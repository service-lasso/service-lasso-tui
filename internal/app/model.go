package app

import (
	"context"
	"errors"
	"fmt"
	"strings"
	"sync"
	"time"
	"unicode"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/service-lasso/service-lasso-tui/internal/api"
)

type runtimeClient interface {
	Health(context.Context) (api.Health, error)
	Services(context.Context) ([]api.Service, error)
	Capabilities(context.Context) (api.Capabilities, error)
	SetupStatus(context.Context) (api.SetupStatus, error)
	RuntimeIdentity(context.Context) (api.RuntimeIdentity, error)
	Inbox(context.Context, string) (api.Inbox, error)
	HealthHistory(context.Context, string) (api.HealthHistory, error)
}

// durableLifecycleClient is deliberately separate from the read dashboard
// client. Older/read-only fixtures cannot accidentally create a mutation.
type durableLifecycleClient interface {
	LifecycleAvailability(context.Context, string) ([]api.LifecycleAvailability, error)
	PreviewLifecycle(context.Context, string, string) (api.LifecyclePreview, error)
	SubmitLifecycle(context.Context, api.LifecyclePreview, string) (api.Operation, error)
	Operation(context.Context, string) (api.Operation, error)
	CancelOperation(context.Context, string) (api.Operation, error)
}

// reconciliationContextProvider is an adapter boundary for Core's pending
// server-issued actor/client/instance reconciliation context. The production
// HTTP client validates the reviewed server endpoint when it exists; the
// qualified Core2633 pin lacks it and fails closed. Local URL, profile, and
// credential material are never a
// substitute for this authority.
type reconciliationContextProvider interface {
	ReconciliationContext() (string, bool, error)
}

type connectionManager interface {
	Names() []string
	Current() string
	Switch(string) (*api.Client, error)
}

type screen int

const (
	dashboardScreen screen = iota
	servicesScreen
	detailScreen
	inboxScreen
	helpScreen
	profilesScreen
)

type model struct {
	client             runtimeClient
	connections        connectionManager
	connectionName     string
	connectionEpoch    uint64
	selectedProfile    string
	ctx                context.Context
	screen             screen
	services           []api.Service
	selectedServiceID  string
	health             api.Health
	capabilities       api.Capabilities
	setup              api.SetupStatus
	identity           api.RuntimeIdentity
	inbox              api.Inbox
	optionalFailures   []string
	historyUnavailable bool
	history            api.HealthHistory
	stale              bool
	searching          bool
	search             string
	narrow             bool
	loading            bool
	err                error
	pendingAction      string
	pendingServiceID   string
	pendingPreview     *api.LifecyclePreview
	preparingAction    bool
	preparationGeneration uint64
	nextSubmissionID   uint64
	operation          *operationSubmission
	// Private accepted failure evidence survives record clearing/navigation. This
	// in-memory graph is not durable capture or measured finite-budget proof.
	operationFailures  []operationMsg
	receivedFailures []operatorReceivedFailure
	operationStore     operationStore
	lastResult         string
	width              int
	height             int
	refreshGeneration  uint64
	historyGeneration  uint64
	historyLoading     bool
	availabilityGeneration uint64
	availabilityService string
	availability       []api.LifecycleAvailability
	availabilityLoading bool
	availabilityUnavailable bool
	lastSnapshot       time.Time
}

type loadedMsg struct {
	generation uint64
	epoch    uint64
	health   api.Health
	services []api.Service
	err      error
}

type lifecyclePreparedMsg struct {
	generation uint64
	epoch             uint64
	action, serviceID string
	preview           api.LifecyclePreview
	err               error
}
type operationMsg struct {
	generation uint64
	attempted bool
	binding string
	submissionID uint64
	operation    api.Operation
	err          error
	cancellation bool
}
type operationPollMsg struct{ submissionID uint64 }
type compatibilityStorageFailure struct{Store operationStore `json:"-"`;Record persistedOperation `json:"-"`;Message operationMsg `json:"-"`}

type operationSubmission struct {
	id                    uint64
	client                durableLifecycleClient
	connectionName        string
	connectionEpoch       uint64
	reconciliationContext string
	cancellationPending   bool
	cancellationAttempted bool
	operation             api.Operation
	preview               *api.LifecyclePreview
	uncertainWithoutID    bool
	readGeneration        uint64
}

type availabilityMsg struct { epoch, generation uint64; serviceID string; actions []api.LifecycleAvailability; err error }

func New(client runtimeClient, ctx context.Context) tea.Model {
	return model{client: client, ctx: ctx, loading: true, screen: dashboardScreen, operationStore: defaultOperationStore()}
}

func NewWithConnections(client runtimeClient, connections connectionManager, ctx context.Context) tea.Model {
	return model{client: client, connections: connections, connectionName: connections.Current(), selectedProfile: connections.Current(), ctx: ctx, loading: true, screen: dashboardScreen, operationStore: defaultOperationStore()}
}

func (m model) Init() tea.Cmd {
	return tea.Batch(m.refresh(), m.refreshDashboard(), m.readStoredOperation())
}

