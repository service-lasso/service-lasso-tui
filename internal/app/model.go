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

type screen int

const (
	dashboardScreen screen = iota
	servicesScreen
	detailScreen
	inboxScreen
	helpScreen
)

type model struct {
	client             runtimeClient
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
	submittingAction   bool
	lastResult         string
	width              int
	height             int
}

type loadedMsg struct {
	health   api.Health
	services []api.Service
	err      error
}

type lifecycleMsg struct {
	result api.LifecycleResult
	action string
	err    error
}

func New(client runtimeClient, ctx context.Context) tea.Model {
	return model{client: client, ctx: ctx, loading: true, screen: dashboardScreen}
}

func (m model) Init() tea.Cmd { return tea.Batch(m.refresh(), m.refreshDashboard()) }

func (m model) refresh() tea.Cmd {
	return func() tea.Msg {
		health, err := m.client.Health(m.ctx)
		if err != nil {
			return loadedMsg{err: err}
		}
		services, err := m.client.Services(m.ctx)
		return loadedMsg{health: health, services: services, err: err}
	}
}

type dashboardMsg struct {
	capabilities api.Capabilities
	setup        api.SetupStatus
	identity     api.RuntimeIdentity
	inbox        api.Inbox
	failures     []string
}
type historyMsg struct {
	history api.HealthHistory
	err     error
}

func (m model) refreshDashboard() tea.Cmd {
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
			capabilities, err = m.client.Capabilities(ctx)
			addFailure("capabilities", err)
		}()
		go func() {
			defer wait.Done()
			var err error
			setup, err = m.client.SetupStatus(ctx)
			addFailure("setup status", err)
		}()
		go func() {
			defer wait.Done()
			var err error
			identity, err = m.client.RuntimeIdentity(ctx)
			addFailure("runtime identity", err)
		}()
		go func() {
			defer wait.Done()
			var err error
			inbox, err = m.client.Inbox(ctx, "")
			addFailure("operator inbox", err)
		}()
		wait.Wait()
		return dashboardMsg{capabilities: capabilities, setup: setup, identity: identity, inbox: inbox, failures: failures}
	}
}

func (m model) loadHistory() tea.Cmd {
	service, ok := m.selectedService()
	if !ok {
		return nil
	}
	id := service.ID
	return func() tea.Msg {
		history, err := m.client.HealthHistory(m.ctx, id)
		return historyMsg{history: history, err: err}
	}
}

func (m model) runLifecycle() tea.Cmd {
	service, ok := m.serviceByID(m.pendingServiceID)
	if !ok {
		return nil
	}
	action := m.pendingAction
	return func() tea.Msg {
		result, err := m.client.Lifecycle(m.ctx, service.ID, action)
		return lifecycleMsg{result: result, action: action, err: err}
	}
}

func (m model) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch message := msg.(type) {
	case tea.WindowSizeMsg:
		m.width, m.height = message.Width, message.Height
		m.narrow = message.Width > 0 && message.Width < 72
	case loadedMsg:
		m.loading = false
		m.err = message.err
		if message.err == nil {
			m.health, m.services = message.health, message.services
			m.stale = false
			m.ensureSelectedService()
			if m.pendingAction != "" && !m.hasServiceID(m.pendingServiceID) {
				m.pendingAction, m.pendingServiceID = "", ""
				m.lastResult = "Selected service changed; confirmation cancelled."
			}
		} else if len(m.services) > 0 {
			m.stale = true
		}
	case dashboardMsg:
		m.capabilities, m.setup, m.identity, m.inbox, m.optionalFailures = message.capabilities, message.setup, message.identity, message.inbox, message.failures
	case historyMsg:
		if message.err == nil {
			m.history = message.history
			m.historyUnavailable = false
		} else {
			m.historyUnavailable = true
		}
	case lifecycleMsg:
		m.loading = false
		m.pendingAction = ""
		m.pendingServiceID = ""
		m.submittingAction = false
		if message.err != nil {
			m.err = message.err
			m.lastResult = ""
		} else {
			m.err = nil
			outcome := "failed"
			if message.result.OK {
				outcome = "completed"
			}
			m.lastResult = fmt.Sprintf("Core %s %s.", outcome, message.action)
		}
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
			if m.pendingAction != "" {
				return m, nil
			}
			m.loading, m.err = true, nil
			return m, tea.Batch(m.refresh(), m.refreshDashboard())
		case "d":
			m.cancelPendingOnNavigation()
			m.screen = dashboardScreen
		case "v":
			m.cancelPendingOnNavigation()
			m.screen = servicesScreen
		case "i":
			if m.screen == detailScreen && m.hasSelectedService() && m.pendingAction == "" {
				m.beginPendingAction("install")
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
				m.pendingAction, m.pendingServiceID = "", ""
			} else {
				m.screen = dashboardScreen
			}
		case "y":
			if m.screen == detailScreen && m.pendingAction != "" && !m.submittingAction && m.hasServiceID(m.pendingServiceID) {
				m.loading, m.err = true, nil
				m.submittingAction = true
				return m, m.runLifecycle()
			}
			if m.pendingAction != "" && !m.hasServiceID(m.pendingServiceID) {
				m.pendingAction, m.pendingServiceID = "", ""
				m.lastResult = "Selected service changed; confirmation cancelled."
			}
		case "c", "s", "x", "R", "l":
			if m.screen == detailScreen && m.hasSelectedService() && m.pendingAction == "" {
				m.beginPendingAction(map[string]string{
					"c": "config", "s": "start", "x": "stop", "R": "restart", "l": "reload",
				}[message.String()])
			}
		case "down", "j":
			m.moveSelected(1)
		case "up", "k":
			m.moveSelected(-1)
		case "enter":
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

func (m *model) beginPendingAction(action string) {
	service, ok := m.selectedService()
	if !ok {
		return
	}
	m.pendingAction, m.pendingServiceID = action, service.ID
}

func (m *model) cancelPendingOnNavigation() {
	if m.screen == detailScreen && m.pendingAction != "" {
		m.pendingAction, m.pendingServiceID = "", ""
		m.lastResult = "Confirmation cancelled after navigation."
	}
}

func (m model) hasSelectedService() bool { _, ok := m.selectedService(); return ok }

func (m *model) moveSelected(direction int) {
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
	if m.screen == helpScreen {
		b.WriteString("d dashboard • v services • i inbox • / search • n narrow • r reconnect • esc back • q quit\n")
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
	b.WriteString("\n↑/k ↓/j navigate • enter details • d dashboard • v services • i inbox • / search • ? help • r reconnect • q quit\n")
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
	if m.submittingAction {
		b.WriteString("\nSubmitting one confirmed request to Core…\n")
	} else if m.pendingAction != "" {
		fmt.Fprintf(b, "\nConfirm %s for %s? y confirm • esc cancel\n", m.pendingAction, safeTerminalText(m.pendingServiceID, 128))
	} else {
		b.WriteString("\ni install • c config • s start • x stop • R restart • l reload\n")
		b.WriteString("Each action asks Core to enforce permission and confirmation.\n")
	}
	if m.lastResult != "" {
		b.WriteString("Last runtime result: " + m.lastResult + "\n")
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
