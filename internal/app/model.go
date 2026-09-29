package app

import (
	"context"
	"fmt"
	"strings"
	"sync"
	"time"

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
	client           runtimeClient
	ctx              context.Context
	screen           screen
	services         []api.Service
	selected         int
	health           api.Health
	capabilities     api.Capabilities
	setup            api.SetupStatus
	identity         api.RuntimeIdentity
	inbox            api.Inbox
	history          api.HealthHistory
	stale            bool
	searching        bool
	search           string
	narrow           bool
	loading          bool
	err              error
	pendingAction    string
	submittingAction bool
	lastResult       string
	width            int
	height           int
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
		var wait sync.WaitGroup
		wait.Add(4)
		go func() { defer wait.Done(); capabilities, _ = m.client.Capabilities(ctx) }()
		go func() { defer wait.Done(); setup, _ = m.client.SetupStatus(ctx) }()
		go func() { defer wait.Done(); identity, _ = m.client.RuntimeIdentity(ctx) }()
		go func() { defer wait.Done(); inbox, _ = m.client.Inbox(ctx, "") }()
		wait.Wait()
		return dashboardMsg{capabilities: capabilities, setup: setup, identity: identity, inbox: inbox}
	}
}

func (m model) loadHistory() tea.Cmd {
	if len(m.services) == 0 {
		return nil
	}
	id := m.services[m.selected].ID
	return func() tea.Msg {
		history, err := m.client.HealthHistory(m.ctx, id)
		return historyMsg{history: history, err: err}
	}
}

func (m model) runLifecycle() tea.Cmd {
	service := m.services[m.selected]
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
		m.narrow = m.narrow || message.Width > 0 && message.Width < 72
	case loadedMsg:
		m.loading = false
		m.err = message.err
		if message.err == nil {
			m.health, m.services = message.health, message.services
			m.stale = false
			if m.selected >= len(m.services) {
				m.selected = max(0, len(m.services)-1)
			}
		} else if len(m.services) > 0 {
			m.stale = true
		}
	case dashboardMsg:
		m.capabilities, m.setup, m.identity, m.inbox = message.capabilities, message.setup, message.identity, message.inbox
	case historyMsg:
		if message.err == nil {
			m.history = message.history
		}
	case lifecycleMsg:
		m.loading = false
		m.pendingAction = ""
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
				m.search += string(message.Runes)
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
			m.screen = dashboardScreen
		case "v":
			m.screen = servicesScreen
		case "i":
			if m.screen == detailScreen && len(m.services) > 0 && m.pendingAction == "" {
				m.pendingAction = "install"
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
			if m.pendingAction != "" {
				m.pendingAction = ""
			} else {
				m.screen = dashboardScreen
			}
		case "y":
			if m.pendingAction != "" && !m.submittingAction {
				m.loading, m.err = true, nil
				m.submittingAction = true
				return m, m.runLifecycle()
			}
		case "c", "s", "x", "R", "l":
			if m.screen == detailScreen && len(m.services) > 0 && m.pendingAction == "" {
				m.pendingAction = map[string]string{
					"c": "config", "s": "start", "x": "stop", "R": "restart", "l": "reload",
				}[message.String()]
			}
		case "down", "j":
			if (m.screen == servicesScreen || m.screen == dashboardScreen) && m.selected < len(m.filteredServices())-1 {
				m.selected++
			}
		case "up", "k":
			if (m.screen == servicesScreen || m.screen == dashboardScreen) && m.selected > 0 {
				m.selected--
			}
		case "enter":
			if (m.screen == servicesScreen || m.screen == dashboardScreen) && len(m.services) > 0 {
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
		if strings.Contains(strings.ToLower(service.Name+" "+service.ID), strings.ToLower(m.search)) {
			filtered = append(filtered, service)
		}
	}
	return filtered
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
		b.WriteString(fmt.Sprintf("Runtime: %s (API %s)\n", m.health.Status, m.health.API.Status))
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
		fmt.Fprintf(&b, "Runtime identity: %s (%s)\nCapabilities: %s\nSetup: %s\nInbox: %d item(s)\n\n", m.identity.Status, m.identity.Phase, m.capabilities.ContractVersion, m.setup.State, m.inbox.Total)
		b.WriteString("Services needing attention\n")
	}
	for index, service := range m.filteredServices() {
		cursor := " "
		if index == m.selected {
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
		fmt.Fprintf(&b, "%s %-24s %-12s %-12s %s\n", cursor, service.Name, state, health, service.ID)
	}
	if len(m.services) == 0 && !m.loading && m.err == nil {
		b.WriteString("No services returned by the runtime.\n")
	}
	if m.searching {
		b.WriteString("\nSearch: " + m.search + "\n")
	}
	b.WriteString("\n↑/k ↓/j navigate • enter details • d dashboard • v services • i inbox • / search • ? help • r reconnect • q quit\n")
	return b.String()
}

func (m model) detailView(b *strings.Builder) string {
	if len(m.services) == 0 {
		b.WriteString("No service selected.\n")
	} else {
		service := m.services[m.selected]
		fmt.Fprintf(b, "%s (%s)\n\n%s\n\nLifecycle: %s\nHealth: %s\nEnabled: %t\nHistory transitions: %d\n", service.Name, service.ID, service.Description, service.Lifecycle.State, service.Health.Status, service.Enabled, m.history.Entries)
	}
	if m.submittingAction {
		b.WriteString("\nSubmitting one confirmed request to Core…\n")
	} else if m.pendingAction != "" {
		fmt.Fprintf(b, "\nConfirm %s for the selected service? y confirm • esc cancel\n", m.pendingAction)
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
		fmt.Fprintf(b, "%s %-9s %-8s %s\n", item.CreatedAt, item.Severity, item.State, item.Title)
	}
	if len(m.inbox.Items) == 0 {
		b.WriteString("No readable inbox items returned.\n")
	}
	b.WriteString("\nd dashboard • v services • / search • r reconnect • ? help • q quit\n")
	return b.String()
}