type storedOperationMsg struct {
	generation uint64
	binding string
	bound   bool
	epoch   uint64
	stored *persistedOperation
	err    error
}

func (m model) readStoredOperation() tea.Cmd {
	store := m.operationStore
	epoch := m.connectionEpoch
	generation := m.nextSubmissionID
	client := m.client
	return func() tea.Msg {
		if store == nil {
			return storedOperationMsg{}
		}
		stored, err := store.Load()
		if err != nil || stored == nil { return storedOperationMsg{stored: stored, err: err, epoch: epoch, generation: generation} }
		binding, bound, contextErr := clientReconciliationContext(client)
		return storedOperationMsg{stored: stored, err: errors.Join(err,contextErr), epoch: epoch, generation: generation, binding: binding, bound: bound}
	}
}

func (m model) refresh() tea.Cmd {
	epoch, client, generation := m.connectionEpoch, m.client, m.refreshGeneration
	return func() tea.Msg {
		ctx, cancel := context.WithTimeout(m.ctx, 5*time.Second); defer cancel()
		health, err := client.Health(ctx)
		if err != nil {
			return loadedMsg{epoch: epoch, generation: generation, err: err}
		}
		services, err := client.Services(ctx)
		return loadedMsg{epoch: epoch, generation: generation, health: health, services: services, err: err}
	}
}

type dashboardMsg struct {
	generation uint64
	epoch        uint64
	capabilities api.Capabilities
	setup        api.SetupStatus
	identity     api.RuntimeIdentity
	inbox        api.Inbox
	failures     []string
	causes       []error
}
type historyMsg struct {
	generation uint64
	serviceID string
	epoch   uint64
	history api.HealthHistory
	err     error
}

func (m model) refreshDashboard() tea.Cmd {
	epoch, client, generation := m.connectionEpoch, m.client, m.refreshGeneration
	return func() tea.Msg {
		// Optional reads share one five-second window. A denied optional read
		// remains empty; connectivity is determined only by /api/health.
		ctx, cancel := context.WithTimeout(m.ctx, 5*time.Second)
		defer cancel()
		var capabilities api.Capabilities
		var setup api.SetupStatus
		var identity api.RuntimeIdentity
		var inbox api.Inbox
		var failures []string
		var causes []error
		var failuresMu sync.Mutex
		addFailure := func(name string, err error) {
			if err != nil {
				failuresMu.Lock()
				failures = append(failures, name)
				causes = append(causes,err)
				failuresMu.Unlock()
			}
		}
		var wait sync.WaitGroup
		wait.Add(4)
		go func() {
			defer wait.Done()
			var err error
			capabilities, err = client.Capabilities(ctx)
			addFailure("capabilities", err)
		}()
		go func() {
			defer wait.Done()
			var err error
			setup, err = client.SetupStatus(ctx)
			addFailure("setup status", err)
		}()
		go func() {
			defer wait.Done()
			var err error
			identity, err = client.RuntimeIdentity(ctx)
			addFailure("runtime identity", err)
		}()
		go func() {
			defer wait.Done()
			var err error
			inbox, err = client.Inbox(ctx, "")
			addFailure("operator inbox", err)
		}()
		wait.Wait()
		return dashboardMsg{epoch: epoch, generation: generation, capabilities: capabilities, setup: setup, identity: identity, inbox: inbox, failures: failures,causes:causes}
	}
}

func (m *model) loadHistory() tea.Cmd {
	service, ok := m.selectedService()
	if !ok {
		return nil
	}
	m.historyGeneration++
	m.history, m.historyLoading, m.historyUnavailable = api.HealthHistory{}, true, false
	id, epoch, client, generation := service.ID, m.connectionEpoch, m.client, m.historyGeneration
	return func() tea.Msg {
		ctx, cancel := context.WithTimeout(m.ctx, 5*time.Second); defer cancel()
		history, err := client.HealthHistory(ctx, id)
		return historyMsg{epoch: epoch, generation: generation, serviceID: id, history: history, err: err}
	}
}

func (m *model) loadAvailability() tea.Cmd {
	m.availabilityGeneration++
	m.availability, m.availabilityService, m.availabilityLoading, m.availabilityUnavailable = nil, m.selectedServiceID, true, false
	durable, ok := m.client.(durableLifecycleClient)
	if !ok || m.selectedServiceID == "" { m.availabilityLoading, m.availabilityUnavailable = false, true; return nil }
	id, epoch, generation := m.selectedServiceID, m.connectionEpoch, m.availabilityGeneration
	return func() tea.Msg { ctx, cancel := context.WithTimeout(m.ctx, 5*time.Second); defer cancel(); actions, err := durable.LifecycleAvailability(ctx, id); return availabilityMsg{epoch: epoch, generation: generation, serviceID: id, actions: actions, err: err} }
}

