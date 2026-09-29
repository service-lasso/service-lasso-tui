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
	inbox           api.Inbox
	inboxErr        error
	lifecycleResult api.LifecycleResult
	lifecycleErr    error
}

func (f fakeClient) Health(context.Context) (api.Health, error) { return f.health, f.healthErr }
func (f fakeClient) Services(context.Context) ([]api.Service, error) {
	return f.services, nil
}
func (f fakeClient) Capabilities(context.Context) (api.Capabilities, error) {
	return api.Capabilities{ContractVersion: "service-lasso.runtime-capabilities.v1"}, nil
}
func (f fakeClient) SetupStatus(context.Context) (api.SetupStatus, error) {
	return api.SetupStatus{State: "setup_complete"}, nil
}
func (f fakeClient) RuntimeIdentity(context.Context) (api.RuntimeIdentity, error) {
	return api.RuntimeIdentity{Status: "active", Phase: "running"}, nil
}
func (f fakeClient) Inbox(context.Context, string) (api.Inbox, error) { return f.inbox, f.inboxErr }
func (f fakeClient) HealthHistory(context.Context, string) (api.HealthHistory, error) {
	return api.HealthHistory{}, nil
}
func (f fakeClient) Lifecycle(context.Context, string, string) (api.LifecycleResult, error) {
	return f.lifecycleResult, f.lifecycleErr
}

func TestNavigationShowsServiceDetails(t *testing.T) {
	client := fakeClient{services: []api.Service{{ID: "echo", Name: "Echo", Description: "test service", Enabled: true}}}
	initial := New(client, context.Background()).(model)
	updated, _ := initial.Update(loadedMsg{services: client.services})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'v'}})
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
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'v'}})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyEnter})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'s'}})
	pending := updated.(model)
	if !strings.Contains(pending.View(), "Confirm start") {
		t.Fatalf("start action did not request confirmation: %s", pending.View())
	}
	_, command := pending.Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'y'}})
	message := command()
	completed, _ := pending.Update(message)
	if !strings.Contains(completed.(model).View(), "Last runtime result: Core completed start.") {
		t.Fatalf("runtime result not rendered: %s", completed.(model).View())
	}
}

type countingClient struct {
	fakeClient
	calls     int
	serviceID string
}

func (f *countingClient) Lifecycle(_ context.Context, serviceID, _ string) (api.LifecycleResult, error) {
	f.calls++
	f.serviceID = serviceID
	return f.lifecycleResult, f.lifecycleErr
}

func TestLifecycleDoubleConfirmDispatchesOnce(t *testing.T) {
	client := &countingClient{fakeClient: fakeClient{
		services:        []api.Service{{ID: "echo", Name: "Echo"}},
		lifecycleResult: api.LifecycleResult{OK: true, Message: "started"},
	}}
	initial := New(client, context.Background()).(model)
	updated, _ := initial.Update(loadedMsg{services: client.services})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'v'}})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyEnter})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'s'}})
	submitted, command := updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'y'}})
	if command == nil {
		t.Fatal("first confirmation did not dispatch")
	}
	_, second := submitted.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'y'}})
	if second != nil {
		t.Fatal("second confirmation dispatched another command")
	}
	_ = command()
	if client.calls != 1 {
		t.Fatalf("lifecycle request count = %d, want 1", client.calls)
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

func TestLifecycleSuccessMessageDoesNotExposeSensitiveMarker(t *testing.T) {
	const marker = "SYNTHETIC_SENSITIVE_MARKER_DO_NOT_DISPLAY"
	initial := New(fakeClient{services: []api.Service{{ID: "echo", Name: "Echo"}}}, context.Background()).(model)
	updated, _ := initial.Update(loadedMsg{services: []api.Service{{ID: "echo", Name: "Echo"}}})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'v'}})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyEnter})
	updated, _ = updated.(model).Update(lifecycleMsg{
		action: "start",
		result: api.LifecycleResult{OK: true, Message: marker},
	})
	view := updated.(model).View()
	if strings.Contains(view, marker) || !strings.Contains(view, "Core completed start.") {
		t.Fatalf("unexpected lifecycle success rendering: %s", view)
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

func TestDashboardKeepsLastServiceSnapshotAsStale(t *testing.T) {
	initial := New(fakeClient{}, context.Background()).(model)
	updated, _ := initial.Update(loadedMsg{services: []api.Service{{ID: "echo", Name: "Echo"}}})
	updated, _ = updated.(model).Update(loadedMsg{err: errors.New("connection refused")})
	view := updated.(model).View()
	if !strings.Contains(view, "stale") || !strings.Contains(view, "Echo") {
		t.Fatalf("stale snapshot missing: %s", view)
	}
}

func TestDashboardKeyboardViewsSearchResizeAndInbox(t *testing.T) {
	initial := New(fakeClient{}, context.Background()).(model)
	updated, _ := initial.Update(loadedMsg{services: []api.Service{{ID: "echo", Name: "Echo"}, {ID: "other", Name: "Other"}}})
	updated, _ = updated.(model).Update(tea.WindowSizeMsg{Width: 60, Height: 20})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'/'}})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'c'}})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'h'}})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyEnter})
	filtered := updated.(model)
	if !filtered.narrow || strings.Contains(filtered.View(), "Other") {
		t.Fatalf("search or resize not applied: %s", filtered.View())
	}
	updated, _ = filtered.Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'i'}})
	if !strings.Contains(updated.(model).View(), "Operator inbox") {
		t.Fatalf("inbox view missing: %s", updated.(model).View())
	}
}

