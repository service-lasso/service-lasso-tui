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

func TestStackHealthChartsTrackSelectedServiceAcrossSizes(t *testing.T) {
	t.Setenv("NO_COLOR", "1")
	m := stackFixture()
	observed := time.Now().UTC().Truncate(time.Millisecond)
	timestamp := func(minute int) string {
		return observed.Add(time.Duration(minute) * time.Minute).Format("2006-01-02T15:04:05.000Z")
	}
	read := operatorChecksLoaded{
		ticket: m.ticket(), receipt: time.Now(), ids: []string{"demo-api", "demo-web"},
		frame: api.OperatorCheckFrame{
			ObservedAt: timestamp(0), WindowSeconds: 900,
			Rows: []api.OperatorServiceChecks{
				{ServiceID: "demo-api", State: "configured", Samples: []api.OperatorCheckSample{
					{At: timestamp(-14), Passed: true, DurationMS: 43},
					{At: timestamp(-4), Passed: false, DurationMS: 231},
					{At: timestamp(-3), Passed: true, DurationMS: 125},
				}},
				{ServiceID: "demo-web", State: "not_configured", Samples: []api.OperatorCheckSample{}},
			},
		},
	}
	updated, _ := m.Update(read)
	m = updated.(operatorModel)
	for _, size := range [][2]int{{60, 18}, {80, 24}, {100, 28}, {112, 33}, {120, 32}, {160, 45}, {200, 20}} {
		m.width, m.height = size[0], size[1]
		view := m.View()
		if !strings.Contains(view, "> demo-api") || !strings.Contains(view, "ID: demo-api") || !strings.Contains(view, "125 ms") || strings.Contains(view, "\x1b") {
			t.Fatalf("lost selected health data at %v\n%s", size, view)
		}
		if m.width >= 110 && (!strings.Contains(view, "−15m") || !strings.Contains(view, "15M CHECKS") || !strings.Contains(view, "▒") || !strings.Contains(view, "▂")) {
			t.Fatalf("missing graph, strip, or failure glyph at %v\n%s", size, view)
		}
		rows := strings.Split(view, "\n")
		if len(rows) != m.height || !strings.Contains(rows[len(rows)-1], "q ") {
			t.Fatalf("bad layout height or recovery controls at %v", size)
		}
		for _, row := range rows {
			if ansi.StringWidth(row) > m.width {
				t.Fatalf("row clipped at %v: %q", size, row)
			}
		}
	}
	m.width, m.height = 112, 33
	updated, _ = m.Update(tea.KeyMsg{Type: tea.KeyDown})
	m = updated.(operatorModel)
	if view := m.View(); !strings.Contains(view, "ID: demo-web") || !strings.Contains(view, "No HTTP check") || strings.Contains(view, "125 ms") {
		t.Fatalf("latency followed the row index instead of selected service\n%s", view)
	}
	m.row = 0
	read.receipt = time.Now().Add(-time.Minute)
	m.acceptStackChecks(read)
	if view := m.View(); !strings.Contains(view, "Stale") || strings.Contains(view, "125 ms") {
		t.Fatalf("old chart was rendered as current\n%s", view)
	}
	m.overviewPaused = true
	if view := m.View(); !strings.Contains(view, "paused 15m") || !strings.Contains(view, "125 ms") {
		t.Fatalf("paused current page lost its observation\n%s", view)
	}
	m.overviewPaused = false
	m.request++
	read.receipt = time.Now()
	m.acceptStackChecks(read)
	if view := m.View(); strings.Contains(view, "125 ms") {
		t.Fatal("old ticket restored chart data")
	}
}

func TestStackHTTPFailureAttentionIsBoundAndDoesNotPromoteAggregateHealth(t *testing.T) {
	m := stackFixture()
	m.width, m.height = 112, 33
	m.services.Rows[0].Running = "yes"
	m.services.Rows[0].HealthStatus = "unknown"
	m.services.Rows[1].Running = "no"
	m.services.Rows[1].HealthStatus = "unknown"
	now := time.Now().UTC()
	stamp := now.Format("2006-01-02T15:04:05.000Z")
	samples := []api.OperatorCheckSample{{At: stamp, Passed: false, DurationMS: 184}}
	m.acceptStackChecks(operatorChecksLoaded{
		ticket: m.ticket(), receipt: now, ids: []string{"demo-api", "demo-web"},
		frame: api.OperatorCheckFrame{ObservedAt: stamp, WindowSeconds: 900, Rows: []api.OperatorServiceChecks{
			{ServiceID: "demo-api", State: "configured", Samples: samples},
			{ServiceID: "demo-web", State: "configured", Samples: samples},
		}},
	})
	view := m.View()
	for _, phrase := range []string{"HTTP CHECK FAILED", "NEEDS ATTENTION · 2 services", "2 of 2 HTTP checked", "0 pass · 2 fail", "Latest HTTP check failed", "Last failed:"} {
		if !strings.Contains(view, phrase) {
			t.Fatalf("missing %q in Stack view\n%s", phrase, view)
		}
	}
	if strings.Count(view, "Latest HTTP check failed") != 2 || strings.Contains(view, "Healthy") {
		t.Fatal("probe lost observed failure with unknown intent or promoted aggregate health")
	}
}

