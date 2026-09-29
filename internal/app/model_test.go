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
	health          api.Health
	services        []api.Service
	healthErr       error
	lifecycleResult api.LifecycleResult
	lifecycleErr    error
}

func (f fakeClient) Health(context.Context) (api.Health, error) { return f.health, f.healthErr }
func (f fakeClient) Services(context.Context) ([]api.Service, error) {
	return f.services, nil
}
func (f fakeClient) Lifecycle(context.Context, string, string) (api.LifecycleResult, error) {
	return f.lifecycleResult, f.lifecycleErr
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

func TestLifecycleRequiresKeyboardConfirmation(t *testing.T) {
	client := fakeClient{
		services:        []api.Service{{ID: "echo", Name: "Echo"}},
		lifecycleResult: api.LifecycleResult{OK: true, Action: "start", ServiceID: "echo", Message: "started"},
	}
	initial := New(client, context.Background()).(model)
	updated, _ := initial.Update(loadedMsg{services: client.services})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyEnter})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'s'}})
	pending := updated.(model)
	if !strings.Contains(pending.View(), "Confirm start") {
		t.Fatalf("start action did not request confirmation: %s", pending.View())
	}
	_, command := pending.Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'y'}})
	message := command()
	completed, _ := pending.Update(message)
	if !strings.Contains(completed.(model).View(), "Last runtime result: started") {
		t.Fatalf("runtime result not rendered: %s", completed.(model).View())
	}
}

func TestLifecycleFailureDoesNotExposeSensitiveMarker(t *testing.T) {
	const marker = "SYNTHETIC_SENSITIVE_MARKER_DO_NOT_DISPLAY"
	initial := New(fakeClient{}, context.Background()).(model)
	updated, _ := initial.Update(lifecycleMsg{err: errors.New("runtime returned 403 Forbidden")})
	view := updated.(model).View()
	if strings.Contains(view, marker) || !strings.Contains(view, "403 Forbidden") {
		t.Fatalf("unexpected lifecycle error rendering: %s", view)
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
