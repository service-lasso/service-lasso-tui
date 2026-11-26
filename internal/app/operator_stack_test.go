package app

import (
	"fmt"
	"strings"
	"testing"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/x/ansi"
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
	for _, want := range []string{"running 1", "stopped 0", "unknown 1", "healthy 0", "unknown 2"} {
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
