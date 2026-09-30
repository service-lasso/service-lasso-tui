package app

import (
	"context"
	"errors"
	"strings"
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

func TestAcceptedOperationSurvivesProfileActivationAndDoesNotReplay(t *testing.T) {
	local, err := api.NewClient("http://127.0.0.1:17883", nil, "local-token")
	if err != nil {
		t.Fatal(err)
	}
	remote, err := api.NewClient("https://remote.example.test", nil, "remote-token")
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