func prepareLifecycle(client durableLifecycleClient, ctx context.Context, epoch, generation uint64, serviceID, action string) tea.Cmd {
	return func() tea.Msg {
		binding, _, contextErr := clientReconciliationContext(client)
		if contextErr != nil { return lifecyclePreparedMsg{epoch:epoch,generation:generation,action:action,serviceID:serviceID,err:contextErr} }
		actions, err := client.LifecycleAvailability(ctx, serviceID)
		if err == nil {
			available := false
			for _, candidate := range actions {
				if candidate.Action == action && candidate.Available && candidate.RequiresConfirmation {
					available = true
					break
				}
			}
			if !available {
				err = &api.OperatorError{Stage: "permission"}
			}
		}
		preview := api.LifecyclePreview{}
		if err == nil {
			preview, err = client.PreviewLifecycle(ctx, serviceID, action)
		}
		if binding != "" { after, valid, contextErr := clientReconciliationContext(client); if contextErr != nil || !valid || after != binding { err = errors.Join(&api.OperatorError{Stage: "context"},err,contextErr) } else if err == nil { preview.ReconciliationContext = binding } }
		return lifecyclePreparedMsg{epoch: epoch, generation: generation, action: action, serviceID: serviceID, preview: preview, err: err}
	}
}

func submitDurableLifecycle(submission operationSubmission, preview api.LifecyclePreview, ctx context.Context) tea.Cmd {
	return func() tea.Msg {
		binding, _, contextErr := clientReconciliationContext(submission.client)
		if contextErr != nil { return operationMsg{submissionID:submission.id,err:contextErr} }
		if preview.ReconciliationContext != "" && binding != preview.ReconciliationContext { return operationMsg{submissionID: submission.id, err: &api.OperatorError{Stage: "context"}} }
		actions, err := submission.client.LifecycleAvailability(ctx, preview.ServiceID)
		if err != nil { return operationMsg{submissionID: submission.id, err: err} }
		allowed := false
		for _, action := range actions { if action.Action == preview.Action && action.Available && action.RequiresConfirmation { allowed = true } }
		if !allowed { return operationMsg{submissionID: submission.id, err: &api.OperatorError{Stage: "permission"}} }
		key, err := api.NewIdempotencyKey()
		if err != nil {
			return operationMsg{submissionID: submission.id, err: err}
		}
		operation, err := submission.client.SubmitLifecycle(ctx, preview, key)
		if binding != "" { after, valid, contextErr := clientReconciliationContext(submission.client); if contextErr != nil || !valid || after != binding { err = errors.Join(&api.OperatorError{Stage: "context"},err,contextErr) } }
		return operationMsg{submissionID: submission.id, operation: operation, err: err, binding: binding, attempted: true}
	}
}

func readOperation(submission operationSubmission, ctx context.Context) tea.Cmd {
	if submission.operation.ID == "" { return nil }
	return func() tea.Msg {
		if err:=submissionContextFailure(submission);err!=nil { return operationMsg{submissionID: submission.id, generation: submission.readGeneration, err:err} }
		operation, err := submission.client.Operation(ctx, submission.operation.ID)
		if contextErr:=submissionContextFailure(submission);contextErr!=nil { err = errors.Join(contextErr,err) }
		return operationMsg{submissionID: submission.id, generation: submission.readGeneration, operation: operation, err: err, binding:submission.reconciliationContext, attempted:true}
	}
}

func cancelOperation(submission operationSubmission, ctx context.Context) tea.Cmd {
	return func() tea.Msg {
		if err:=submissionContextFailure(submission);err!=nil { return operationMsg{submissionID: submission.id, generation: submission.readGeneration, err:err, cancellation: true} }
		operation, err := submission.client.CancelOperation(ctx, submission.operation.ID)
		if contextErr:=submissionContextFailure(submission);contextErr!=nil { err = errors.Join(contextErr,err) }
		return operationMsg{submissionID: submission.id, generation: submission.readGeneration, operation: operation, err: err, binding:submission.reconciliationContext, cancellation: true, attempted:true}
	}
}

func submissionContextFailure(submission operationSubmission) error { if submission.reconciliationContext == "" { return nil }; binding, ok, err := clientReconciliationContext(submission.client); if err!=nil||!ok||binding!=submission.reconciliationContext{return errors.Join(&api.OperatorError{Stage:"context"},err)};return nil }

func (m *model) readRetainedOperation() tea.Cmd { if m.operation == nil || m.operation.operation.ID == "" { return nil }; m.operation.readGeneration++; return readOperation(*m.operation, m.ctx) }

