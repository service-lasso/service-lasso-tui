package app

import (
	"context"
	"fmt"
	"strings"
	"testing"
	"time"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/lipgloss"
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
	for _, want := range []string{"0 healthy · 0/2 checked", "1 stopped or unhealthy", "unverified 2", "Incident history unavailable"} {
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
	for _, want := range []string{`local\nprofile`, "manual refresh", "Unavailable", "ID: demo-api", "Runtime:", "Enter Inspect", "Tab Focus"} {
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

func TestOperatorThemeOnlyAdvertisesAdmittedCancellation(t *testing.T) {
	m := stackFixture()
	m.width, m.height = 120, 30
	operation, _, _ := terminalRefreshFixture(t)
	operation.ActionID = "update_download"
	operation.Status, operation.Phase, operation.Outcome = "running", "downloading_update", nil
	m.operation = &api.OperatorReadback{Operation: operation}
	if strings.Contains(m.View(), "c cancel") {
		t.Fatal("cancellation hint shown for unsupported operation")
	}
	m.operation.Operation.CancellationSupported = true
	if !strings.Contains(m.View(), "c cancel") {
		t.Fatal("cancellation hint missing for an eligible operation")
	}
	m.cancelAttempted = true
	if strings.Contains(m.View(), "c cancel") {
		t.Fatal("cancellation hint invites repeating a cancellation request")
	}

	m.cancelAttempted = false
	m.logSearchDraft = &operatorDraft{}
	if strings.Contains(m.View(), "c cancel") {
		t.Fatal("cancellation hint shown while the search editor owns the keyboard")
	}
	m.logSearchDraft = nil
	outcome := "succeeded"
	m.operation.Operation.Outcome = &outcome
	m.operation.Operation.Status, m.operation.Operation.Phase = "succeeded", "completed"
	if !strings.Contains(m.View(), "x clear settled") {
		t.Fatal("clear hint missing for a settled operation")
	}
	m.draft.Dirty = true
	if strings.Contains(m.View(), "x clear settled") {
		t.Fatal("clear hint shown while a draft owns the keyboard")
	}
}

func TestOperatorResponsiveStackMatrix(t *testing.T) {
	t.Setenv("NO_COLOR", "1")
	for _, size := range [][2]int{{60, 18}, {80, 24}, {100, 28}, {112, 33}, {120, 32}, {160, 45}, {200, 20}} {
		t.Run(fmt.Sprintf("%dx%d", size[0], size[1]), func(t *testing.T) {
			m := stackFixture()
			m.width, m.height = size[0], size[1]
			for i := 0; i < 35; i++ {
				m.services.Rows = append(m.services.Rows, operatorJourneyService(fmt.Sprintf("service-%02d", i)))
			}
			updated, _ := m.Update(tea.KeyMsg{Type: tea.KeyEnd})
			m = updated.(operatorModel)
			view := m.View()
			for _, want := range []string{"[Stack]", "> service-34", "ID: service-34", "Incident history unavailable", "q Quit", "unavailable"} {
				if !strings.Contains(view, want) {
					t.Fatalf("missing %q\n%s", want, view)
				}
			}
			if strings.Contains(view, "\x1b") {
				t.Fatal("monochrome escaped")
			}
			rows := strings.Split(view, "\n")
			if len(rows) > m.height {
				t.Fatal("height overflow")
			}
			for _, row := range rows {
				if ansi.StringWidth(row) > m.width {
					t.Fatalf("width overflow %q", row)
				}
			}
			updated, _ = m.Update(tea.KeyMsg{Type: tea.KeyEnter})
			opened := updated.(operatorModel)
			if opened.service != "service-34" || opened.journey != "detail" {
				t.Fatal("Enter selected a different observation")
			}
			m.errorText = "Source unavailable"
			m.services.Stale = true
			view = m.View()
			for _, want := range []string{"READ FAILED", "Read failed", "Error: Source unavailable", "q Quit"} {
				if !strings.Contains(view, want) {
					t.Fatalf("lost failure/recovery %q\n%s", want, view)
				}
			}
		})
	}
}

func TestOperatorTallStackKeepsAttentionNearObservation(t *testing.T) {
	m := stackFixture()
	m.width, m.height = 160, 45
	view := ansi.Strip(m.View())
	rows := strings.Split(view, "\n")
	attention := -1
	for i, row := range rows {
		if strings.Contains(row, "NEEDS ATTENTION ·") {
			attention = i
			break
		}
	}
	if attention < 0 || attention > 20 {
		t.Fatalf("attention separated from small observation at row %d\n%s", attention, view)
	}
	if !strings.Contains(view, "NAME") || !strings.Contains(view, "STATE") {
		t.Fatal("missing service column headings")
	}
}

func TestOperatorResponsiveSemanticStatusAndBottomRecovery(t *testing.T) {
	for _, test := range []struct {
		running, health string
		want            lipgloss.Color
	}{{"no", "healthy", operatorCoral}, {"yes", "unhealthy", operatorAmber}, {"yes", "healthy", operatorGreen}, {"yes", "unknown", operatorMuted}} {
		if got := stackStatusColor(api.OperatorService{Running: test.running, HealthStatus: test.health}); got != test.want {
			t.Fatalf("wrong status color %s/%s", test.running, test.health)
		}
	}
	m := stackFixture()
	m.width, m.height = 160, 45
	rows := strings.Split(ansi.Strip(m.View()), "\n")
	if len(rows) != m.height || !strings.Contains(rows[m.height-1], "q Quit") || !strings.Contains(rows[m.height-2], "Enter Inspect") {
		t.Fatal("recovery keys not at terminal bottom")
	}
}

func TestStackNumberedNavigationUsesSelectedService(t *testing.T) {
	m := stackFixture()
	m.ctx = context.Background()
	m.row = 1
	for _, test := range []struct{ key, journey string }{{"2", "detail"}, {"3", "history"}, {"4", "logs"}} {
		updated, _ := m.Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune(test.key)})
		got := updated.(operatorModel)
		if got.journey != test.journey || got.service != "demo-web" {
			t.Fatalf("shortcut %s opened %s/%s", test.key, got.journey, got.service)
		}
	}
	m.toggleFocus()
	updated, _ := m.Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune("1")})
	if got := updated.(operatorModel); got.focus != 1 || got.row != 1 {
		t.Fatal("Stack shortcut lost service focus or identity")
	}
	m.epoch++
	updated, _ = m.Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune("3")})
	if got := updated.(operatorModel); got.journey != "overview" {
		t.Fatal("operation history opened without an admitted selected service")
	}
}

func TestStackWideHeaderShowsObservationAndMonitorStates(t *testing.T) {
	m := monitorFixture(t)
	m.width, m.height = 112, 33
	m.overviewObservedAt = time.Now()
	for _, state := range []struct {
		want    string
		prepare func()
	}{
		{"LIVE · updated", func() {}},
		{"PAUSED · updated", func() { m.overviewPaused = true }},
		{"STALE · updated", func() { m.services.Stale = true }},
		{"READ FAILED · updated", func() { m.errorText = "Source unavailable" }},
	} {
		state.prepare()
		if view := ansi.Strip(m.View()); !strings.Contains(view, state.want) {
			t.Fatalf("missing %q\n%s", state.want, view)
		}
	}
}
