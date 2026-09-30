package app

import (
	"context"
	"errors"
	"strings"
	"sync"
	"testing"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/service-lasso/service-lasso-tui/internal/api"
)

type fakeClient struct{ services []api.Service }

func (f fakeClient) Health(context.Context) (api.Health, error)      { return api.Health{}, nil }
func (f fakeClient) Services(context.Context) ([]api.Service, error) { return f.services, nil }
func (f fakeClient) Capabilities(context.Context) (api.Capabilities, error) {
	return api.Capabilities{}, nil
}
func (f fakeClient) SetupStatus(context.Context) (api.SetupStatus, error) {
	return api.SetupStatus{}, nil
}
func (f fakeClient) RuntimeIdentity(context.Context) (api.RuntimeIdentity, error) {
	return api.RuntimeIdentity{}, nil
}
func (f fakeClient) Inbox(context.Context, string) (api.Inbox, error) { return api.Inbox{}, nil }
func (f fakeClient) HealthHistory(context.Context, string) (api.HealthHistory, error) {
	return api.HealthHistory{}, nil
}

type durableFake struct {
	fakeClient
	reconciliation string
}

// recordingDurableClient makes the command boundary observable. It is used to
// prove that a frozen preview is submitted unchanged and recovery performs a
// read through the retained client without another lifecycle mutation.
type recordingDurableClient struct {
	fakeClient
	mu                sync.Mutex
	availability      []api.LifecycleAvailability
	preview           api.LifecyclePreview
	submittedPreview  api.LifecyclePreview
	operation         api.Operation
	availabilityErr   error
	previewErr        error
	submitErr         error
	operationErr      error
	cancelErr         error
	availabilityCalls int
	previewCalls      int
	submitCalls       int
	operationCalls    int
	cancelCalls       int
	reconciliation    string
}

func (c *recordingDurableClient) LifecycleAvailability(context.Context, string) ([]api.LifecycleAvailability, error) {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.availabilityCalls++
	return c.availability, c.availabilityErr
}
func (c *recordingDurableClient) PreviewLifecycle(context.Context, string, string) (api.LifecyclePreview, error) {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.previewCalls++
	return c.preview, c.previewErr
}
func (c *recordingDurableClient) SubmitLifecycle(_ context.Context, preview api.LifecyclePreview, _ string) (api.Operation, error) {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.submitCalls++
	c.submittedPreview = preview
	return c.operation, c.submitErr
}
func (c *recordingDurableClient) Operation(context.Context, string) (api.Operation, error) {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.operationCalls++
	return c.operation, c.operationErr
}
func (c *recordingDurableClient) CancelOperation(context.Context, string) (api.Operation, error) {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.cancelCalls++
	return c.operation, c.cancelErr
}
func (c *recordingDurableClient) ReconciliationContext() (string, bool) {
	return c.reconciliation, c.reconciliation != ""
}

func (c *recordingDurableClient) calls() (availability, preview, submit, operation, cancel int) {
	c.mu.Lock()
	defer c.mu.Unlock()
	return c.availabilityCalls, c.previewCalls, c.submitCalls, c.operationCalls, c.cancelCalls
}

func (f durableFake) LifecycleAvailability(context.Context, string) ([]api.LifecycleAvailability, error) {
	return nil, nil
}
func (f durableFake) PreviewLifecycle(context.Context, string, string) (api.LifecyclePreview, error) {
	return api.LifecyclePreview{}, nil
}
func (f durableFake) SubmitLifecycle(context.Context, api.LifecyclePreview, string) (api.Operation, error) {
	return api.Operation{}, errors.New("not called in keyboard test")
}
func (f durableFake) Operation(context.Context, string) (api.Operation, error) {
	return api.Operation{}, nil
}
func (f durableFake) CancelOperation(context.Context, string) (api.Operation, error) {
	return api.Operation{}, nil
}
func (f durableFake) ReconciliationContext() (string, bool) {
	return f.reconciliation, f.reconciliation != ""
}

type fakeConnectionManager struct {
	current string
	clients map[string]*api.Client
}

func (m *fakeConnectionManager) Names() []string { return []string{"local", "remote"} }
func (m *fakeConnectionManager) Current() string { return m.current }
func (m *fakeConnectionManager) Switch(name string) (*api.Client, error) {
	c, ok := m.clients[name]
	if !ok {
		return nil, errors.New("unknown profile")
	}
	m.current = name
	return c, nil
}