func (m model) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	var receivedErr error
	switch message:=msg.(type){case loadedMsg:receivedErr=message.err;case dashboardMsg:receivedErr=errors.Join(message.causes...);case historyMsg:receivedErr=message.err;case availabilityMsg:receivedErr=message.err;case lifecyclePreparedMsg:receivedErr=message.err;case storedOperationMsg:receivedErr=message.err;case operationMsg:receivedErr=message.err}
	if receivedErr!=nil{m.receivedFailures=append(m.receivedFailures,operatorReceivedFailure{Message:msg,Err:receivedErr})}
	switch message := msg.(type) {
	case tea.WindowSizeMsg:
		m.width, m.height = message.Width, message.Height
		m.narrow = message.Width > 0 && message.Width < 72
	case loadedMsg:
		if message.epoch != m.connectionEpoch || message.generation != m.refreshGeneration {
			return m, nil
		}
		// The confirmation view is a record of one Core-issued preview. A list
		// result can have been started before that view was entered, so it must
		// not alter selection or invalidate the advertised target while the
		// operator can still choose y or Escape.
		if m.pendingAction != "" {
			return m, nil
		}
		m.loading = false
		m.err = message.err
		if message.err == nil {
			m.health, m.services = message.health, message.services
			m.stale = false
			m.lastSnapshot = time.Now()
			m.ensureSelectedService()
			if m.screen == detailScreen { command := tea.Batch(m.loadHistory(), m.loadAvailability()); return m, command }
		} else if len(m.services) > 0 {
			m.stale = true
		}
	case dashboardMsg:
		if message.epoch != m.connectionEpoch || message.generation != m.refreshGeneration {
			return m, nil
		}
		m.capabilities, m.setup, m.identity, m.inbox, m.optionalFailures = message.capabilities, message.setup, message.identity, message.inbox, message.failures
	case historyMsg:
		if message.epoch != m.connectionEpoch || message.generation != m.historyGeneration || message.serviceID != m.selectedServiceID {
			return m, nil
		}
		m.historyLoading = false
		if message.err == nil && message.history.ServiceID == message.serviceID {
			m.history = message.history
			m.historyUnavailable = false
		} else {
			m.historyUnavailable = true
		}
	case availabilityMsg:
		if message.epoch != m.connectionEpoch || message.generation != m.availabilityGeneration || message.serviceID != m.selectedServiceID || m.pendingAction != "" { return m, nil }
		m.availabilityLoading, m.availabilityUnavailable = false, message.err != nil
		if message.err == nil { m.availability = message.actions } else { m.availability = nil }
	case lifecyclePreparedMsg:
		if message.epoch != m.connectionEpoch || message.generation != m.preparationGeneration || message.serviceID != m.pendingServiceID || message.action != m.pendingAction {
			return m, nil
		}
		m.preparingAction = false
		if message.err != nil {
			m.pendingAction, m.pendingServiceID, m.pendingPreview = "", "", nil
			m.err = message.err
			m.lastResult = ""
			return m, nil
		}
		if message.preview.Action != message.action || message.preview.ServiceID != message.serviceID { m.pendingAction, m.pendingServiceID = "", ""; m.err = &api.OperatorError{Stage: "response"}; return m, nil }
		message.preview.Targets, message.preview.Effects = append([]string(nil), message.preview.Targets...), append([]string(nil), message.preview.Effects...)
		m.pendingPreview = &message.preview
	case storedOperationMsg:
		if message.epoch != m.connectionEpoch || message.generation != m.nextSubmissionID {
			return m, nil
		}
		// A delayed startup read must not steal a submitted operation's client,
		// context or identity, including one retained after a storage failure.
		if m.operation != nil {
			return m, nil
		}
		if message.err != nil {
			m.err = message.err
			return m, nil
		}
		if message.stored == nil || message.stored.ConnectionName != m.connectionName {
			return m, nil
		}
		binding, matches := message.binding, message.bound
		if !matches || binding != message.stored.Binding {
			m.lastResult = "Retained operation belongs to a different actor or connection; it was not read or replayed."
			return m, nil
		}
		durable, ok := m.client.(durableLifecycleClient)
		if !ok {
			return m, nil
		}
		m.nextSubmissionID++
		m.operation = &operationSubmission{id: m.nextSubmissionID, client: durable, connectionName: m.connectionName, connectionEpoch: m.connectionEpoch, reconciliationContext: binding, operation: api.Operation{ID: message.stored.OperationID, Status: "reconciling", Phase: "reconciling"}}
		command := m.readRetainedOperation(); return m, command
	case operationMsg:
		if m.operation == nil || message.submissionID != m.operation.id || message.generation != m.operation.readGeneration {
			return m, nil
		}
		if message.err != nil {
			m.operationFailures = append(m.operationFailures,message)
			if message.cancellation {
				m.operation.cancellationPending = false
			}
			m.err = message.err
			if m.operation.operation.ID == "" {
				if !message.attempted { m.operation = nil; m.lastResult = "Action was not submitted; Core availability or local preparation failed."; return m, nil }
				m.operation.uncertainWithoutID = true
				m.lastResult = "Submission outcome is uncertain before an operation ID was received. Refresh cannot reconcile it; Core correlation support is required. No mutation will be replayed."
			} else { m.lastResult = "Operation outcome is uncertain; refresh only reads the retained operation ID." }
			return m, nil
		}
		if message.operation.ID == "" || (m.operation.operation.ID != "" && message.operation.ID != m.operation.operation.ID) || (message.binding != "" && m.operation.reconciliationContext != "" && message.binding != m.operation.reconciliationContext) {
			m.err = fmt.Errorf("operation readback does not match its retained identity or context")
			return m, nil
		}
		if m.operation.preview != nil && !api.MatchOperation(message.operation, *m.operation.preview) { m.err = &api.OperatorError{Stage: "response"}; return m, nil }
		if m.operation.preview == nil && m.operation.operation.Action != "" && (message.operation.Action != m.operation.operation.Action || message.operation.Ownership != "own") { m.err = &api.OperatorError{Stage: "response"}; return m, nil }
		if message.binding != "" && m.operation.reconciliationContext == "" {
			m.operation.reconciliationContext = message.binding
		}
		m.operation.operation = message.operation
		if message.cancellation {
			m.operation.cancellationPending = false
			m.lastResult = "Core cancellation: " + safeTerminalText(message.operation.CancellationResult, 32) + "."
		}
		m.err = nil
		retained := persistedOperation{Version: 2, OperationID: message.operation.ID, ConnectionName: m.operation.connectionName, Binding: m.operation.reconciliationContext}
		persisted := false
		if m.operationStore != nil && m.operation.reconciliationContext != "" {
			if err := m.operationStore.Save(retained); err != nil {
				m.receivedFailures=append(m.receivedFailures,operatorReceivedFailure{Message:compatibilityStorageFailure{Store:m.operationStore,Record:retained,Message:message},Err:err})
				m.err = err
			} else {
				persisted = true
			}
		}
		if message.operation.Outcome != "" || message.operation.Status == "unknown_after_crash" {
			m.lastResult = fmt.Sprintf("Core operation %s: %s.", safeTerminalText(message.operation.ID, 80), safeTerminalText(firstNonEmpty(message.operation.Outcome, message.operation.Status), 48))
			if message.operation.Phase == "interrupted" || message.operation.Status == "unknown_after_crash" { m.lastResult = "Core operation " + safeTerminalText(message.operation.ID, 80) + ": authoritative mutation outcome unavailable (" + safeTerminalText(message.operation.Status, 32) + "/" + safeTerminalText(message.operation.Phase, 32) + "). No mutation was replayed." }
			// Failure to replace B must leave the original durable A untouched.
			// Keep this submission for read-only recovery when either write fails.
			if m.err != nil {
				return m, nil
			}
			if persisted {
				if err := m.operationStore.Clear(retained); err != nil {
					m.receivedFailures=append(m.receivedFailures,operatorReceivedFailure{Message:compatibilityStorageFailure{Store:m.operationStore,Record:retained,Message:message},Err:err})
					m.err = err
					return m, nil
				}
			}
			m.operation = nil
			m.refreshGeneration++; m.loading, m.stale = true, true
			return m, m.refresh()
		}
		return m, tea.Tick(250*time.Millisecond, func(time.Time) tea.Msg { return operationPollMsg{submissionID: message.submissionID} })
	case operationPollMsg:
		if m.operation == nil || message.submissionID != m.operation.id {
			return m, nil
		}
		command := m.readRetainedOperation(); return m, command
	case tea.KeyMsg:
		// A Core preview freezes the exact context shown to the operator. While
		// it is visible, only the advertised confirm/cancel keys can affect UI
		// state; navigation, refresh, profile selection, search, and quit are
		// deliberately ignored.
		if m.pendingAction != "" {
			switch message.String() {
			case "esc":
				m.preparationGeneration++
				m.loading, m.availabilityLoading, m.availabilityUnavailable = false, false, true
				m.pendingAction, m.pendingServiceID, m.pendingPreview, m.preparingAction = "", "", nil, false
				m.lastResult = "Confirmation cancelled."
				return m, nil
			case "y":
				if !m.preparingAction && m.pendingPreview != nil && m.operation == nil {
					command := m.submitPendingAction(); return m, command
				}
			}
			return m, nil
		}
		if message.Type == tea.KeyRunes && string(message.Runes) == "/" {
			m.searching = true
			return m, nil
		}
		if m.searching {
			switch message.String() {
			case "esc":
				m.searching = false
				return m, nil
			case "backspace":
				if len(m.search) > 0 {
					editor:=operatorDraft{Text:m.search,Cursor:len(m.search)};editor.erase(true);m.search=editor.Text
				}
				return m, nil
			case "enter":
				m.searching = false
				return m, nil
			}
			if len(message.Runes) > 0 {
				m.search = safeTerminalText(m.search+string(message.Runes), 80)
				m.ensureSelectedService()
			}
			return m, nil
		}
		switch message.String() {
		case "ctrl+c", "q":
			return m, tea.Quit
		case "r":
			m.refreshGeneration++
			m.loading, m.err = true, nil
			commands := []tea.Cmd{m.refresh(), m.refreshDashboard()}
			if m.operation != nil {
				// A retained operation is read using the client captured when it was
				// submitted. Refresh must never construct a new request or bind the
				// operation to the currently selected profile.
				if m.operation.operation.ID != "" { commands = append(commands, m.readRetainedOperation()) }
			}
			return m, tea.Batch(commands...)
		case "p":
			if m.connections != nil {
				m.selectedProfile = m.connectionName
				m.screen = profilesScreen
			}
		case "d":
			m.screen = dashboardScreen
		case "v":
			m.screen = servicesScreen
		case "i":
			if m.screen == detailScreen && m.hasSelectedService() {
				command := m.beginPendingAction("install"); return m, command
			} else {
				m.screen = inboxScreen
			}
		case "?":
			m.screen = helpScreen
		case "/":
			m.searching = true
		case "n":
			m.narrow = !m.narrow
		case "esc", "backspace":
			m.screen = dashboardScreen
		case "c", "s", "x", "R", "l":
			if m.screen == detailScreen && m.hasSelectedService() && m.operation == nil {
				command := m.beginPendingAction(map[string]string{
					"c": "config", "s": "start", "x": "stop", "R": "restart", "l": "reload",
				}[message.String()]); return m, command
			}
		case "z":
			if m.operation != nil && m.operation.preview != nil && m.operation.operation.CancellationSupported && !m.operation.cancellationPending && !m.operation.cancellationAttempted && m.operation.operation.ID != "" {
				m.operation.cancellationPending = true
				m.operation.cancellationAttempted = true
				m.operation.readGeneration++
				return m, cancelOperation(*m.operation, m.ctx)
			}
		case "down", "j":
			m.moveSelected(1)
		case "up", "k":
			m.moveSelected(-1)
		case "enter":
			if m.screen == profilesScreen {
				client, err := m.connections.Switch(m.selectedProfile)
				if err != nil {
					m.err = err
					return m, nil
				}
				m.activateConnection(client)
				return m, tea.Batch(m.refresh(), m.refreshDashboard())
			}
			if (m.screen == servicesScreen || m.screen == dashboardScreen) && m.hasSelectedService() {
				m.screen = detailScreen
				command := tea.Batch(m.loadHistory(), m.loadAvailability()); return m, command
			}
		}
	}
	return m, nil
}

