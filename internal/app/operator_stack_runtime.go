package app

import (
	"context"
	"fmt"
	"time"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/service-lasso/service-lasso-tui/internal/api"
)

type operatorStackRuntimeLoaded struct {
	ticket  operatorTicket
	ids     []string
	frame   api.OperatorStackRuntimeFrame
	receipt time.Time
	err     error
}

type stackRuntimeService struct {
	row                              api.OperatorStackServiceRuntime
	revision, generation, modelEpoch string
	receipt                          time.Time
	status                           string
}

type stackRuntimeState struct {
	frame   api.OperatorStackRuntimeFrame
	receipt time.Time
	status  string
	pending bool
	rows    map[string]stackRuntimeService
}

func (m *operatorModel) refreshStackRuntime(read operatorOverviewRead, ticket operatorTicket, replace bool) tea.Cmd {
	m.stackRuntime.pending = false
	if replace {
		retained := make(map[string]stackRuntimeService, len(read.Page.Rows))
		for _, row := range read.Page.Rows {
			if runtime, ok := m.stackRuntime.rows[row.ServiceID]; ok && runtime.matches(row) {
				retained[row.ServiceID] = runtime
			}
		}
		m.stackRuntime.rows = retained
	}
	if m.client == nil || m.ctx == nil || !m.pageSelectionCurrent() {
		return nil
	}
	ids := make([]string, 0, len(read.Page.Rows))
	for _, row := range read.Page.Rows {
		ids = append(ids, row.ServiceID)
	}
	m.stackRuntime.pending = true
	client, parent := m.client, m.ctx
	return func() tea.Msg {
		ctx, cancel := context.WithTimeout(parent, 2*time.Second)
		defer cancel()
		response := operatorStackRuntimeLoaded{ticket: ticket, ids: ids}
		if response.err = operatorSameContext(ctx, client, ticket.Binding); response.err != nil {
			return response
		}
		response.frame, response.err = client.OperatorStackRuntime(ctx, ids)
		if response.err == nil {
			response.receipt = time.Now()
			response.err = operatorSameContext(ctx, client, ticket.Binding)
		}
		return response
	}
}

func (m *operatorModel) acceptStackRuntime(msg operatorStackRuntimeLoaded) {
	if !m.current(msg.ticket) || !m.pageSelectionCurrent() || m.services.Stale {
		return
	}
	state := &m.stackRuntime
	state.pending = false
	state.status = stackCheckErrorStatus(msg.err)
	state.receipt = msg.receipt
	state.frame = msg.frame
	observed := make(map[string]api.OperatorStackServiceRuntime, len(msg.frame.Rows))
	for _, row := range msg.frame.Rows {
		observed[row.ServiceID] = row
	}
	services := make(map[string]api.OperatorService, len(m.services.Rows))
	for _, service := range m.services.Rows {
		services[service.ServiceID] = service
	}
	if state.rows == nil {
		state.rows = make(map[string]stackRuntimeService, len(msg.ids))
	}
	for _, id := range msg.ids {
		if service, ok := services[id]; ok {
			state.rows[id] = stackRuntimeService{
				row: observed[id], revision: service.DefinitionRevision, generation: service.Generation,
				modelEpoch: service.ModelEpoch, receipt: state.receipt, status: state.status,
			}
		}
	}
}

func (m operatorModel) stackRuntimeFreshness(now, receipt time.Time, status string) string {
	if !m.pageSelectionCurrent() || m.services.Snapshot == "" {
		if m.loading {
			return "Pending"
		}
		return "Unavailable"
	}
	if m.services.Stale || operatorSnapshotExpired(m.services.Expiry, now) {
		return "Stale"
	}
	if status != "" {
		return status
	}
	if receipt.IsZero() {
		if m.loading || m.stackRuntime.pending {
			return "Pending"
		}
		return "Unavailable"
	}
	if now.Before(receipt) || !m.overviewPaused && now.Sub(receipt) > 3*overviewRefreshInterval {
		return "Stale"
	}
	return ""
}

func (m operatorModel) stackServiceRuntimeFor(s api.OperatorService, now time.Time) (api.OperatorStackServiceRuntime, string) {
	row, ok := m.stackRuntime.rows[s.ServiceID]
	if !ok || !row.matches(s) {
		return api.OperatorStackServiceRuntime{}, m.stackRuntimeFreshness(now, time.Time{}, "")
	}
	return row.row, m.stackRuntimeFreshness(now, row.receipt, row.status)
}

func (r stackRuntimeService) matches(s api.OperatorService) bool {
	return r.revision == s.DefinitionRevision && r.generation == s.Generation && r.modelEpoch == s.ModelEpoch
}

func stackUptime(ms *int64, status string) string {
	if status != "" {
		return status
	}
	if ms == nil {
		return "Unavailable"
	}
	seconds := *ms / 1000
	switch {
	case seconds < 60:
		return fmt.Sprintf("%ds", seconds)
	case seconds < 3600:
		return fmt.Sprintf("%dm %02ds", seconds/60, seconds%60)
	case seconds < 86400:
		return fmt.Sprintf("%dh %02dm", seconds/3600, seconds%3600/60)
	default:
		return fmt.Sprintf("%dd %02dh", seconds/86400, seconds%86400/3600)
	}
}

func stackRunUptime(s api.OperatorService, runtime api.OperatorStackServiceRuntime, status string) string {
	if s.Running == "no" {
		return "Stopped"
	}
	return stackUptime(runtime.UptimeMS, status)
}

func stackRestartCount(runtime api.OperatorStackServiceRuntime, status string) string {
	if status != "" {
		return status
	}
	if runtime.RestartCount == nil {
		return "Unavailable"
	}
	return fmt.Sprint(*runtime.RestartCount)
}

func stackIncidentLabel(kind string) string {
	switch kind {
	case "http_check_failed":
		return "HTTP check failed"
	case "health_failed":
		return "Health check failed"
	case "service_stopped":
		return "Service stopped"
	case "operation_failed":
		return "Operation failed"
	case "operation_active":
		return "Operation in progress"
	default:
		return ""
	}
}

func (m operatorModel) stackIncidentRows(now time.Time, status string, services []api.OperatorService) ([]string, string) {
	if status == "Unavailable" || status == "" && m.stackRuntime.frame.Incidents == nil {
		return nil, "Incident history unavailable · summary observations only"
	}
	if status != "" {
		return nil, "Incident history: " + status
	}
	frame := m.stackRuntime.frame
	current := make(map[string]bool, len(services))
	for _, s := range services {
		row, ok := m.stackRuntime.rows[s.ServiceID]
		if ok && row.receipt.Equal(m.stackRuntime.receipt) && row.matches(s) && m.stackRuntimeFreshness(now, row.receipt, row.status) == "" {
			current[s.ServiceID] = true
		}
	}
	rows := make([]string, 0, len(*frame.Incidents))
	for _, incident := range *frame.Incidents {
		if current[incident.ServiceID] {
			at, _ := time.Parse(time.RFC3339Nano, incident.At)
			rows = append(rows, fmt.Sprintf("%sZ  %s  ·  %s", at.UTC().Format("15:04:05"), operatorText(incident.ServiceID), stackIncidentLabel(incident.Kind)))
		}
	}
	scope := "listed services"
	if len(current) != len(services) {
		scope = "latest batch"
	}
	if len(rows) == 0 {
		return nil, "No recent Core incidents · " + scope
	}
	return rows, "Core incidents · UTC · " + scope
}
