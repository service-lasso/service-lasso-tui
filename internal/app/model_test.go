package app

import (
	"context"
	"errors"
	"io"
	"net/http"
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

type fakeConnectionManager struct {
	current string
	clients map[string]*api.Client
}

func (m *fakeConnectionManager) Names() []string { return []string{"local", "remote"} }
func (m *fakeConnectionManager) Current() string { return m.current }
func (m *fakeConnectionManager) Switch(name string) (*api.Client, error) {
	client, ok := m.clients[name]
	if !ok {
		return nil, errors.New("unknown profile")
	}
	m.current = name
	return client, nil
}

// rawStatusLineTransport models an HTTP response after net/http has parsed a
// server-controlled status line. It lets this rendered-output regression cover
// the untrusted Response.Status value without placing control bytes on the
// local test process's terminal or network logs.
type rawStatusLineTransport struct {
	status string
	body   string
}

func (t rawStatusLineTransport) RoundTrip(request *http.Request) (*http.Response, error) {
	if request.URL.Path == "/api/health" {
		return &http.Response{
			Status:     "200 OK",
			StatusCode: http.StatusOK,
			Body:       io.NopCloser(strings.NewReader(`{"status":"ok","api":{"status":"up"}}`)),
			Header:     make(http.Header),
			Request:    request,
		}, nil
	}
	return &http.Response{
		Status:     t.status,
		StatusCode: 599,
		Body:       io.NopCloser(strings.NewReader(t.body)),
		Header:     make(http.Header),
		Request:    request,
	}, nil
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
	submitted, command := pending.Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'y'}})
	message := command()
	completed, _ := submitted.(model).Update(message)
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
	submittedModel, command := updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'y'}})
	submitted := submittedModel.(model)
	if command == nil {
		t.Fatal("first confirmation did not dispatch")
	}
	_, second := submitted.Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'y'}})
	if second != nil {
		t.Fatal("second confirmation dispatched another command")
	}
	_ = command()
	if client.calls != 1 {
		t.Fatalf("lifecycle request count = %d, want 1", client.calls)
	}
}

func TestSubmittedLifecycleKeepsNavigationResponsiveAndRendersOriginalResult(t *testing.T) {
	client := &countingClient{fakeClient: fakeClient{
		services:        []api.Service{{ID: "echo", Name: "Echo"}},
		lifecycleResult: api.LifecycleResult{OK: true},
	}}
	local, err := api.NewClient("http://127.0.0.1:17883", nil, "local-token")
	if err != nil {
		t.Fatal(err)
	}
	remote, err := api.NewClient("https://remote.example.test", nil, "remote-token")
	if err != nil {
		t.Fatal(err)
	}
	connections := &fakeConnectionManager{current: "local", clients: map[string]*api.Client{"local": local, "remote": remote}}
	initial := NewWithConnections(client, connections, context.Background()).(model)
	updated, _ := initial.Update(loadedMsg{services: client.services})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'v'}})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyEnter})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'s'}})
	submittedModel, command := updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'y'}})
	submitted := submittedModel.(model)
	if command == nil || !submitted.hasOutstandingAction() {
		t.Fatal("confirmation did not submit lifecycle request")
	}
	if submission := submitted.outstandingAction; submission.client != client || submission.connectionName != "local" || submission.connectionEpoch != 0 || submission.serviceID != "echo" || submission.action != "start" {
		t.Fatalf("submission did not retain its original context: %#v", submission)
	}
	// Profile activation remains blocked, but read/navigation input must keep the
	// UI useful while Core processes the already-submitted mutation.
	for _, key := range []rune{'d', '?'} {
		next, _ := submitted.Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{key}})
		submitted = next.(model)
	}
	if submitted.screen != helpScreen {
		t.Fatalf("help was blocked by submission: %#v", submitted)
	}
	next, _ := submitted.Update(tea.KeyMsg{Type: tea.KeyEsc})
	submitted = next.(model)
	next, _ = submitted.Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'v'}})
	submitted = next.(model)
	next, _ = submitted.Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'/'}})
	submitted = next.(model)
	next, _ = submitted.Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'e'}})
	submitted = next.(model)
	next, _ = submitted.Update(tea.KeyMsg{Type: tea.KeyEnter})
	submitted = next.(model)
	refreshed, refresh := submitted.Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'r'}})
	if refresh == nil {
		t.Fatal("refresh was blocked by submission")
	}
	submitted = refreshed.(model)
	next, blockedProfile := submitted.Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'p'}})
	if blockedProfile != nil {
		t.Fatal("profile selection started a command while a lifecycle request was pending")
	}
	submitted = next.(model)
	if submitted.screen != servicesScreen || submitted.connectionName != "local" || connections.current != "local" {
		t.Fatalf("outstanding submission allowed profile switch or blocked navigation: %#v", submitted)
	}

	completed, _ := submitted.Update(command())
	view := completed.(model).View()
	if client.calls != 1 || client.serviceID != "echo" || strings.Count(view, "Last runtime result: Core completed start.") != 1 {
		t.Fatalf("original lifecycle result was not rendered exactly once after navigation: calls=%d service=%q view=%s", client.calls, client.serviceID, view)
	}
	_, second := completed.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'y'}})
	if second != nil || client.calls != 1 {
		t.Fatalf("completed submission replayed: command=%v calls=%d", second != nil, client.calls)
	}
}

