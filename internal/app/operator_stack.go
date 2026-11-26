package app

import (
	"fmt"
	"strings"

	"github.com/service-lasso/service-lasso-tui/internal/api"
)

// stackView uses only the current overview observation. Its inspector deliberately
// shares the exact filtered row index used by Enter, not a retained detail read.
func (m operatorModel) stackView() string {
	w := m.width
	lines := []string{dashboardTitle.Render(padLine(" SL  Service Lasso / STACK MONITOR", w))}
	focus := "Services"
	if m.focus == 0 {
		focus = "Views"
	}
	lines = append(lines, padLine(" Focus: "+focus+" · Tab changes focus · filter: "+m.filter(), w))
	state := "Pending: no snapshot loaded for this selection."
	current := m.pageSelectionCurrent()
	if current {
		state = m.snapshotStatus()
		if state == "" {
			state = "Pending: no snapshot loaded."
		}
	}
	if m.loading {
		state = "Loading… " + state
	}
	if m.errorText != "" {
		state = "Read failed: " + m.errorText + " · " + state
	}
	lines = append(lines, padLine(operatorText(state), w))
	var services []api.OperatorService
	if current && m.services.Snapshot != "" {
		services = m.overviewServices()
	}
	running, stopped, unknown, healthy, unhealthy, healthUnknown := 0, 0, 0, 0, 0, 0
	for _, s := range services {
		switch s.Running {
		case "yes":
			running++
		case "no":
			stopped++
		default:
			unknown++
		}
		switch s.HealthStatus {
		case "healthy":
			healthy++
		case "unhealthy":
			unhealthy++
		default:
			healthUnknown++
		}
	}
	summary := " Observed statuses: unavailable"
	if services != nil {
		summary = fmt.Sprintf(" Observed %d · running %d · stopped %d · unknown %d", len(services), running, stopped, unknown)
	}
	lines = append(lines, padLine(summary, w))
	health := " Health: unavailable"
	if services != nil {
		health = fmt.Sprintf(" Health · healthy %d · unhealthy %d · unknown %d", healthy, unhealthy, healthUnknown)
	}
	lines = append(lines, padLine(health, w))
	bodyHeight := m.height - len(lines) - 1
	if bodyHeight < 3 {
		return strings.Join(lines[:min(len(lines), m.height)], "\n")
	}
	if m.focus == 0 {
		rows := make([]string, len(operatorViews))
		for i, v := range operatorViews {
			mark := "  "
			if i == m.row {
				mark = "> "
			}
			rows[i] = mark + v
		}
		start := stackStart(m.row, len(rows), bodyHeight-2)
		lines = append(lines, dashboardPanel("VIEWS", rows[start:], w, bodyHeight)...)
	} else {
		inspector := []string{"No selected service observation."}
		if m.row >= 0 && m.row < len(services) {
			s := services[m.row]
			inspector = []string{operatorText(s.DisplayName), "ID: " + operatorText(s.ServiceID), "Running: " + s.Running + " · Health: " + s.HealthStatus, "Installed: " + s.Installed + " · Configured: " + s.Configured, "Update: " + s.UpdateStatus, operatorText(s.Description)}
		}
		listWidth := w
		listHeight := bodyHeight
		wide := w >= 110
		if wide {
			listWidth = w * 55 / 100
		} else {
			listHeight = bodyHeight - 8
			if listHeight < 3 {
				listHeight = 3
			}
		}
		rows := []string{}
		for i, s := range services {
			mark := "  "
			if i == m.row {
				mark = "> "
			}
			row := padLine(operatorText(fmt.Sprintf("%s%s · %s · %s", mark, s.ServiceID, s.Running, s.HealthStatus)), listWidth-2)
			if i == m.row {
				row = dashboardSelect.Render(row)
			}
			rows = append(rows, row)
		}
		if len(rows) == 0 {
			if current && m.services.Snapshot != "" {
				rows = append(rows, "No services in this observation.")
			} else {
				rows = append(rows, "Waiting for a current observation.")
			}
		}
		start := stackStart(m.row, len(rows), listHeight-2)
		list := dashboardPanel(fmt.Sprintf("SERVICES %d/%d", min(m.row+1, len(services)), len(services)), rows[start:], listWidth, listHeight)
		if wide {
			detail := dashboardPanel("INSPECTOR", inspector, w-listWidth-1, bodyHeight)
			for i := range list {
				lines = append(lines, list[i]+" "+detail[i])
			}
		} else {
			lines = append(lines, list...)
			remaining := m.height - len(lines) - 1
			if remaining >= 3 {
				lines = append(lines, dashboardPanel("INSPECTOR", inspector, w, remaining)...)
			}
		}
	}
	footer := " ↑/↓ select · Enter inspect · ←/→ views · Tab focus · r refresh · f filter · q quit"
	lines = append(lines, dashboardBar.Render(padLine(footer, w)))
	return strings.Join(lines, "\n")
}

func stackStart(row, count, visible int) int {
	if visible < 1 {
		return 0
	}
	start := row - visible + 1
	if start < 0 {
		start = 0
	}
	if maxStart := count - visible; maxStart >= 0 && start > maxStart {
		start = maxStart
	}
	if start >= count {
		return 0
	}
	return start
}
