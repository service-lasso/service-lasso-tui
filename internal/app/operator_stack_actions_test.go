package app

import (
	"context"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
	"time"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/x/ansi"
	"github.com/service-lasso/service-lasso-tui/internal/api"
)

func TestStackActionsReadSelectedServiceOnly(t *testing.T) {
	var requests []string
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		requests = append(requests, r.Method+" "+r.URL.Path)
		w.WriteHeader(http.StatusForbidden)
	}))
	defer server.Close()
	m := stackFixture()
	m.ctx = context.Background()
	var err error
	m.client, err = api.NewClient(server.URL, server.Client(), "fixture-token")
	if err != nil {
		t.Fatal(err)
	}
	m.row = 1
	updated, cmd := m.Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune("a")})
	opened := updated.(operatorModel)
	if opened.journey != "lifecycle" || opened.service != "demo-web" || !opened.loading || opened.lifecycle != nil || opened.draft.Dirty || opened.draft.Preview != nil || cmd == nil {
		t.Fatal("Actions did not open a fresh selected-service availability read")
	}
	msg := cmd()
	opened.Update(msg)
	if len(requests) != 1 || requests[0] != "GET /api/operator/v2/services/demo-web/lifecycle-availability" {
		t.Fatalf("navigation dispatched unexpected requests: %v", requests)
	}
}

func TestStackActionsRejectUnavailableSelection(t *testing.T) {
	tests := []struct {
		name  string
		alter func(*operatorModel)
	}{
		{"loading", func(m *operatorModel) { m.loading = true }},
		{"stale", func(m *operatorModel) { m.services.Stale = true }},
		{"expired", func(m *operatorModel) { m.services.Expiry = time.Now().Add(-time.Minute).Format(time.RFC3339Nano) }},
		{"absent snapshot", func(m *operatorModel) { m.services.Snapshot = "" }},
		{"no rows", func(m *operatorModel) { m.services.Rows = nil }},
		{"negative row", func(m *operatorModel) { m.row = -1 }},
		{"past last row", func(m *operatorModel) { m.row = len(m.services.Rows) }},
		{"foreign selection", func(m *operatorModel) { m.epoch++ }},
		{"view focus", func(m *operatorModel) { m.focus = 0 }},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			m := stackFixture()
			test.alter(&m)
			updated, cmd := m.Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune("a")})
			got := updated.(operatorModel)
			if cmd != nil || got.journey != "overview" || got.service != m.service || got.draft.Dirty {
				t.Fatal("unavailable row supplied an action target")
			}
		})
	}
}

func TestStackActionsFooterAndInspect(t *testing.T) {
	t.Setenv("NO_COLOR", "1")
	for _, width := range []int{60, 80, 120} {
		m := stackFixture()
		m.width = width
		m.height = 24
		view := ansi.Strip(m.View())
		if !strings.Contains(view, "a Actions") || !strings.Contains(view, "Enter Inspect") {
			t.Fatalf("missing keys at %d\n%s", width, view)
		}
		for _, line := range strings.Split(view, "\n") {
			if ansi.StringWidth(line) > width {
				t.Fatalf("footer/layout overflow at %d: %q", width, line)
			}
		}
		updated, _ := m.Update(tea.KeyMsg{Type: tea.KeyEnter})
		got := updated.(operatorModel)
		if got.journey != "detail" || got.service != "demo-api" {
			t.Fatal("Enter no longer inspects selected service")
		}
	}
}