func preparedModel(t *testing.T, client runtimeClient) model {
	t.Helper()
	m := New(client, context.Background()).(model)
	m.services, m.selectedServiceID, m.screen = []api.Service{{ID: "echo", Name: "Echo"}}, "echo", detailScreen
	m.pendingAction, m.pendingServiceID = "start", "echo"
	m.pendingPreview = &api.LifecyclePreview{Action: "start", ServiceID: "echo", ConfirmationID: "mcp-confirmation-12345678", ConfirmationPhrase: "confirm start", Targets: []string{"echo"}, Effects: []string{"start"}}
	return m
}
func key(r rune) tea.KeyMsg { return tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{r}} }

func TestConfirmationFreezesEveryUnadvertisedKey(t *testing.T) {
	for _, candidate := range []rune{'d', 'v', 'i', '?', 'r', 'p', '/', 'n', 'q', 'j'} {
		t.Run(string(candidate), func(t *testing.T) {
			m := preparedModel(t, durableFake{})
			before := m
			updated, command := m.Update(key(candidate))
			got := updated.(model)
			if command != nil || got.screen != before.screen || got.pendingAction != "start" || got.pendingServiceID != "echo" || got.pendingPreview == nil || got.searching || got.loading != before.loading {
				t.Fatalf("%q changed frozen confirmation: %#v", candidate, got)
			}
		})
	}
}

func TestConfirmationOnlyEscCancelsAndYSubmits(t *testing.T) {
	m := preparedModel(t, durableFake{})
	updated, command := m.Update(tea.KeyMsg{Type: tea.KeyEsc})
	if command != nil || updated.(model).pendingAction != "" || updated.(model).lastResult != "Confirmation cancelled." {
		t.Fatalf("escape did not cancel: %#v", updated)
	}
	m = preparedModel(t, durableFake{})
	updated, command = m.Update(key('y'))
	if command == nil || updated.(model).operation == nil || updated.(model).pendingAction != "" {
		t.Fatalf("y did not make one durable submission: %#v", updated)
	}
}

func TestConfirmationBackspaceDoesNotCancel(t *testing.T) {
	m := preparedModel(t, durableFake{})
	updated, command := m.Update(tea.KeyMsg{Type: tea.KeyBackspace})
	got := updated.(model)
	if command != nil || got.pendingAction != "start" || got.pendingPreview == nil || got.lastResult != "" {
		t.Fatalf("backspace altered a confirmation that advertises only y and Escape: %#v", got)
	}
}

func TestLateSameEpochListCannotInvalidateFrozenPreview(t *testing.T) {
	client := &recordingDurableClient{operation: api.Operation{ID: "operation-1", Status: "accepted", Phase: "accepted"}}
	m := preparedModel(t, client)
	preview := *m.pendingPreview
	updated, command := m.Update(loadedMsg{epoch: m.connectionEpoch, services: []api.Service{{ID: "other", Name: "Other"}}})
	got := updated.(model)
	if command != nil || got.pendingPreview == nil || got.pendingPreview.Action != preview.Action || got.pendingPreview.ServiceID != preview.ServiceID || got.pendingPreview.ConfirmationID != preview.ConfirmationID || got.pendingServiceID != "echo" {
		t.Fatalf("late list result changed the frozen preview: %#v", got)
	}
	updated, submit := got.Update(key('y'))
	if submit == nil || updated.(model).operation == nil {
		t.Fatal("frozen preview could not be submitted after a late list result")
	}
	message := submit()
	if _, ok := message.(operationMsg); !ok {
		t.Fatalf("submit returned %T, want operationMsg", message)
	}
	_, _, submits, _, _ := client.calls()
	if submits != 1 || client.submittedPreview.Action != preview.Action || client.submittedPreview.ServiceID != preview.ServiceID || client.submittedPreview.ConfirmationID != preview.ConfirmationID {
		t.Fatalf("submission did not use exactly the displayed Core preview: calls=%d preview=%#v", submits, client.submittedPreview)
	}
}

func TestConfirmationSubmitsOnceAndRepeatedEscapeCannotMutate(t *testing.T) {
	client := &recordingDurableClient{operation: api.Operation{ID: "operation-1", Status: "accepted"}}
	m := preparedModel(t, client)
	updated, first := m.Update(key('y'))
	if first == nil {
		t.Fatal("first y did not submit")
	}
	_, second := updated.(model).Update(key('y'))
	if second != nil {
		t.Fatal("repeated y queued a second submission")
	}
	_ = first()
	_, _, submits, _, _ := client.calls()
	if submits != 1 {
		t.Fatalf("submission count = %d, want 1", submits)
	}

	cancelled := preparedModel(t, client)
	updated, command := cancelled.Update(tea.KeyMsg{Type: tea.KeyEsc})
	if command != nil || updated.(model).pendingAction != "" {
		t.Fatalf("Escape did not cancel the frozen preview: %#v", updated)
	}
	_, replay := updated.(model).Update(tea.KeyMsg{Type: tea.KeyEsc})
	if replay != nil {
		t.Fatal("repeated Escape dispatched a lifecycle command")
	}
}

