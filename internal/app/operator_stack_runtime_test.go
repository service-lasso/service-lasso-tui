package app

import (
	"context"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
	"time"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/x/ansi"
	"github.com/service-lasso/service-lasso-tui/internal/api"
)

func TestStackRuntimeViewBindsSelectedStatusAndIncidents(t *testing.T) {
	t.Setenv("NO_COLOR", "1")
	m := stackFixture()
	m.services.Rows[0].Running, m.services.Rows[1].Running = "yes", "no"
	now := time.Now().UTC()
	stamp := now.Format("2006-01-02T15:04:05.000Z")
	core, run, restarts := int64(310_000_000), int64(512_000), int64(2)
	incidents := []api.OperatorStackIncident{{At: stamp, ServiceID: "demo-api", Kind: "health_failed"}}
	msg := operatorStackRuntimeLoaded{
		ticket: m.ticket(), ids: []string{"demo-api", "demo-web"}, receipt: now,
		frame: api.OperatorStackRuntimeFrame{
			ObservedAt: stamp, CoreUptimeMS: &core,
			ActiveOperations: &api.OperatorStackOperations{Scope: "workspace", Count: 1},
			Rows: []api.OperatorStackServiceRuntime{
				{ServiceID: "demo-api", UptimeMS: &run, RestartCount: &restarts},
				{ServiceID: "demo-web", RestartCount: new(int64)},
			},
			Incidents: &incidents,
		},
	}
	updated, _ := m.Update(msg)
	m = updated.(operatorModel)
	for _, size := range [][2]int{{60, 18}, {80, 24}, {100, 28}, {112, 33}, {120, 32}, {160, 45}, {200, 20}} {
		m.width, m.height = size[0], size[1]
		view := m.View()
		for _, text := range []string{"Uptime 8m 32s · Restarts 2", "3d 14h", "workspace", "Health check failed", "Core incidents · UTC", "> demo-api", "q Quit"} {
			if !strings.Contains(view, text) {
				t.Fatalf("%v did not render %q\n%s", size, text, view)
			}
		}
		if m.width >= 80 && (!strings.Contains(view, "RUN UPTIME") || !strings.Contains(view, "15M CHECKS")) {
			t.Fatalf("%v lost table telemetry\n%s", size, view)
		}
		if strings.Contains(view, "observed summary") {
			t.Fatalf("current service observations mixed into the timestamped Core timeline at %v\n%s", size, view)
		}
		rows := strings.Split(view, "\n")
		if len(rows) != m.height || strings.Contains(view, "\x1b") {
			t.Fatalf("incorrect height or ANSI escape at %v", size)
		}
		for _, row := range rows {
			if ansi.StringWidth(row) > m.width {
				t.Fatalf("row overflows %v: %q", size, row)
			}
		}
	}
	updated, _ = m.Update(tea.KeyMsg{Type: tea.KeyDown})
	m = updated.(operatorModel)
	if view := m.View(); !strings.Contains(view, "Uptime Stopped · Restarts 0") || strings.Contains(view, "Restarts 2") {
		t.Fatalf("selected metrics did not follow service identity\n%s", view)
	}
	m.request++
	msg.frame.CoreUptimeMS = new(int64)
	m.acceptStackRuntime(msg)
	if value := m.stackRuntime.frame.CoreUptimeMS; value == nil || *value != core {
		t.Fatal("late response replaced current runtime frame")
	}
	m.services.Rows[1].Generation = "new-generation"
	if _, status := m.stackServiceRuntimeFor(m.services.Rows[1], now); status != "Unavailable" {
		t.Fatalf("old generation's service telemetry was reused: %s", status)
	}
}

func TestStackRuntimeUnavailableAndEmptyAreDifferent(t *testing.T) {
	m := stackFixture()
	m.width, m.height = 112, 33
	now := time.Now()
	for _, tc := range []struct {
		message operatorStackRuntimeLoaded
		want    string
	}{
		{operatorStackRuntimeLoaded{err: &api.OperatorPublicError{Code: "permission_denied"}}, "Incident history: Access denied"},
		{operatorStackRuntimeLoaded{err: &api.OperatorPublicError{Code: "not_found"}}, "Incident history unavailable"},
		{operatorStackRuntimeLoaded{err: &api.OperatorPublicError{Code: "source_unavailable"}}, "Incident history: Read failed"},
		{operatorStackRuntimeLoaded{receipt: now}, "Incident history unavailable"},
		{operatorStackRuntimeLoaded{receipt: now, frame: api.OperatorStackRuntimeFrame{Incidents: &[]api.OperatorStackIncident{}}}, "No recent Core incidents"},
		{operatorStackRuntimeLoaded{receipt: now.Add(-time.Minute)}, "Incident history: Stale"},
	} {
		msg := tc.message
		msg.ticket = m.ticket()
		m.acceptStackRuntime(msg)
		if view := m.View(); !strings.Contains(view, tc.want) || !strings.Contains(view, "> demo-api") || strings.Contains(view, "0 active · workspace") {
			t.Fatalf("missing %q or invented a zero\n%s", tc.want, view)
		}
	}
}

func TestStackRuntimeReadsEmptyAdmittedOverview(t *testing.T) {
	m := stackFixture()
	m.services.Rows = nil
	m.width, m.height = 112, 33
	m.ctx = context.Background()
	m.binding = selectedOperatorTestContext("a")
	page := api.OperatorPage[api.OperatorService]{
		SnapshotID: "empty", SourceRevision: "empty-revision", ExpiresAt: "2099-10-08T00:00:00.000Z",
		Rows: []api.OperatorService{},
	}
	runtimeReads := 0
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		switch r.URL.RequestURI() {
		case "/api/operator/v2/reconciliation-context":
			json.NewEncoder(w).Encode(map[string]any{
				"contractVersion": "service-lasso-durable-reconciliation-context.v1", "context": m.binding,
			})
		case "/api/operator/v2/stack-runtime":
			runtimeReads++
			stamp := time.Now().UTC().Format("2006-01-02T15:04:05.000Z")
			json.NewEncoder(w).Encode(map[string]any{
				"contractVersion": api.OperatorContract, "requestId": "request", "generatedAt": stamp,
				"data": map[string]any{"observedAt": stamp, "coreUptimeMs": 42000, "activeOperations": map[string]any{"scope": "workspace", "count": 0}, "rows": []any{}, "incidents": []any{}},
			})
		default:
			t.Errorf("unexpected route %s", r.URL.RequestURI())
			w.WriteHeader(http.StatusBadRequest)
		}
	}))
	defer server.Close()
	var err error
	m.client, err = api.NewClient(server.URL, server.Client(), "fixture-token")
	if err != nil {
		t.Fatal(err)
	}
	updated, cmd := m.Update(operatorLoaded{Ticket: m.ticket(), Data: operatorOverviewRead{Page: page}})
	current := updated.(operatorModel)
	if runtimeReads != 0 || cmd == nil || !strings.Contains(current.View(), "No services in this observation") {
		t.Fatal("overview publication waited for or skipped the empty runtime read")
	}
	updated, _ = current.Update(cmd())
	if view := updated.(operatorModel).View(); runtimeReads != 1 || !strings.Contains(view, "42s") || !strings.Contains(view, "0 active · workspace") || !strings.Contains(view, "No recent Core incidents") {
		t.Fatalf("empty dashboard lost known Core metrics\n%s", view)
	}
}
