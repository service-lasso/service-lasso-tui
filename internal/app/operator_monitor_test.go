package app

import (
	"context"
	"net/http"
	"testing"
	"time"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/service-lasso/service-lasso-tui/internal/api"
)

func monitorFixture(t *testing.T) operatorModel {
	t.Helper()
	m := dashboardTestModel()
	client, err := api.NewClient("http://127.0.0.1:1", http.DefaultClient, "fixture-token")
	if err != nil {
		t.Fatal(err)
	}
	m.client = client
	m.ctx = context.Background()
	m.discovery.Capabilities = []api.OperatorCapability{{ID: "read_overview", Availability: api.OperatorAvailability{Available: true}}}
	m.acceptPageSelection(m.ticket())
	m.services.Complete = true
	m.overviewObservedAt = time.Now().Add(-10 * time.Second)
	return m
}
func TestOperatorMonitorEligibilityAndSingleFlight(t *testing.T) {
	now := time.Now()
	for _, test := range []struct {
		name   string
		change func(*operatorModel)
	}{
		{"small viewport", func(m *operatorModel) { m.width = 60 }},
		{"paused", func(m *operatorModel) { m.overviewPaused = true }},
		{"favorites-only", func(m *operatorModel) { m.favoritesOnly = true }},
		{"view focus", func(m *operatorModel) { m.focus = 0 }},
		{"other view", func(m *operatorModel) { m.journey = "logs" }},
		{"pending", func(m *operatorModel) { m.loading = true }},
		{"bootstrap", func(m *operatorModel) { m.bootInFlight = true }},
		{"draft", func(m *operatorModel) { m.draft.Dirty = true }},
		{"sent", func(m *operatorModel) { m.draft.Sent = true }},
		{"preview", func(m *operatorModel) { m.draft.Preview = &api.OperatorPreview{} }},
		{"operation", func(m *operatorModel) { m.operation = &api.OperatorReadback{} }},
		{"custody", func(m *operatorModel) { m.custodyWarning = "held" }},
		{"unknown selection", func(m *operatorModel) { m.epoch++ }},
		{"incomplete", func(m *operatorModel) { m.services.Complete = false }},
		{"truncated", func(m *operatorModel) { m.services.Truncated = true }},
		{"accumulated", func(m *operatorModel) { m.services.seenCursors = map[string]bool{"old": true} }},
		{"unadmitted", func(m *operatorModel) { m.discovery.Capabilities = nil }},
		{"too soon", func(m *operatorModel) { m.overviewObservedAt = now }},
	} {
		t.Run(test.name, func(t *testing.T) {
			m := monitorFixture(t)
			test.change(&m)
			if m.refreshOverviewIfDue(now) != nil {
				t.Fatal("ineligible overview refreshed")
			}
		})
	}
	m := monitorFixture(t)
	if m.refreshOverviewIfDue(now) == nil || !m.loading {
		t.Fatal("eligible due overview did not read")
	}
	if m.refreshOverviewIfDue(now.Add(10*time.Second)) != nil {
		t.Fatal("concurrent overview read")
	}
	m.loading = false
	if m.refreshOverviewIfDue(now.Add(time.Second)) != nil {
		t.Fatal("attempt rate limit bypassed")
	}
	if m.cancel != nil {
		m.cancel()
	}
}
func TestOperatorMonitorPauseAndSelectionOnReplacement(t *testing.T) {
	m := monitorFixture(t)
	m.row = 1
	updated, _ := m.Update(tea.KeyMsg{Type: tea.KeySpace})
	m = updated.(operatorModel)
	if !m.overviewPaused || m.overviewMonitorMode() != "Paused · Space Resume" {
		t.Fatal("Space did not pause")
	}
	if m.refreshOverviewIfDue(time.Now()) != nil {
		t.Fatal("paused monitor read")
	}
	updated, _ = m.Update(tea.KeyMsg{Type: tea.KeySpace})
	m = updated.(operatorModel)
	if m.overviewPaused {
		t.Fatal("Space did not resume")
	}
	if m.refreshOverviewIfDue(time.Now()) == nil {
		t.Fatal("resumed due monitor did not read")
	}
	page := api.OperatorPage[api.OperatorService]{SnapshotID: "new", SourceRevision: "revision", ExpiresAt: "2099-10-08T00:00:00.000Z", Rows: []api.OperatorService{m.services.Rows[1], m.services.Rows[0]}}
	updated, _ = m.Update(operatorLoaded{Ticket: m.ticket(), Data: operatorOverviewRead{Page: page}})
	m = updated.(operatorModel)
	if m.row != 0 || m.overviewServices()[m.row].ServiceID != "demo-web" {
		t.Fatal("refresh reordered selected service identity")
	}
	if m.cancel != nil {
		m.cancel()
	}
}
func TestOperatorMonitorFailureRetainsRowsAgeAndRetriesBoundedly(t *testing.T) {
	m := monitorFixture(t)
	before := m.overviewObservedAt
	now := time.Now()
	if m.refreshOverviewIfDue(now) == nil {
		t.Fatal("missing due read")
	}
	updated, _ := m.Update(operatorLoaded{Ticket: m.ticket(), Err: &api.OperatorPublicError{Code: "source_unavailable"}})
	m = updated.(operatorModel)
	if m.overviewObservedAt != before || len(m.overviewServices()) != 2 || !m.services.Stale {
		t.Fatal("failure renewed or discarded observation")
	}
	if m.refreshOverviewIfDue(now.Add(time.Second)) != nil {
		t.Fatal("failure retried without delay")
	}
	if m.refreshOverviewIfDue(now.Add(6*time.Second)) == nil {
		t.Fatal("stale failed observation could not recover")
	}
	if m.cancel != nil {
		m.cancel()
	}
}
