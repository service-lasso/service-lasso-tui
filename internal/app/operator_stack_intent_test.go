package app

import (
	"strings"
	"testing"
	"time"

	"github.com/charmbracelet/x/ansi"
	"github.com/service-lasso/service-lasso-tui/internal/api"
)

func TestStackAuthoritativeIntentClassification(t *testing.T) {
	t.Setenv("NO_COLOR", "1")
	cases := []struct {
		intent, label string
		attention     bool
	}{
		{"expected", "Expected stopped", true}, {"prepared", "Ready", false},
		{"optional", "Optional stopped", false}, {"disabled", "Disabled", false},
		{"unsupported", "Unsupported", false}, {"gated", "Setup gated", false},
		{"unprepared", "Not prepared", false}, {"", "Stopped / unknown", false},
		{"future-value", "Stopped / unknown", false},
	}
	for _, tc := range cases {
		t.Run(tc.intent, func(t *testing.T) {
			m := stackFixture()
			now := time.Now()
			service := &m.services.Rows[0]
			service.Running = "no"
			service.HealthStatus = "unknown"
			m.stackRuntime.rows = map[string]stackRuntimeService{service.ServiceID: {
				row:      api.OperatorStackServiceRuntime{ServiceID: service.ServiceID, StackIntent: tc.intent},
				revision: service.DefinitionRevision, generation: service.Generation, modelEpoch: service.ModelEpoch, receipt: now,
			}}
			text, color := m.stackIntentStatus(*service, stackServiceChecks{}, "No HTTP check", now)
			if tc.intent == "prepared" && color != operatorGreen {
				t.Fatalf("prepared Ready color %q, want green %q", color, operatorGreen)
			}
			if text != tc.label || m.stackNeedsAttention(*service, now) != tc.attention {
				t.Fatalf("%s classification %q", tc.intent, text)
			}
			for _, size := range [][2]int{{120, 32}, {80, 24}} {
				m.width, m.height = size[0], size[1]
				view := m.View()
				if !strings.Contains(view, tc.label) || !strings.Contains(view, "> demo-api") || !strings.Contains(view, "q Quit") {
					t.Fatalf("%v missing intent/selection/recovery:\n%s", size, view)
				}
				lines := strings.Split(view, "\n")
				if len(lines) != size[1] {
					t.Fatal("height")
				}
				for _, line := range lines {
					if ansi.StringWidth(line) > size[0] {
						t.Fatal("overflow")
					}
				}
			}
			service.HealthStatus = "unhealthy"
			if text, _ := m.stackIntentStatus(*service, stackServiceChecks{}, "No HTTP check", now); text != "Degraded" || !m.stackNeedsAttention(*service, now) {
				t.Fatal("intent hid unhealthy observation")
			}
		})
	}
}

func TestStackIntentCannotOutliveMetadataBinding(t *testing.T) {
	now := time.Now()
	m := stackFixture()
	s := m.services.Rows[0]
	s.Running = "no"
	s.HealthStatus = "unknown"
	m.services.Rows[0] = s
	row := stackRuntimeService{row: api.OperatorStackServiceRuntime{StackIntent: "expected"}, revision: s.DefinitionRevision, generation: s.Generation, modelEpoch: s.ModelEpoch, receipt: now}
	for _, status := range []string{"Access denied", "Read failed", "Unavailable", "Stale", "Pending"} {
		row.status = status
		m.stackRuntime.rows = map[string]stackRuntimeService{s.ServiceID: row}
		if intent, _ := m.stackIntent(s, now); intent != "" || m.stackNeedsAttention(s, now) {
			t.Fatalf("promoted %s metadata", status)
		}
	}
	row.status = ""
	row.generation = "foreign"
	m.stackRuntime.rows[s.ServiceID] = row
	if intent, _ := m.stackIntent(s, now); intent != "" {
		t.Fatal("reused prior generation")
	}
	row.generation = s.Generation
	row.receipt = now.Add(-time.Hour)
	m.stackRuntime.rows[s.ServiceID] = row
	if intent, status := m.stackIntent(s, now); intent != "" || status != "Stale" {
		t.Fatal("stale intent admitted")
	}
}
