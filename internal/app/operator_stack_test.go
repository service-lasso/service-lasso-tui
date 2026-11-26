package app

import (
	"context"
	"fmt"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/x/ansi"
	"github.com/service-lasso/service-lasso-tui/internal/api"
)

func stackFixture() operatorModel {
	m := dashboardTestModel()
	m.services.Complete = true
	m.acceptPageSelection(m.ticket())
	return m
}
func TestStackResponsiveSelectionAndEnter(t *testing.T) {
	for _, width := range []int{80, 120, 160} {
		t.Run(fmt.Sprint(width), func(t *testing.T) {
			m := stackFixture()
			m.width = width
			m.height = 24
			for i := 0; i < 35; i++ {
				m.services.Rows = append(m.services.Rows, operatorJourneyService(fmt.Sprintf("service-%02d", i)))
			}
			updated, _ := m.Update(tea.KeyMsg{Type: tea.KeyEnd})
			m = updated.(operatorModel)
			for _, height := range []int{20, 24, 30} {
				m.height = height
				view := ansi.Strip(m.View())
				if !strings.Contains(view, "> service-34") || !strings.Contains(view, "ID: service-34") {
					t.Fatalf("selection lost %dx%d\n%s", width, height, view)
				}
				rows := strings.Split(view, "\n")
				if len(rows) > height {
					t.Fatal("height overflow")
				}
				for _, row := range rows {
					if ansi.StringWidth(row) > width {
						t.Fatalf("width overflow: %q", row)
					}
				}
			}
			updated, _ = m.Update(tea.KeyMsg{Type: tea.KeyEnter})
			opened := updated.(operatorModel)
			if opened.service != "service-34" || opened.journey != "detail" {
				t.Fatal("Enter opened different service")
			}
		})
	}
}
func TestStackObservationStatesAndUnknown(t *testing.T) {
	m := stackFixture()
	m.services.Rows[0].Running = "yes"
	m.services.Rows[1].Running = "unknown"
	m.services.Rows[0].HealthStatus = "unknown"
	m.services.Rows[1].HealthStatus = "unknown"
	view := m.View()
	for _, want := range []string{"running 1", "stopped 0", "unknown 1", "0 of 2 checked", "unverified 2"} {
		if !strings.Contains(view, want) {
			t.Fatalf("missing %q: %s", want, view)
		}
	}
	m.services.Stale = true
	m.errorText = "Source unavailable"
	if view = m.View(); !strings.Contains(view, "Read failed") || !strings.Contains(view, "stale") {
		t.Fatal(view)
	}
	m.errorText = ""
	m.services.Stale = false
	m.services.Rows = nil
	if view = m.View(); !strings.Contains(view, "no rows") || !strings.Contains(view, "No services") {
		t.Fatal(view)
	}
	m.services.Rows = append(m.services.Rows, operatorJourneyService("private-prior-row"))
	m.epoch++
	if view = m.View(); strings.Contains(view, "private-prior-row") || !strings.Contains(view, "Pending") || !strings.Contains(view, "unavailable") {
		t.Fatal(view)
	}
	m.loading = true
	if !strings.Contains(m.View(), "Loading") {
		t.Fatal("missing pending load")
	}
}
func TestStackFocusAndSafeSummary(t *testing.T) {
	m := stackFixture()
	m.services.Rows[0].DisplayName = "name\x1b[31m"
	m.services.Rows[0].Description = "line\nnext"
	view := m.View()
	if strings.Contains(view, "name\x1b") || !strings.Contains(view, `line\nnext`) {
		t.Fatal("summary not escaped")
	}
	updated, _ := m.Update(tea.KeyMsg{Type: tea.KeyTab})
	m = updated.(operatorModel)
	if view = m.View(); !strings.Contains(view, "Focus: Views") || !strings.Contains(view, "VIEWS") {
		t.Fatal(view)
	}
	m.custodyWarning = "retained-original"
	if !strings.Contains(m.View(), "retained-original") {
		t.Fatal("safety warning hidden")
	}
}

func TestStackAttentionAndSafetyFallback(t *testing.T) {
	m := stackFixture()
	m.services.Rows[0].Running = "no"
	m.services.Rows[0].HealthStatus = "unhealthy"
	m.services.Rows[1].Running = "yes"
	view := m.View()
	for _, want := range []string{"1 stopped or unhealthy", "Stopped", "Running"} {
		if !strings.Contains(view, want) {
			t.Fatal(view)
		}
	}
	for _, rule := range []string{"│", "╭", "╰"} {
		if strings.Contains(view, rule) {
			t.Fatal("boxed stack panel")
		}
	}
	operation, _, _ := terminalRefreshFixture(t)
	m.operation = &api.OperatorReadback{Operation: operation, Result: &api.OperatorResult{Summary: "Verified terminal result"}}
	m.terminalRefreshWarning = "source unavailable"
	view = m.View()
	for _, want := range []string{"Original service refresh failed:", "source unavailable", "Verified terminal result"} {
		if !strings.Contains(view, want) {
			t.Fatalf("safety fallback missing %q\n%s", want, view)
		}
	}
}

func TestStackFailedBootstrapRefreshSingleFlight(t *testing.T) {
	reads := 0
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		reads++
		if r.Method != "GET" {
			t.Error("bootstrap mutated")
		}
		w.WriteHeader(http.StatusServiceUnavailable)
	}))
	defer server.Close()
	client, err := api.NewClient(server.URL, server.Client(), "fixture-token")
	if err != nil {
		t.Fatal(err)
	}
	m := stackFixture()
	m.client = client
	m.ctx = context.Background()
	m.discovery = nil
	m.errorText = "Unavailable"
	if view := m.View(); !strings.Contains(view, "Service Lasso  operator/v2") || !strings.Contains(view, "Runtime API unavailable") {
		t.Fatal(view)
	}
	updated, command := m.Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'r'}})
	m = updated.(operatorModel)
	if command == nil || !m.bootInFlight {
		t.Fatal("no bootstrap retry")
	}
	updated, duplicate := m.Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'r'}})
	m = updated.(operatorModel)
	if duplicate != nil {
		t.Fatal("duplicate bootstrap")
	}
	updated, _ = m.Update(command())
	m = updated.(operatorModel)
	if reads != 1 || m.bootInFlight || m.discovery != nil || !strings.Contains(m.View(), "Runtime API unavailable") {
		t.Fatal("bootstrap failure lost safe state")
	}
	_, retry := m.Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'r'}})
	if retry == nil {
		t.Fatal("failure could not retry")
	}
}
