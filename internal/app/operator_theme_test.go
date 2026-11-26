package app

import (
	"fmt"
	"strings"
	"testing"
	"time"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/x/ansi"
	"github.com/service-lasso/service-lasso-tui/internal/api"
)

func TestOperatorThemeHomeCellBoundsAndSelection(t *testing.T) {
	for _, width := range []int{80, 120, 160} {
		for _, height := range []int{20, 24, 30} {
			t.Run(fmt.Sprintf("%dx%d", width, height), func(t *testing.T) {
				m := dashboardTestModel()
				m.width = width
				m.height = height
				m.acceptPageSelection(m.ticket())
				m.services.Complete = true
				m.row = 1
				view := ansi.Strip(m.View())
				for _, want := range []string{"HEALTH", "NEEDS ATTENTION", "OPERATION", "CORE UPTIME", "> demo-web", "ID: demo-web", "unavailable", "Incident history"} {
					if !strings.Contains(view, want) {
						t.Fatalf("missing %q\n%s", want, view)
					}
				}
				rows := strings.Split(view, "\n")
				if len(rows) > height {
					t.Fatal("height overflow")
				}
				for _, row := range rows {
					if ansi.StringWidth(row) > width {
						t.Fatalf("width overflow %q", row)
					}
				}
			})
		}
	}
}

func TestOperatorThemeHomeDoesNotPromoteUnavailableTelemetry(t *testing.T) {
	m := dashboardTestModel()
	m.acceptPageSelection(m.ticket())
	m.services.Complete = true
	m.services.Rows[0].Running = "no"
	m.services.Rows[0].HealthStatus = "unknown"
	m.services.Rows[1].Running = "yes"
	m.services.Rows[1].HealthStatus = "unknown"
	view := m.operatorStackView()
	for _, want := range []string{"healthy 0 / 2", "1 stopped or unhealthy", "unverified 2", "Incident history unavailable"} {
		if !strings.Contains(view, want) {
			t.Fatalf("missing %q\n%s", want, view)
		}
	}
	m.epoch++
	if view = m.operatorStackView(); strings.Contains(view, "demo-api") || !strings.Contains(view, "Pending") {
		t.Fatal("rendered prior selection")
	}
}

func TestOperatorThemeUsesActualViewsAndPreservesSafety(t *testing.T) {
	t.Setenv("NO_COLOR", "1")
	m := dashboardTestModel()
	m.journey = "lifecycle"
	m.custodyWarning = "custody retained"
	m.errorText = "permission denied"
	view := m.View()
	for _, want := range []string{"[Lifecycle]", "Focus:", "custody retained", "permission denied"} {
		if !strings.Contains(view, want) {
			t.Fatalf("missing %q\n%s", want, view)
		}
	}
	if strings.Contains(view, "\x1b") {
		t.Fatal("monochrome contains ANSI")
	}
	for _, name := range operatorViews {
		if operatorViewLabel(name) == "" {
			t.Fatalf("missing %s tab", name)
		}
	}
}

func TestOperatorThemeCompactTabsKeepActiveView(t *testing.T) {
	t.Setenv("NO_COLOR", "1")
	for _, journey := range operatorViews {
		m := dashboardTestModel()
		m.width = 80
		m.journey = journey
		if !strings.Contains(m.View(), "["+operatorViewLabel(journey)+"]") {
			t.Fatalf("active tab hidden for %s\n%s", journey, m.View())
		}
	}
}

