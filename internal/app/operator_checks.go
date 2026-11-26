package app

import (
	"context"
	"errors"
	"net/http"
	"time"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/lipgloss"
	"github.com/service-lasso/service-lasso-tui/internal/api"
)

type operatorChecksLoaded struct {
	ticket  operatorTicket
	ids     []string
	frame   api.OperatorCheckFrame
	receipt time.Time
	err     error
}

type stackServiceChecks struct {
	observation api.OperatorServiceChecks
	revision    string
	generation  string
	modelEpoch  string
	observed    time.Time
	receipt     time.Time
	status      string
}

func (m *operatorModel) refreshStackChecks(read operatorOverviewRead, ticket operatorTicket, replace bool) tea.Cmd {
	m.stackChecksPending = false
	if replace {
		retained := make(map[string]stackServiceChecks, len(read.Page.Rows))
		for _, row := range read.Page.Rows {
			if checks, ok := m.stackChecks[row.ServiceID]; ok && checks.matches(row) {
				retained[row.ServiceID] = checks
			}
		}
		m.stackChecks = retained
	}
	if m.client == nil || m.ctx == nil || len(read.Page.Rows) == 0 || !m.pageSelectionCurrent() {
		return nil
	}
	ids := make([]string, 0, len(read.Page.Rows))
	for _, row := range read.Page.Rows {
		ids = append(ids, row.ServiceID)
	}
	m.stackChecksPending = true
	client, parent := m.client, m.ctx
	return func() tea.Msg {
		ctx, cancel := context.WithTimeout(parent, 2*time.Second)
		defer cancel()
		response := operatorChecksLoaded{ticket: ticket, ids: ids}
		if response.err = operatorSameContext(ctx, client, ticket.Binding); response.err != nil {
			return response
		}
		response.frame, response.err = client.OperatorHealthChecks(ctx, ids)
		if response.err == nil {
			response.receipt = time.Now()
			response.err = operatorSameContext(ctx, client, ticket.Binding)
		}
		return response
	}
}

func (m *operatorModel) acceptStackChecks(msg operatorChecksLoaded) {
	if !m.current(msg.ticket) || !m.pageSelectionCurrent() || m.services.Stale {
		return
	}
	m.stackChecksPending = false
	rows := make(map[string]api.OperatorServiceChecks, len(msg.frame.Rows))
	for _, row := range msg.frame.Rows {
		rows[row.ServiceID] = row
	}
	current := make(map[string]api.OperatorService, len(m.services.Rows))
	for _, service := range m.services.Rows {
		current[service.ServiceID] = service
	}
	observed, _ := time.Parse(time.RFC3339Nano, msg.frame.ObservedAt)
	status := stackCheckErrorStatus(msg.err)
	if m.stackChecks == nil {
		m.stackChecks = make(map[string]stackServiceChecks, len(msg.ids))
	}
	for _, id := range msg.ids {
		if service, ok := current[id]; ok {
			m.stackChecks[id] = stackServiceChecks{
				observation: rows[id], revision: service.DefinitionRevision, generation: service.Generation,
				modelEpoch: service.ModelEpoch, observed: observed, receipt: msg.receipt, status: status,
			}
		}
	}
}

func stackCheckErrorStatus(err error) string {
	if err == nil {
		return ""
	}
	var public *api.OperatorPublicError
	var transport *api.OperatorError
	switch {
	case errors.As(err, &public):
		switch public.Code {
		case "permission_denied", "authentication_required":
			return "Access denied"
		case "not_found", "unsupported_capability", "unsupported_version", "unsupported_selector":
			return "Unavailable"
		}
	case errors.As(err, &transport):
		switch transport.Status {
		case http.StatusForbidden, http.StatusUnauthorized:
			return "Access denied"
		case http.StatusNotFound:
			return "Unavailable"
		}
	}
	return "Read failed"
}

func (c stackServiceChecks) latest() api.OperatorCheckSample {
	return c.observation.Samples[len(c.observation.Samples)-1]
}

func stackCheckServiceStatus(service api.OperatorService, checks stackServiceChecks, status string) (string, lipgloss.Color) {
	if status == "" && service.Running != "no" && service.HealthStatus != "healthy" && service.HealthStatus != "unhealthy" {
		if checks.latest().Passed {
			return "HTTP check passed", operatorGreen
		}
		return "HTTP check failed", operatorAmber
	}
	return stackObservedStatus(service), stackStatusColor(service)
}

func (c stackServiceChecks) matches(service api.OperatorService) bool {
	return c.revision == service.DefinitionRevision && c.generation == service.Generation && c.modelEpoch == service.ModelEpoch
}

func (m operatorModel) stackCheckFor(service api.OperatorService, now time.Time) (stackServiceChecks, string) {
	if m.services.Stale || operatorSnapshotExpired(m.services.Expiry, now) {
		return stackServiceChecks{}, "Stale"
	}
	checks, ok := m.stackChecks[service.ServiceID]
	if !ok || !checks.matches(service) {
		if m.loading || m.stackChecksPending {
			return stackServiceChecks{}, "Pending"
		}
		return stackServiceChecks{}, "Unavailable"
	}
	if checks.status != "" {
		return stackServiceChecks{}, checks.status
	}
	if now.Before(checks.receipt) || !m.overviewPaused && now.Sub(checks.receipt) > 3*overviewRefreshInterval {
		return stackServiceChecks{}, "Stale"
	}
	if checks.observation.State == "not_configured" {
		return stackServiceChecks{}, "No HTTP check"
	}
	if len(checks.observation.Samples) == 0 {
		return stackServiceChecks{}, "No samples"
	}
	return checks, ""
}
