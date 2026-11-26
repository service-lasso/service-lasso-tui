package app

import (
	"fmt"
	"strings"
	"testing"

	"github.com/charmbracelet/x/ansi"
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
				for _, want := range []string{"HEALTH", "NEEDS ATTENTION", "KNOWN OPERATION", "CORE UPTIME", "> demo-web", "ID: demo-web", "unavailable", "Incident history"} {
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