func TestStackOptionalChecksReadDoesNotFailOverview(t *testing.T) {
	m := stackFixture()
	m.width, m.height = 112, 33
	m.ctx = context.Background()
	m.binding = selectedOperatorTestContext("a")
	page := api.OperatorPage[api.OperatorService]{
		SnapshotID: "snapshot-checks", SourceRevision: "list-revision", ExpiresAt: "2099-10-08T00:00:00.000Z",
		Rows: m.services.Rows,
	}
	checkReads, runtimeReads := 0, 0
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		switch r.URL.Path {
		case "/api/operator/v2/reconciliation-context":
			json.NewEncoder(w).Encode(map[string]any{
				"contractVersion": "service-lasso-durable-reconciliation-context.v1",
				"context":         m.binding,
			})
		case "/api/operator/v2/health-check-samples":
			checkReads++
			ids := r.URL.Query()["serviceId"]
			if len(ids) != 2 || ids[0] != "demo-api" || ids[1] != "demo-web" {
				t.Errorf("optional read crossed admitted page: %v", ids)
			}
			w.WriteHeader(http.StatusNotFound)
		case "/api/operator/v2/stack-runtime":
			runtimeReads++
			w.WriteHeader(http.StatusNotFound)
		default:
			t.Errorf("unexpected route %s", r.URL.Path)
			w.WriteHeader(http.StatusBadRequest)
		}
	}))
	defer server.Close()
	client, err := api.NewClient(server.URL, server.Client(), "fixture-token")
	if err != nil {
		t.Fatal(err)
	}
	m.client = client
	updated, side := m.Update(operatorLoaded{Ticket: m.ticket(), Data: operatorOverviewRead{Page: page}})
	current := updated.(operatorModel)
	if checkReads != 0 || runtimeReads != 0 || side == nil || !current.pageSelectionCurrent() || current.services.Snapshot != page.SnapshotID {
		t.Fatal("overview publication waited for or skipped the optional check read")
	}
	if view := current.View(); !strings.Contains(view, "> demo-api") || !strings.Contains(view, "Pending") {
		t.Fatalf("optional pending check hid the overview\n%s", view)
	}
	batch, ok := side().(tea.BatchMsg)
	if !ok || len(batch) != 2 {
		t.Fatal("optional reads are not independently scheduled")
	}
	for _, read := range batch {
		updated, _ = current.Update(read())
		current = updated.(operatorModel)
	}
	if checkReads != 1 || runtimeReads != 1 {
		t.Fatalf("expected one batched read per source, got %d health and %d runtime", checkReads, runtimeReads)
	}
	if view := current.View(); !strings.Contains(view, "Unavailable") || !strings.Contains(view, "ID: demo-api") || current.errorText != "" {
		t.Fatalf("older Core check route failed overview\n%s", view)
	}
}

func TestStackEmptyChecksKeepGraphicsAcrossSizes(t *testing.T) {
	t.Setenv("NO_COLOR", "1")
	for _, state := range []string{"not_configured", "configured", "Pending", "Stale", "Unavailable", "Access denied", "Read failed"} {
		t.Run(state, func(t *testing.T) {
			for _, size := range [][2]int{{60, 18}, {80, 24}, {100, 28}, {112, 33}, {120, 32}, {160, 45}, {200, 20}} {
				m := stackFixture()
				m.width, m.height = size[0], size[1]
				now := time.Now().UTC()
				check := stackServiceChecks{observation: api.OperatorServiceChecks{State: state}, receipt: now}
				service := m.services.Rows[0]
				check.revision, check.generation, check.modelEpoch = service.DefinitionRevision, service.Generation, service.ModelEpoch
				if state != "configured" && state != "not_configured" {
					check.status = state
				}
				m.stackChecks = map[string]stackServiceChecks{service.ServiceID: check}
				_, status := m.stackCheckFor(service, now)
				view := m.View()
				if !strings.Contains(view, status) || !strings.Contains(view, "ID: demo-api") || !strings.Contains(view, "─") && !strings.Contains(view, "━") {
					t.Fatalf("missing state or graphic for %s at %v\n%s", state, size, view)
				}
				if strings.Contains(view, "Latest 0 ms") || strings.Contains(view, "0 ms · pass") {
					t.Fatal("fabricated sample")
				}
				rows := strings.Split(view, "\n")
				if len(rows) != size[1] {
					t.Fatalf("height %d at %v", len(rows), size)
				}
				for _, row := range rows {
					if ansi.StringWidth(row) > size[0] {
						t.Fatalf("overflow at %v: %q", size, row)
					}
				}
				strip := stackCheckStrip(check, status, operatorBaseStyle, 15)
				want := strings.Repeat("─", 15)
				if state == "not_configured" {
					want = strings.Repeat("━", 15)
				}
				if strip != want {
					t.Fatalf("neutral strip %q want %q", strip, want)
				}
			}
		})
	}
}