func TestResizeRecomputesAutomaticNarrowLayout(t *testing.T) {
	initial := New(fakeClient{}, context.Background()).(model)
	updated, _ := initial.Update(tea.WindowSizeMsg{Width: 60, Height: 20})
	if !updated.(model).narrow {
		t.Fatal("narrow layout was not enabled")
	}
	updated, _ = updated.(model).Update(tea.WindowSizeMsg{Width: 120, Height: 20})
	if updated.(model).narrow {
		t.Fatal("narrow layout was not cleared after resize")
	}
}

func TestFilteredServiceConfirmationTargetsDisplayedService(t *testing.T) {
	client := &countingClient{fakeClient: fakeClient{services: []api.Service{{ID: "alpha", Name: "Alpha"}, {ID: "echo", Name: "Echo"}}, lifecycleResult: api.LifecycleResult{OK: true}}}
	initial := New(client, context.Background()).(model)
	updated, _ := initial.Update(loadedMsg{services: client.services})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'/'}})
	for _, key := range []rune{'e', 'c', 'h', 'o'} {
		updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{key}})
	}
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyEnter})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyEnter})
	if !strings.Contains(updated.(model).View(), "Echo (echo)") {
		t.Fatalf("filtered detail mismatch: %s", updated.(model).View())
	}
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'s'}})
	_, command := updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'y'}})
	if command == nil {
		t.Fatal("confirmation did not dispatch")
	}
	_ = command()
	if client.serviceID != "echo" {
		t.Fatalf("lifecycle service = %q, want echo", client.serviceID)
	}
}

func TestPendingConfirmationKeepsItsServiceAcrossSearchChanges(t *testing.T) {
	client := &countingClient{fakeClient: fakeClient{services: []api.Service{{ID: "alpha", Name: "Alpha"}, {ID: "echo", Name: "Echo"}}, lifecycleResult: api.LifecycleResult{OK: true}}}
	initial := New(client, context.Background()).(model)
	updated, _ := initial.Update(loadedMsg{services: client.services})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'v'}})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'j'}})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyEnter})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'s'}})
	if !strings.Contains(updated.(model).View(), "Confirm start for echo") {
		t.Fatalf("confirmation did not name frozen target: %s", updated.(model).View())
	}
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'/'}})
	for _, key := range []rune{'a', 'l', 'p', 'h', 'a'} {
		updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{key}})
	}
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyEnter})
	_, command := updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'y'}})
	if command == nil {
		t.Fatal("confirmation did not dispatch frozen target")
	}
	_ = command()
	if client.serviceID != "echo" {
		t.Fatalf("lifecycle service = %q, want frozen echo", client.serviceID)
	}
}

func TestPendingConfirmationCancelsWhenServiceDisappears(t *testing.T) {
	initial := New(fakeClient{services: []api.Service{{ID: "echo", Name: "Echo"}}}, context.Background()).(model)
	updated, _ := initial.Update(loadedMsg{services: []api.Service{{ID: "echo", Name: "Echo"}}})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'v'}})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyEnter})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'s'}})
	updated, _ = updated.(model).Update(loadedMsg{services: nil})
	if strings.Contains(updated.(model).View(), "Confirm start") || !strings.Contains(updated.(model).View(), "confirmation cancelled") {
		t.Fatalf("missing cancellation: %s", updated.(model).View())
	}
}

func TestDashboardSanitizesTerminalTextAndShowsOptionalReadFailure(t *testing.T) {
	initial := New(fakeClient{}, context.Background()).(model)
	updated, _ := initial.Update(loadedMsg{services: []api.Service{{ID: "echo", Name: "Echo\x1b[31m\nINJECT", Description: "line\r\nnext"}}})
	updated, _ = updated.(model).Update(dashboardMsg{identity: api.RuntimeIdentity{Status: "active\x1b[2J"}, inbox: api.Inbox{Items: []api.InboxItem{{Title: "alert\nitem"}}}, failures: []string{"operator inbox"}})
	view := updated.(model).View()
	if strings.Contains(view, "\x1b") || strings.Contains(view, "\x1b[31m") || !strings.Contains(view, "Unavailable optional reads: operator inbox") {
		t.Fatalf("unsafe terminal text or missing optional failure: %q", view)
	}
}