func TestCoreAvailabilityAndPreviewGateConfirmation(t *testing.T) {
	preview := api.LifecyclePreview{Action: "start", ServiceID: "echo", ConfirmationID: "core-confirm-1", ConfirmationPhrase: "confirm", Targets: []string{"echo"}, Effects: []string{"start"}}
	client := &recordingDurableClient{availability: []api.LifecycleAvailability{{Action: "start", Available: true, RequiresConfirmation: true}}, preview: preview}
	m := New(client, context.Background()).(model)
	m.services, m.selectedServiceID, m.screen = []api.Service{{ID: "echo", Name: "Echo"}}, "echo", detailScreen
	updated, prepare := m.Update(key('s'))
	if prepare == nil || !updated.(model).preparingAction {
		t.Fatal("start did not request Core availability and preview")
	}
	confirmed, _ := updated.(model).Update(prepare())
	got := confirmed.(model)
	if got.pendingPreview == nil || got.pendingPreview.ConfirmationID != preview.ConfirmationID || got.preparingAction {
		t.Fatalf("Core preview did not become the confirmation context: %#v", got)
	}
	availability, previews, submits, _, _ := client.calls()
	if availability != 1 || previews != 1 || submits != 0 {
		t.Fatalf("unexpected Core lifecycle calls before confirmation: availability/preview/submit=%d/%d/%d", availability, previews, submits)
	}
}

func TestRefreshReadsRetainedOperationWithItsOriginalClient(t *testing.T) {
	original := &recordingDurableClient{operation: api.Operation{ID: "operation-1", Status: "succeeded", Outcome: "completed"}}
	current := &recordingDurableClient{}
	m := New(current, context.Background()).(model)
	m.operation = &operationSubmission{id: 4, client: original, connectionName: "original", connectionEpoch: 0, operation: api.Operation{ID: "operation-1", Status: "unknown_after_crash"}}

	updated, refresh := m.Update(key('r'))
	if refresh == nil || updated.(model).operation == nil {
		t.Fatal("refresh discarded the retained uncertain operation")
	}
	batch, ok := refresh().(tea.BatchMsg)
	if !ok {
		t.Fatalf("refresh returned %T, want tea.BatchMsg", refresh())
	}
	var readback operationMsg
	for _, command := range batch {
		if message, ok := command().(operationMsg); ok {
			readback = message
		}
	}
	_, _, originalSubmits, originalReads, originalCancels := original.calls()
	_, _, currentSubmits, _, currentCancels := current.calls()
	if originalReads != 1 || originalSubmits != 0 || originalCancels != 0 || currentSubmits != 0 || currentCancels != 0 {
		t.Fatalf("refresh made an operation request other than the retained read: original submit/read/cancel=%d/%d/%d current submit/cancel=%d/%d", originalSubmits, originalReads, originalCancels, currentSubmits, currentCancels)
	}
	finished, _ := updated.(model).Update(readback)
	if finished.(model).operation != nil || !strings.Contains(finished.(model).lastResult, "completed") {
		t.Fatalf("terminal operation readback did not complete recovery: %#v", finished)
	}
}

func TestOperationReadFailureRetainsRecoveryAndSaveFailureDoesNotDiscardIt(t *testing.T) {
	store := failingOperationStore{err: errors.New("save failed")}
	m := New(durableFake{}, context.Background()).(model)
	m.operationStore = &store
	m.operation = &operationSubmission{id: 9, client: durableFake{}, reconciliationContext: "core-context", operation: api.Operation{ID: "operation-9", Status: "running"}}
	updated, _ := m.Update(operationMsg{submissionID: 9, operation: api.Operation{ID: "operation-9", Status: "running"}})
	if updated.(model).operation == nil || updated.(model).err == nil {
		t.Fatalf("persistence failure discarded a nonterminal operation: %#v", updated)
	}
	updated, _ = updated.(model).Update(operationMsg{submissionID: 9, err: errors.New("401 unauthorized")})
	if updated.(model).operation == nil || !strings.Contains(updated.(model).lastResult, "uncertain") {
		t.Fatalf("read failure did not leave terminal recovery available: %#v", updated)
	}
}