func TestOperatorThemedDetailKeepsKeyboardTabs(t *testing.T) {
	m := dashboardTestModel()
	m.journey = "detail"
	m.acceptPageSelection(m.ticket())
	port := 8080
	m.detail.Endpoints = []api.OperatorEndpoint{{Label: "public-web", Protocol: "http", Port: &port, Availability: api.OperatorAvailability{Available: true}}}
	for _, test := range []struct{ key, want string }{{"1", "[1 Overview]"}, {"2", "No dependencies reported."}, {"3", "public-web http port=8080 available"}} {
		updated, _ := m.Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune(test.key)})
		m = updated.(operatorModel)
		if view := m.View(); !strings.Contains(view, test.want) {
			t.Fatalf("lost detail tab %s\n%s", test.key, view)
		}
	}
	m.epoch++
	if strings.Contains(m.View(), "public-web") {
		t.Fatal("retained detail escaped current-selection guard")
	}
}

func TestOperatorCompactInspectorAndConnectionProfile(t *testing.T) {
	t.Setenv("NO_COLOR", "1")
	m := dashboardTestModel()
	m.width = 80
	m.height = 20
	m.acceptPageSelection(m.ticket())
	m.connection = "local\nprofile"
	view := m.View()
	for _, want := range []string{`local\nprofile`, "manual refresh", "None known", "ID: demo-api", "Runtime:", "Enter Inspect", "Tab Focus"} {
		if !strings.Contains(view, want) {
			t.Fatalf("missing %q\n%s", want, view)
		}
	}
	lines := strings.Split(view, "\n")
	serviceLine, inspectorLine := -1, -1
	for i, line := range lines {
		if strings.Contains(line, "> demo-api") {
			serviceLine = i
		}
		if strings.Contains(line, "ID: demo-api") {
			inspectorLine = i
		}
	}
	if inspectorLine <= serviceLine {
		t.Fatal("compact inspector did not stack below services")
	}
}

func TestOperatorOverviewReadAgeOnlyAfterAdmittedSuccess(t *testing.T) {
	m := dashboardTestModel()
	page := api.OperatorPage[api.OperatorService]{SnapshotID: "observed", SourceRevision: "revision", ExpiresAt: "2099-10-08T00:00:00.000Z", Rows: []api.OperatorService{operatorJourneyService("demo-api")}}
	ticket := m.ticket()
	before := time.Now()
	updated, _ := m.Update(operatorLoaded{Ticket: ticket, Data: operatorOverviewRead{Page: page}})
	m = updated.(operatorModel)
	if m.overviewObservedAt.Before(before) || !strings.Contains(m.overviewReadAge(time.Now()), "updated ") {
		t.Fatal("successful validated read did not record receipt")
	}
	observed := m.overviewObservedAt
	updated, _ = m.Update(operatorLoaded{Ticket: m.ticket(), Err: &api.OperatorPublicError{Code: "source_unavailable"}})
	m = updated.(operatorModel)
	if m.overviewObservedAt != observed {
		t.Fatal("failed read renewed receipt age")
	}
	m.epoch++
	updated, _ = m.Update(operatorLoaded{Ticket: ticket, Data: operatorOverviewRead{Page: page}})
	m = updated.(operatorModel)
	if m.overviewObservedAt != observed || m.overviewReadAge(time.Now()) != "read age unavailable" {
		t.Fatal("obsolete read acquired freshness")
	}
}

func TestOperatorCompactMonitorRetainsFourServiceRows(t *testing.T) {
	m := dashboardTestModel()
	m.width = 80
	m.height = 24
	m.acceptPageSelection(m.ticket())
	for i := 0; i < 10; i++ {
		m.services.Rows = append(m.services.Rows, operatorJourneyService(fmt.Sprintf("service-%02d", i)))
	}
	m.row = 7
	view := m.View()
	if strings.Count(view, "service-") < 4 {
		t.Fatalf("compact list lost useful service rows\n%s", view)
	}
	if !strings.Contains(view, "> service-05") || !strings.Contains(view, "ID: service-05") {
		t.Fatal("compact selected service/inspector lost")
	}
	for _, line := range strings.Split(view, "\n") {
		if strings.Contains(line, "▌") && strings.Contains(line, "…") {
			t.Fatalf("compact tile truncated %q", line)
		}
	}
}
