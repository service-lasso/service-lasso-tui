package app

import (
	"context"
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
	Lifecycle(context.Context, string, string) (api.LifecycleResult, error)
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
	nextSubmissionID   uint64
	outstandingAction  *lifecycleSubmission
	operation          *operationSubmission
	operationStore     operationStore
	lastResult         string
	width              int
	height             int
}

type loadedMsg struct {
	epoch    uint64
	health   api.Health
	services []api.Service
	err      error
}

type lifecycleMsg struct {
	submissionID uint64
	result       api.LifecycleResult
	err          error
}

type lifecyclePreparedMsg struct {
	epoch             uint64
	action, serviceID string
	preview           api.LifecyclePreview
	err               error
}
type operationMsg struct {
	submissionID uint64
	operation    api.Operation
	err          error
}
type operationPollMsg struct{ submissionID uint64 }

// lifecycleSubmission is deliberately separate from the confirmation and the
// current screen. Once Core has received the request, its client, profile,
// target, and action must remain stable until its one result is handled.
type lifecycleSubmission struct {
	id                uint64
	client            runtimeClient
	connectionName    string
	connectionEpoch   uint64
	serviceID, action string
}

type operationSubmission struct {
	id              uint64
	client          durableLifecycleClient
	connectionName  string
	connectionEpoch uint64
	operation       api.Operation
}

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
	stored *persistedOperation
	err    error
}

func (m model) readStoredOperation() tea.Cmd {
	store := m.operationStore
	return func() tea.Msg {
		if store == nil {
			return storedOperationMsg{}
		}
		stored, err := store.Load()
		return storedOperationMsg{stored: stored, err: err}
	}
}

func (m model) refresh() tea.Cmd {
	epoch, client := m.connectionEpoch, m.client
	return func() tea.Msg {
		health, err := client.Health(m.ctx)
		if err != nil {
			return loadedMsg{epoch: epoch, err: err}
		}
		services, err := client.Services(m.ctx)
		return loadedMsg{epoch: epoch, health: health, services: services, err: err}
	}
}

type dashboardMsg struct {
	epoch        uint64
	capabilities api.Capabilities
	setup        api.SetupStatus
	identity     api.RuntimeIdentity
	inbox        api.Inbox
	failures     []string
}
type historyMsg struct {
	epoch   uint64
	history api.HealthHistory
	err     error
}

func (m model) refreshDashboard() tea.Cmd {
	epoch, client := m.connectionEpoch, m.client
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
		var failuresMu sync.Mutex
		addFailure := func(name string, err error) {
			if err != nil {
				failuresMu.Lock()
				failures = append(failures, name)
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
		return dashboardMsg{epoch: epoch, capabilities: capabilities, setup: setup, identity: identity, inbox: inbox, failures: failures}
	}
}

func (m model) loadHistory() tea.Cmd {
	service, ok := m.selectedService()
	if !ok {
		return nil
	}
	id, epoch, client := service.ID, m.connectionEpoch, m.client
	return func() tea.Msg {
		history, err := client.HealthHistory(m.ctx, id)
		return historyMsg{epoch: epoch, history: history, err: err}
	}
}

func runLifecycle(submission lifecycleSubmission, ctx context.Context) tea.Cmd {
	return func() tea.Msg {
		result, err := submission.client.Lifecycle(ctx, submission.serviceID, submission.action)
		return lifecycleMsg{submissionID: submission.id, result: result, err: err}
	}
}

func prepareLifecycle(client durableLifecycleClient, ctx context.Context, epoch uint64, serviceID, action string) tea.Cmd {
	return func() tea.Msg {
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
				err = fmt.Errorf("Core does not currently allow %s for this service", action)
			}
		}
		preview := api.LifecyclePreview{}
		if err == nil {
			preview, err = client.PreviewLifecycle(ctx, serviceID, action)
		}
		return lifecyclePreparedMsg{epoch: epoch, action: action, serviceID: serviceID, preview: preview, err: err}
	}
}

func submitDurableLifecycle(submission operationSubmission, preview api.LifecyclePreview, ctx context.Context) tea.Cmd {
	return func() tea.Msg {
		key, err := api.NewIdempotencyKey()
		if err != nil {
			return operationMsg{submissionID: submission.id, err: err}
		}
		operation, err := submission.client.SubmitLifecycle(ctx, preview, key)
		return operationMsg{submissionID: submission.id, operation: operation, err: err}
	}
}