func TestAcceptedOperationSurvivesProfileActivationAndDoesNotReplay(t *testing.T) {
	local, err := api.NewClient("http://127.0.0.1:17883", nil, "local-token")
	if err != nil {
		t.Fatal(err)
	}
	remote, err := api.NewClientWithAuth("https://remote.example.test", nil, "remote-token", api.AuthModeOAuthBearer, "service-lasso:read", "service-lasso:lifecycle:write")
	if err != nil {
		t.Fatal(err)
	}
	connections := &fakeConnectionManager{current: "local", clients: map[string]*api.Client{"local": local, "remote": remote}}
	m := NewWithConnections(local, connections, context.Background()).(model)
	m.services, m.selectedServiceID, m.screen = []api.Service{{ID: "echo", Name: "Echo"}}, "echo", detailScreen
	m.operation = &operationSubmission{id: 7, client: local, connectionName: "local", connectionEpoch: 0}
	updated, _ := m.Update(key('p'))
	updated, _ = updated.(model).Update(key('j'))
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyEnter})
	switched := updated.(model)
	if switched.connectionName != "remote" || switched.operation == nil || switched.operation.id != 7 {
		t.Fatalf("profile switch lost submitted operation: %#v", switched)
	}
	accepted := api.Operation{ID: "mcp-operation-12345678", Action: "service_start", Status: "running", Phase: "executing", Progress: 50, Ownership: "own"}
	updated, poll := switched.Update(operationMsg{submissionID: 7, operation: accepted})
	if poll == nil || updated.(model).operation == nil || updated.(model).operation.operation.ID != accepted.ID {
		t.Fatalf("late acceptance was lost: %#v", updated)
	}
	_, replay := updated.(model).Update(key('y'))
	if replay != nil {
		t.Fatal("accepted operation replayed after profile switch")
	}
}

func TestDeniedAndReloadNeverCreateConfirmation(t *testing.T) {
	m := New(fakeClient{services: []api.Service{{ID: "echo", Name: "Echo"}}}, context.Background()).(model)
	m.services, m.selectedServiceID, m.screen = []api.Service{{ID: "echo", Name: "Echo"}}, "echo", detailScreen
	updated, command := m.Update(key('s'))
	if command != nil || updated.(model).pendingAction != "" || !strings.Contains(updated.(model).err.Error(), "durable lifecycle operations") {
		t.Fatalf("non-durable client offered a mutation: %#v", updated)
	}
	m = preparedModel(t, durableFake{})
	m.pendingAction, m.pendingPreview = "", nil
	updated, command = m.Update(key('l'))
	if command != nil || updated.(model).pendingAction != "" || !strings.Contains(updated.(model).err.Error(), "reload is unavailable") {
		t.Fatalf("reload offered a mutation: %#v", updated)
	}
}

func TestCoreDenialClearsPreparingConfirmation(t *testing.T) {
	m := New(durableFake{}, context.Background()).(model)
	m.services, m.selectedServiceID, m.screen = []api.Service{{ID: "echo", Name: "Echo"}}, "echo", detailScreen
	command := m.beginPendingAction("start")
	if command == nil || !m.preparingAction {
		t.Fatalf("durable preview did not start: %#v", m)
	}
	updated, _ := m.Update(command())
	got := updated.(model)
	if got.pendingAction != "" || got.pendingPreview != nil || got.preparingAction || got.err == nil || !strings.Contains(got.err.Error(), "does not currently allow") {
		t.Fatalf("Core denial left a confirmable action: %#v", got)
	}
}

func TestReconnectKeepsUncertainOperationAndCancellationDispatchesOnce(t *testing.T) {
	m := New(durableFake{}, context.Background()).(model)
	m.operation = &operationSubmission{id: 4, client: durableFake{}, operation: api.Operation{ID: "mcp-operation-12345678", Status: "running", Phase: "executing", Ownership: "own", CancellationSupported: true}}
	updated, retry := m.Update(operationMsg{submissionID: 4, err: errors.New("temporary read failure")})
	if retry != nil || updated.(model).operation == nil || !strings.Contains(updated.(model).lastResult, "uncertain") {
		t.Fatalf("uncertain operation was discarded: %#v", updated)
	}
	updated, refresh := updated.(model).Update(key('r'))
	if refresh == nil || updated.(model).operation == nil || updated.(model).operation.id != 4 {
		t.Fatalf("reconnect lost uncertain operation: %#v", updated)
	}
	updated, first := updated.(model).Update(key('z'))
	if first == nil || !updated.(model).operation.cancellationPending {
		t.Fatalf("first cancellation was not marked pending: %#v", updated)
	}
	_, second := updated.(model).Update(key('z'))
	if second != nil {
		t.Fatal("repeated cancellation dispatched while the first request was pending")
	}
}

