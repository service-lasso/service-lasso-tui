package app

import (
	"context"
	"errors"
	"strings"
	"testing"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/service-lasso/service-lasso-tui/internal/api"
)

type fakeClient struct {
	health    api.Health
	services  []api.Service
	healthErr error
}

func (f fakeClient) Health(context.Context) (api.Health, error) { return f.health, f.healthErr }
func (f fakeClient) Services(context.Context) ([]api.Service, error) {
	return f.services, nil
}

func TestNavigationShowsServiceDetails(t *testing.T) {
	client := fakeClient{services: []api.Service{{ID: "echo", Name: "Echo", Description: "test service", Enabled: true}}}
	initial := New(client, context.Background()).(model)
	updated, _ := initial.Update(loadedMsg{services: client.services})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyEnter})
	view := updated.(model).View()
	if !strings.Contains(view, "Echo (echo)") || !strings.Contains(view, "esc back") {
		t.Fatalf("detail view did not render selection: %s", view)
	}
}

func TestErrorStateShowsRetryPath(t *testing.T) {
	initial := New(fakeClient{}, context.Background()).(model)
	updated, _ := initial.Update(loadedMsg{err: errors.New("connection refused")})
	view := updated.(model).View()
	if !strings.Contains(view, "Runtime API unavailable") || !strings.Contains(view, "Press r to retry") {
		t.Fatalf("error view omitted retry path: %s", view)
	}
}