func (m model) filteredServices() []api.Service {
	if m.search == "" {
		return m.services
	}
	var filtered []api.Service
	for _, service := range m.services {
		if strings.Contains(strings.ToLower(safeTerminalText(service.Name, 120)+" "+safeTerminalText(service.ID, 128)), strings.ToLower(safeTerminalText(m.search, 80))) {
			filtered = append(filtered, service)
		}
	}
	return filtered
}

func (m *model) ensureSelectedService() {
	if _, ok := m.selectedService(); ok {
		return
	}
	filtered := m.filteredServices()
	if len(filtered) > 0 {
		m.selectedServiceID = filtered[0].ID
	} else {
		m.selectedServiceID = ""
	}
}

func (m model) selectedService() (api.Service, bool) {
	for _, service := range m.filteredServices() {
		if service.ID == m.selectedServiceID {
			return service, true
		}
	}
	return api.Service{}, false
}

func (m model) serviceByID(id string) (api.Service, bool) {
	for _, service := range m.services {
		if service.ID == id {
			return service, true
		}
	}
	return api.Service{}, false
}

func (m model) hasServiceID(id string) bool { _, ok := m.serviceByID(id); return ok }

func (m *model) beginPendingAction(action string) tea.Cmd {
	if m.operation != nil || m.pendingAction != "" {
		return nil
	}
	service, ok := m.selectedService()
	if !ok {
		return nil
	}
	durable, ok := m.client.(durableLifecycleClient)
	if !ok {
		m.err = fmt.Errorf("durable lifecycle operations are unavailable for this runtime client")
		m.lastResult = ""
		return nil
	}
	if action == "reload" {
		m.err = fmt.Errorf("reload is unavailable: Core does not provide a durable operation contract")
		m.lastResult = ""
		return nil
	}
	m.pendingAction, m.pendingServiceID, m.pendingPreview, m.preparingAction = action, service.ID, nil, true
	m.loading = false
	m.preparationGeneration++
	return prepareLifecycle(durable, m.ctx, m.connectionEpoch, m.preparationGeneration, service.ID, action)
}

