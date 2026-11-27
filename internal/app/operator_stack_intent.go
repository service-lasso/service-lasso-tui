package app

import (
	"github.com/charmbracelet/lipgloss"
	"github.com/service-lasso/service-lasso-tui/internal/api"
	"time"
)

// Intent is supplied by Core, never inferred from a service's name or probe.
func (m operatorModel) stackIntent(s api.OperatorService, now time.Time) (string, string) {
	row, status := m.stackServiceRuntimeFor(s, now)
	if status != "" {
		return "", status
	}
	switch row.StackIntent {
	case "expected", "prepared", "optional", "disabled", "unsupported", "gated", "unprepared":
		return row.StackIntent, ""
	default:
		return "", "Unavailable"
	}
}

func (m operatorModel) stackNeedsAttention(s api.OperatorService, now time.Time) bool {
	if s.HealthStatus == "unhealthy" {
		return true
	}
	intent, _ := m.stackIntent(s, now)
	return s.Running == "no" && intent == "expected"
}

func (m operatorModel) stackIntentStatusText(s api.OperatorService, now time.Time) string {
	text, _ := m.stackIntentStatus(s, stackServiceChecks{}, "Unavailable", now)
	return text
}

func (m operatorModel) stackIntentStatus(s api.OperatorService, checks stackServiceChecks, checkStatus string, now time.Time) (string, lipgloss.Color) {
	// Actual unhealthy observations retain precedence over lifecycle intent.
	if s.HealthStatus == "unhealthy" {
		return "Degraded", operatorAmber
	}
	if checkStatus == "" && !checks.latest().Passed {
		return "HTTP check failed", operatorAmber
	}
	if s.Running != "no" {
		return stackCheckServiceStatus(s, checks, checkStatus)
	}
	intent, metadataStatus := m.stackIntent(s, now)
	switch intent {
	case "expected":
		return "Expected stopped", operatorCoral
	case "prepared":
		return "Prepared", operatorMuted
	case "optional":
		return "Optional stopped", operatorMuted
	case "disabled":
		return "Disabled", operatorMuted
	case "unsupported":
		return "Unsupported", operatorMuted
	case "gated":
		return "Setup gated", operatorMuted
	case "unprepared":
		return "Not prepared", operatorMuted
	default:
		switch metadataStatus {
		case "Pending":
			return "Intent pending", operatorMuted
		case "Stale":
			return "Intent stale", operatorMuted
		case "Access denied":
			return "Intent denied", operatorMuted
		case "Read failed":
			return "Intent read error", operatorMuted
		default:
			return "Stopped / unknown", operatorMuted
		}
	}
}