func readOperation(submission operationSubmission, ctx context.Context) tea.Cmd {
	return func() tea.Msg {
		operation, err := submission.client.Operation(ctx, submission.operation.ID)
		return operationMsg{submissionID: submission.id, operation: operation, err: err}
	}
}

func cancelOperation(submission operationSubmission, ctx context.Context) tea.Cmd {
	return func() tea.Msg {
		operation, err := submission.client.CancelOperation(ctx, submission.operation.ID)
		return operationMsg{submissionID: submission.id, operation: operation, err: err}
	}
}

func (m model) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch message := msg.(type) {
	case tea.WindowSizeMsg:
		m.width, m.height = message.Width, message.Height
		m.narrow = message.Width > 0 && message.Width < 72
	case loadedMsg:
		if message.epoch != m.connectionEpoch {
			return m, nil
		}
		m.loading = false
		m.err = message.err
		if message.err == nil {
			m.health, m.services = message.health, message.services
			m.stale = false
			m.ensureSelectedService()
			if m.pendingAction != "" && !m.hasServiceID(m.pendingServiceID) {
				m.pendingAction, m.pendingServiceID, m.pendingPreview, m.preparingAction = "", "", nil, false
				m.lastResult = "Selected service changed; confirmation cancelled."
			}
		} else if len(m.services) > 0 {
			m.stale = true
		}
	case dashboardMsg:
		if message.epoch != m.connectionEpoch {
			return m, nil
		}
		m.capabilities, m.setup, m.identity, m.inbox, m.optionalFailures = message.capabilities, message.setup, message.identity, message.inbox, message.failures
	case historyMsg:
		if message.epoch != m.connectionEpoch {
			return m, nil
		}
		if message.err == nil {
			m.history = message.history
			m.historyUnavailable = false
		} else {
			m.historyUnavailable = true
		}
	case lifecycleMsg:
		if m.outstandingAction == nil || message.submissionID != m.outstandingAction.id {
			return m, nil
		}
		m.loading = false
		submission := *m.outstandingAction
		m.outstandingAction = nil
		if message.err != nil {
			m.err = message.err
			m.lastResult = ""
		} else {
			m.err = nil
			outcome := "failed"
			if message.result.OK {
				outcome = "completed"
			}
			m.lastResult = fmt.Sprintf("Core %s %s.", outcome, submission.action)
		}
	case lifecyclePreparedMsg:
		if message.epoch != m.connectionEpoch || message.serviceID != m.pendingServiceID || message.action != m.pendingAction {
			return m, nil
		}
		m.preparingAction = false
		if message.err != nil {
			m.pendingAction, m.pendingServiceID, m.pendingPreview = "", "", nil
			m.err = message.err
			m.lastResult = ""
			return m, nil
		}
		m.pendingPreview = &message.preview
	case storedOperationMsg:
		if message.err != nil {
			m.err = message.err
			return m, nil
		}
		if message.stored == nil || message.stored.ConnectionName != m.connectionName {
			return m, nil
		}
		binding, matches := m.connectionBinding()
		if !matches || binding != message.stored.Binding {
			m.lastResult = "Retained operation belongs to a different actor or connection; it was not read or replayed."
			return m, nil
		}
		durable, ok := m.client.(durableLifecycleClient)
		if !ok {
			return m, nil
		}
		m.nextSubmissionID++
		m.operation = &operationSubmission{id: m.nextSubmissionID, client: durable, connectionName: m.connectionName, connectionEpoch: m.connectionEpoch, operation: api.Operation{ID: message.stored.OperationID, Status: "reconciling", Phase: "reconciling"}}
		return m, readOperation(*m.operation, m.ctx)
	case operationMsg:
		if m.operation == nil || message.submissionID != m.operation.id {
			return m, nil
		}
		if message.err != nil {
			m.err = message.err
			m.lastResult = "Operation outcome is uncertain; reconnect or refresh only reads the retained operation."
			return m, nil
		}
		m.operation.operation = message.operation
		m.err = nil
		if m.operationStore != nil {
			if binding, ok := m.connectionBinding(); ok {
				if err := m.operationStore.Save(persistedOperation{Version: 1, OperationID: message.operation.ID, ConnectionName: m.operation.connectionName, Binding: binding}); err != nil {
					m.err = err
				}
			}
		}
		if message.operation.Outcome != "" || message.operation.Status == "unknown_after_crash" {
			m.lastResult = fmt.Sprintf("Core operation %s: %s.", safeTerminalText(message.operation.ID, 80), safeTerminalText(firstNonEmpty(message.operation.Outcome, message.operation.Status), 48))
			m.operation = nil
			if m.operationStore != nil {
				_ = m.operationStore.Clear()
			}
			return m, nil
		}
		return m, tea.Tick(250*time.Millisecond, func(time.Time) tea.Msg { return operationPollMsg{submissionID: message.submissionID} })
	case operationPollMsg:
		if m.operation == nil || message.submissionID != m.operation.id || m.operation.connectionEpoch != m.connectionEpoch {
			return m, nil
		}
		return m, readOperation(*m.operation, m.ctx)
	case tea.KeyMsg:
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
					m.search = m.search[:len(m.search)-1]
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
			m.loading, m.err = true, nil
			return m, tea.Batch(m.refresh(), m.refreshDashboard())
		case "p":
			if m.connections != nil && m.pendingAction == "" && !m.hasOutstandingAction() {
				m.selectedProfile = m.connectionName
				m.screen = profilesScreen
			}
		case "d":
			m.cancelPendingOnNavigation()
			m.screen = dashboardScreen
		case "v":
			m.cancelPendingOnNavigation()
			m.screen = servicesScreen
		case "i":
			if m.screen == detailScreen && m.hasSelectedService() && m.pendingAction == "" {
				return m, m.beginPendingAction("install")
			} else {
				m.cancelPendingOnNavigation()
				m.screen = inboxScreen
			}
		case "?":
			m.cancelPendingOnNavigation()
			m.screen = helpScreen
		case "/":
			m.searching = true
		case "n":
			m.narrow = !m.narrow
		case "esc", "backspace":
			if m.pendingAction != "" {
				m.pendingAction, m.pendingServiceID, m.pendingPreview, m.preparingAction = "", "", nil, false
			} else {
				m.screen = dashboardScreen
			}
		case "y":
			if m.screen == detailScreen && m.pendingAction != "" && (!m.preparingAction) && !m.hasOutstandingAction() && m.operation == nil && m.hasServiceID(m.pendingServiceID) {
				return m, m.submitPendingAction()
			}
			if m.pendingAction != "" && !m.hasServiceID(m.pendingServiceID) {
				m.pendingAction, m.pendingServiceID = "", ""
				m.lastResult = "Selected service changed; confirmation cancelled."
			}
		case "c", "s", "x", "R", "l":
			if m.screen == detailScreen && m.hasSelectedService() && m.pendingAction == "" && !m.hasOutstandingAction() && m.operation == nil {
				return m, m.beginPendingAction(map[string]string{
					"c": "config", "s": "start", "x": "stop", "R": "restart", "l": "reload",
				}[message.String()])
			}
		case "z":
			if m.operation != nil && m.operation.operation.CancellationSupported {
				return m, cancelOperation(*m.operation, m.ctx)
			}
		case "down", "j":
			m.moveSelected(1)
		case "up", "k":
			m.moveSelected(-1)
		case "enter":
			if m.screen == profilesScreen {
				if m.hasOutstandingAction() {
					return m, nil
				}
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
				return m, m.loadHistory()
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
	if m.hasOutstandingAction() || m.pendingAction != "" {
		return nil
	}
	service, ok := m.selectedService()
	if !ok {
		return nil
	}
	durable, ok := m.client.(durableLifecycleClient)
	if !ok {
		// Compatibility is restricted to in-process read/test adapters. The
		// concrete production API client implements durableLifecycleClient and
		// therefore cannot reach the retired synchronous route.
		m.pendingAction, m.pendingServiceID, m.pendingPreview, m.preparingAction = action, service.ID, nil, false
		return nil
	}
	m.pendingAction, m.pendingServiceID, m.pendingPreview, m.preparingAction = action, service.ID, nil, true
	return prepareLifecycle(durable, m.ctx, m.connectionEpoch, service.ID, action)
}

func (m *model) hasOutstandingAction() bool { return m.outstandingAction != nil }

func (m *model) submitPendingAction() tea.Cmd {
	if m.pendingPreview != nil {
		durable, ok := m.client.(durableLifecycleClient)
		if !ok {
			m.err = fmt.Errorf("durable lifecycle operations are unavailable for this runtime client")
			return nil
		}
		m.nextSubmissionID++
		submission := operationSubmission{id: m.nextSubmissionID, client: durable, connectionName: m.connectionName, connectionEpoch: m.connectionEpoch}
		preview := *m.pendingPreview
		m.pendingAction, m.pendingServiceID, m.pendingPreview, m.preparingAction = "", "", nil, false
		m.operation = &submission
		m.loading, m.err = false, nil
		return submitDurableLifecycle(submission, preview, m.ctx)
	}
	m.nextSubmissionID++
	submission := lifecycleSubmission{
		id:              m.nextSubmissionID,
		client:          m.client,
		connectionName:  m.connectionName,
		connectionEpoch: m.connectionEpoch,
		serviceID:       m.pendingServiceID,
		action:          m.pendingAction,
	}
	// Confirmation is local, transient UI state. The submitted request retains
	// its own immutable context so navigation and search cannot retarget it.
	m.pendingAction, m.pendingServiceID = "", ""
	m.outstandingAction = &submission
	m.loading, m.err = true, nil
	return runLifecycle(submission, m.ctx)
}

func (m *model) cancelPendingOnNavigation() {
	if m.screen == detailScreen && m.pendingAction != "" {
		m.pendingAction, m.pendingServiceID, m.pendingPreview, m.preparingAction = "", "", nil, false
		m.lastResult = "Confirmation cancelled after navigation."
	}
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
	if m.hasOutstandingAction() {
		return
	}
	m.client = client
	m.connectionName = m.connections.Current()
	m.connectionEpoch++
	m.services = nil
	m.selectedServiceID = ""
	m.health, m.capabilities, m.setup, m.identity, m.inbox, m.history = api.Health{}, api.Capabilities{}, api.SetupStatus{}, api.RuntimeIdentity{}, api.Inbox{}, api.HealthHistory{}
	m.optionalFailures, m.pendingAction, m.pendingServiceID, m.lastResult = nil, "", "", ""
	m.pendingPreview, m.preparingAction, m.operation = nil, false, nil
	m.historyUnavailable, m.stale, m.loading = false, false, true
	m.err = nil
	m.screen = dashboardScreen
}

func (m model) connectionBinding() (string, bool) {
	client, ok := m.client.(*api.Client)
	if !ok {
		return "", false
	}
	return client.ConnectionBinding(), true
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
		b.WriteString("Runtime API unavailable: " + m.err.Error() + "\nPress r to retry.\n")
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
	}
	if m.hasOutstandingAction() {
		b.WriteString("\nSubmitting one confirmed request to Core…\n")
	} else if m.operation != nil {
		fmt.Fprintf(b, "\nCore operation %s: %s (%d%%)\n", safeTerminalText(m.operation.operation.ID, 80), safeTerminalText(m.operation.operation.Phase, 48), m.operation.operation.Progress)
		if m.operation.operation.CancellationSupported {
			b.WriteString("z request Core cancellation\n")
		}
	} else if m.preparingAction {
		b.WriteString("\nChecking Core availability and preview…\n")
	} else if m.pendingAction != "" {
		if m.pendingPreview != nil {
			fmt.Fprintf(b, "\nCore preview: %s %s → %s\nConfirm %s for %s? y confirm • esc cancel\n", safeTerminalText(m.pendingAction, 40), strings.Join(m.pendingPreview.Targets, ", "), strings.Join(m.pendingPreview.Effects, ", "), m.pendingAction, safeTerminalText(m.pendingServiceID, 128))
		} else if _, durable := m.client.(durableLifecycleClient); !durable {
			fmt.Fprintf(b, "\nConfirm %s for %s? y confirm • esc cancel\n", m.pendingAction, safeTerminalText(m.pendingServiceID, 128))
		} else {
			b.WriteString("\nCore preview is unavailable.\n")
		}
	} else {
		b.WriteString("\ni install • c config • s start • x stop • R restart • l reload (Core currently denies reload)\n")
		b.WriteString("Each action checks Core availability and asks Core to enforce permission and confirmation.\n")
	}
	b.WriteString("esc back • r refresh • q quit\n")
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