func TestLifecycleFailureDoesNotExposeSensitiveMarker(t *testing.T) {
	const marker = "SYNTHETIC_SENSITIVE_MARKER_DO_NOT_DISPLAY"
	initial := New(fakeClient{}, context.Background()).(model)
	initial.outstandingAction = &lifecycleSubmission{id: 1, action: "start"}
	updated, _ := initial.Update(lifecycleMsg{submissionID: 1, err: errors.New("runtime returned 403 Forbidden")})
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
	updatedModel := updated.(model)
	updatedModel.outstandingAction = &lifecycleSubmission{id: 1, action: "start"}
	updated, _ = updatedModel.Update(lifecycleMsg{
		submissionID: 1,
		result:       api.LifecycleResult{OK: true, Message: marker},
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

func TestRenderedRuntimeErrorOmitsNon2xxResponseBody(t *testing.T) {
	const bodyMarker = "SYNTHETIC_SECRET_NON_2XX_BODY_DO_NOT_RENDER"
	const reasonMarker = "SYNTHETIC_SECRET_REASON_DO_NOT_RENDER"
	client, err := api.NewClient("http://127.0.0.1:17883", &http.Client{Transport: rawStatusLineTransport{
		status: "599 " + reasonMarker + "\x1b[2J",
		body:   `{"message":"` + bodyMarker + `"}`,
	}}, "")
	if err != nil {
		t.Fatal(err)
	}
	initial := New(client, context.Background()).(model)
	message := initial.refresh()()
	updated, _ := initial.Update(message)
	view := updated.(model).View()
	if strings.Contains(view, bodyMarker) || strings.Contains(view, reasonMarker) || strings.Contains(view, "\x1b") {
		t.Fatalf("rendered terminal output leaked a non-2xx body or reason phrase: %q", view)
	}
	if !strings.Contains(view, "GET /api/services") || !strings.Contains(view, "599") || !strings.Contains(view, "Press r to retry") {
		t.Fatalf("rendered terminal output omitted useful safe failure context: %q", view)
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

func TestConnectionSwitchClearsContextAndRejectsOldResults(t *testing.T) {
	local, err := api.NewClient("http://127.0.0.1:17883", nil, "local-token")
	if err != nil {
		t.Fatal(err)
	}
	remote, err := api.NewClient("https://remote.example.test", nil, "remote-token")
	if err != nil {
		t.Fatal(err)
	}
	connections := &fakeConnectionManager{current: "local", clients: map[string]*api.Client{"local": local, "remote": remote}}
	initial := NewWithConnections(local, connections, context.Background()).(model)
	initial.services = []api.Service{{ID: "local", Name: "Local service"}}
	initial.lastResult = "Core completed start."
	updated, _ := initial.Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'p'}})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'j'}})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyEnter})
	switched := updated.(model)
	if switched.connectionName != "remote" || switched.connectionEpoch != 1 || len(switched.services) != 0 || switched.lastResult != "" {
		t.Fatalf("switch retained local context: %#v", switched)
	}
	stale, _ := switched.Update(loadedMsg{epoch: 0, services: []api.Service{{ID: "local", Name: "Old result"}}})
	if strings.Contains(stale.(model).View(), "Old result") {
		t.Fatalf("old connection result was rendered: %s", stale.(model).View())
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

func TestPendingConfirmationCannotDispatchAfterNavigationAway(t *testing.T) {
	client := &countingClient{fakeClient: fakeClient{services: []api.Service{{ID: "echo", Name: "Echo"}}, lifecycleResult: api.LifecycleResult{OK: true}}}
	initial := New(client, context.Background()).(model)
	updated, _ := initial.Update(loadedMsg{services: client.services})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'v'}})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyEnter})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'s'}})
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'d'}})
	_, command := updated.(model).Update(tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{'y'}})
	if command != nil || client.calls != 0 {
		t.Fatalf("hidden confirmation dispatched: command=%v calls=%d", command != nil, client.calls)
	}
	if updated.(model).lastResult != "Confirmation cancelled after navigation." {
		t.Fatalf("navigation cancellation not retained: %#v", updated.(model))
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