func (m *model) submitPendingAction() tea.Cmd {
	if m.pendingPreview != nil {
		durable, ok := m.client.(durableLifecycleClient)
		if !ok {
			m.err = fmt.Errorf("durable lifecycle operations are unavailable for this runtime client")
			return nil
		}
		m.nextSubmissionID++
		binding := ""
		preview := *m.pendingPreview
		preview.Targets, preview.Effects = append([]string(nil), preview.Targets...), append([]string(nil), preview.Effects...)
		submission := operationSubmission{id: m.nextSubmissionID, client: durable, connectionName: m.connectionName, connectionEpoch: m.connectionEpoch, reconciliationContext: binding, preview: &preview}
		m.pendingAction, m.pendingServiceID, m.pendingPreview, m.preparingAction = "", "", nil, false
		m.operation = &submission
		m.loading, m.err = false, nil
		return submitDurableLifecycle(submission, preview, m.ctx)
	}
	return nil
}

func (m model) hasSelectedService() bool { _, ok := m.selectedService(); return ok }

func (m *model) moveSelected(direction int) {
	if m.screen == profilesScreen {
		names := m.connections.Names()
		if len(names) == 0 {
			return
		}
		current := 0
		for i, name := range names {
			if name == m.selectedProfile {
				current = i
				break
			}
		}
		next := current + direction
		if next >= 0 && next < len(names) {
			m.selectedProfile = names[next]
		}
		return
	}
	if m.screen != servicesScreen && m.screen != dashboardScreen {
		return
	}
	services := m.filteredServices()
	if len(services) == 0 {
		m.selectedServiceID = ""
		return
	}
	current := 0
	for i, service := range services {
		if service.ID == m.selectedServiceID {
			current = i
			break
		}
	}
	next := current + direction
	if next >= 0 && next < len(services) {
		m.selectedServiceID = services[next].ID
	}
}