func TestPersistenceRequiresServerSuppliedContext(t *testing.T) {
	store := &memoryOperationStore{}
	m := preparedModel(t, durableFake{})
	m.operationStore = store
	updated, _ := m.Update(key('y'))
	_, _ = updated.(model).Update(operationMsg{submissionID: 1, operation: api.Operation{ID: "mcp-operation-12345678", Action: "service_start", Status: "running", Phase: "executing", Ownership: "own"}})
	if store.value != nil {
		t.Fatal("operation persisted without a Core reconciliation context")
	}
	m = preparedModel(t, durableFake{reconciliation: "core-context-opaque-123"})
	m.operationStore = store
	updated, _ = m.Update(key('y'))
	_, _ = updated.(model).Update(operationMsg{submissionID: 1, operation: api.Operation{ID: "mcp-operation-12345678", Action: "service_start", Status: "running", Phase: "executing", Ownership: "own"}})
	if store.value == nil || store.value.Version != 2 || store.value.Binding != "core-context-opaque-123" {
		t.Fatalf("server context was not retained: %#v", store.value)
	}
}

type memoryOperationStore struct{ value *persistedOperation }

func (s *memoryOperationStore) Load() (*persistedOperation, error) { return s.value, nil }
func (s *memoryOperationStore) Save(v persistedOperation) error    { s.value = &v; return nil }
func (s *memoryOperationStore) Clear() error                       { s.value = nil; return nil }

type failingOperationStore struct{ err error }

func (s *failingOperationStore) Load() (*persistedOperation, error) { return nil, nil }
func (s *failingOperationStore) Save(persistedOperation) error      { return s.err }
func (s *failingOperationStore) Clear() error                       { return nil }

func TestDashboardNavigationSearchResizeAndSanitizationRemainCovered(t *testing.T) {
	m := New(fakeClient{}, context.Background()).(model)
	updated, _ := m.Update(loadedMsg{services: []api.Service{{ID: "echo", Name: "Echo\x1b[31m\nINJECT"}, {ID: "other", Name: "Other"}}})
	updated, _ = updated.(model).Update(dashboardMsg{identity: api.RuntimeIdentity{Status: "active\x1b[2J"}, inbox: api.Inbox{Items: []api.InboxItem{{Title: "alert\nitem"}}}, failures: []string{"operator inbox"}})
	updated, _ = updated.(model).Update(tea.WindowSizeMsg{Width: 60, Height: 20})
	updated, _ = updated.(model).Update(key('/'))
	updated, _ = updated.(model).Update(key('e'))
	updated, _ = updated.(model).Update(key('n'))
	updated, _ = updated.(model).Update(tea.KeyMsg{Type: tea.KeyEnter})
	got := updated.(model)
	if !got.narrow || strings.Contains(got.View(), "\x1b") || !strings.Contains(got.View(), "Unavailable optional reads: operator inbox") {
		t.Fatalf("navigation, resize, or terminal sanitization regressed: %q", got.View())
	}
	updated, _ = got.Update(key('i'))
	if !strings.Contains(updated.(model).View(), "Operator inbox") {
		t.Fatalf("inbox navigation regressed: %s", updated.(model).View())
	}
}

func TestStaleSnapshotAndEpochIsolationRemainCovered(t *testing.T) {
	m := New(fakeClient{}, context.Background()).(model)
	updated, _ := m.Update(loadedMsg{services: []api.Service{{ID: "echo", Name: "Echo"}}})
	updated, _ = updated.(model).Update(loadedMsg{err: errors.New("connection refused")})
	if !strings.Contains(updated.(model).View(), "stale") || !strings.Contains(updated.(model).View(), "Echo") {
		t.Fatalf("stale service snapshot regressed: %s", updated.(model).View())
	}
	m = updated.(model)
	m.connectionEpoch = 2
	ignored, _ := m.Update(loadedMsg{epoch: 1, services: []api.Service{{ID: "old", Name: "Old result"}}})
	if strings.Contains(ignored.(model).View(), "Old result") {
		t.Fatal("stale connection result was rendered")
	}
}