func (m *model) activateConnection(client runtimeClient) {
	m.client = client
	m.connectionName = m.connections.Current()
	m.connectionEpoch++
	m.preparationGeneration++
	m.refreshGeneration++; m.historyGeneration++; m.availabilityGeneration++
	m.historyLoading, m.availabilityLoading, m.availabilityUnavailable = false, false, false
	m.availability, m.availabilityService, m.lastSnapshot = nil, "", time.Time{}
	m.services = nil
	m.selectedServiceID = ""
	m.health, m.capabilities, m.setup, m.identity, m.inbox, m.history = api.Health{}, api.Capabilities{}, api.SetupStatus{}, api.RuntimeIdentity{}, api.Inbox{}, api.HealthHistory{}
	m.optionalFailures, m.pendingAction, m.pendingServiceID, m.lastResult = nil, "", "", ""
	m.pendingPreview, m.preparingAction = nil, false
	m.historyUnavailable, m.stale, m.loading = false, false, true
	m.err = nil
	m.screen = dashboardScreen
}

func clientReconciliationContext(client any) (string, bool, error) {
	provider, ok := client.(reconciliationContextProvider)
	if !ok {
		return "", false, nil
	}
	context, ok, err := provider.ReconciliationContext()
	if err!=nil{return "",false,err}
	return context, ok && context != "",nil
}

func safeTerminalText(value string, limit int) string {
	var b strings.Builder
	ansi := false
	written := 0
	for _, r := range value {
		if ansi {
			if r >= '@' && r <= '~' {
				ansi = false
			}
			continue
		}
		if r == '\x1b' {
			ansi = true
			continue
		}
		if unicode.IsControl(r) {
			if r == '\n' || r == '\r' || r == '\t' {
				b.WriteByte(' ')
			}
			continue
		}
		b.WriteRune(r)
		written++
		if written >= limit {
			b.WriteString("…")
			break
		}
	}
	return strings.TrimSpace(b.String())
}

func firstNonEmpty(values ...string) string {
	for _, value := range values {
		if value != "" {
			return value
		}
	}
	return "unknown"
}

func (m model) View() string {
	var b strings.Builder
	b.WriteString("Service Lasso TUI\n")
	if m.loading {
		b.WriteString("Connecting to runtime API…\n")
	} else if m.err != nil {
		b.WriteString(api.ErrorTitle(m.err) + ": " + api.SafeError(m.err) + "\nPress r to retry.\n")
		if m.stale {
			b.WriteString("Showing last successful service snapshot (stale).\n")
		}
	} else {
		b.WriteString(fmt.Sprintf("Runtime: %s (API %s)\n", safeTerminalText(m.health.Status, 40), safeTerminalText(m.health.API.Status, 40)))
		if m.stale {
			b.WriteString("Showing last successful service snapshot (stale).\n")
		}
	}
	b.WriteString("\n")
	if !m.lastSnapshot.IsZero() { fmt.Fprintf(&b, "Service snapshot age: %ds%s\n", int(time.Since(m.lastSnapshot).Seconds()), map[bool]string{true: " (stale)", false: ""}[m.stale || time.Since(m.lastSnapshot) >= 15*time.Second]) }
	if m.lastResult != "" {
		b.WriteString("Last runtime result: " + m.lastResult + "\n\n")
	}
	if m.screen == helpScreen {
		b.WriteString("d dashboard • v services • i inbox • p connections • / search • n narrow • r reconnect • esc back • q quit\n")
		return b.String()
	}
	if m.screen == profilesScreen {
		b.WriteString("Connections\n")
		for _, name := range m.connections.Names() {
			cursor := " "
			if name == m.selectedProfile {
				cursor = ">"
			}
			active := ""
			if name == m.connectionName {
				active = " (active)"
			}
			fmt.Fprintf(&b, "%s %s%s\n", cursor, safeTerminalText(name, 80), active)
		}
		b.WriteString("j/k navigate • enter switch • esc back\n")
		return b.String()
	}
	if m.screen == inboxScreen {
		return m.inboxView(&b)
	}
	if m.screen == detailScreen {
		return m.detailView(&b)
	}
	if m.screen == dashboardScreen {
		fmt.Fprintf(&b, "Runtime identity: %s (%s)\nCapabilities: %s\nSetup: %s\nInbox: %d item(s)\n", safeTerminalText(m.identity.Status, 40), safeTerminalText(m.identity.Phase, 40), safeTerminalText(m.capabilities.ContractVersion, 80), safeTerminalText(m.setup.State, 40), m.inbox.Total)
		if len(m.optionalFailures) > 0 {
			b.WriteString("Unavailable optional reads: " + strings.Join(m.optionalFailures, ", ") + "\n")
		}
		b.WriteString("\n")
		b.WriteString("Services needing attention\n")
	}
	for _, service := range m.filteredServices() {
		cursor := " "
		if service.ID == m.selectedServiceID {
			cursor = ">"
		}
		state := service.Lifecycle.State
		if state == "" {
			state = "unknown"
		}
		health := service.Health.Status
		if health == "" {
			health = "unknown"
		}
		fmt.Fprintf(&b, "%s %-24s %-12s %-12s %s\n", cursor, safeTerminalText(service.Name, 80), safeTerminalText(state, 40), safeTerminalText(health, 40), safeTerminalText(service.ID, 128))
	}
	if len(m.services) == 0 && !m.loading && m.err == nil {
		b.WriteString("No services returned by the runtime.\n")
	}
	if m.searching {
		b.WriteString("\nSearch: " + safeTerminalText(m.search, 80) + "\n")
	}
	b.WriteString("\n↑/k ↓/j navigate • enter details • d dashboard • v services • i inbox • p connections • / search • ? help • r reconnect • q quit\n")
	return b.String()
}

func (m model) detailView(b *strings.Builder) string {
	service, selected := m.selectedService()
	if !selected {
		b.WriteString("No service selected.\n")
	} else {
		fmt.Fprintf(b, "%s (%s)\n\n%s\n\nLifecycle: %s\nHealth: %s\nEnabled: %t\nHistory transitions: %d\n", safeTerminalText(service.Name, 120), safeTerminalText(service.ID, 128), safeTerminalText(service.Description, 240), safeTerminalText(service.Lifecycle.State, 40), safeTerminalText(service.Health.Status, 40), service.Enabled, m.history.Entries)
		if m.historyUnavailable {
			b.WriteString("Health history is unavailable.\n")
		}
		if m.historyLoading { b.WriteString("Health history loading for selected service.\n") }
	}
	if m.operation != nil {
		if m.operation.uncertainWithoutID { b.WriteString("\nUncertain submission: no operation ID; read recovery unavailable.\n") }
		fmt.Fprintf(b, "\nCore operation %s: %s (%d%%)\n", safeTerminalText(m.operation.operation.ID, 80), safeTerminalText(m.operation.operation.Phase, 48), m.operation.operation.Progress)
		fmt.Fprintf(b, "Server-reported targets: %s\n", safeTerminalText(strings.Join(m.operation.operation.Targets, ", "), 240))
		if m.operation.preview == nil { b.WriteString("Targets are server-reported; original request/preview correlation is unproved. Restored cancellation unavailable.\n") }
		if m.operation.operation.CancellationSupported && m.operation.preview != nil {
			b.WriteString("z request Core cancellation\n")
		}
	} else if m.preparingAction {
		b.WriteString("\nChecking Core availability and preview…\n")
	} else if m.pendingAction != "" {
		if m.pendingPreview != nil {
			fmt.Fprintf(b, "\nCore preview: %s %s → %s\nConfirm %s for %s? y confirm • esc cancel\n", safeTerminalText(m.pendingAction, 40), strings.Join(m.pendingPreview.Targets, ", "), strings.Join(m.pendingPreview.Effects, ", "), m.pendingAction, safeTerminalText(m.pendingServiceID, 128))
		} else {
			b.WriteString("\nCore preview is unavailable.\n")
		}
	} else {
		if m.availabilityLoading { b.WriteString("\nChecking selected service action availability…\n") } else if m.availabilityUnavailable || m.availabilityService != service.ID { b.WriteString("\nAction availability unavailable; actions recheck Core before preview.\n") } else {
			for _, action := range m.availability { label := "disabled: " + api.ReasonLabel(action.Reason); if action.Available && action.RequiresConfirmation { label = "available; confirmation required" }; fmt.Fprintf(b, "%s: %s\n", safeTerminalText(action.Action, 20), label) }
		}
		b.WriteString("\nAction keys: i install • c config • s start • x stop • R restart; disabled actions cannot submit.\n")
		b.WriteString("Each action checks Core availability and asks Core to enforce permission and confirmation.\n")
	}
	if m.pendingAction != "" {
		b.WriteString("y confirm • esc cancel\n")
	} else {
		b.WriteString("esc back • r refresh • q quit\n")
	}
	return b.String()
}


func (m model) inboxView(b *strings.Builder) string {
	b.WriteString(fmt.Sprintf("Operator inbox: %d item(s)\n", m.inbox.Total))
	for _, item := range m.inbox.Items {
		fmt.Fprintf(b, "%s %-9s %-8s %s\n", safeTerminalText(item.CreatedAt, 40), safeTerminalText(item.Severity, 40), safeTerminalText(item.State, 40), safeTerminalText(item.Title, 120))
	}
	if len(m.inbox.Items) == 0 {
		b.WriteString("No readable inbox items returned.\n")
	}
	b.WriteString("\nd dashboard • v services • / search • r reconnect • ? help • q quit\n")
	return b.String()
}
