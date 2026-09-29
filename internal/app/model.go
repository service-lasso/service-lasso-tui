package app

import (
	"context"
	"fmt"
	"strings"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/service-lasso/service-lasso-tui/internal/api"
)

type runtimeClient interface {
	Health(context.Context) (api.Health, error)
	Services(context.Context) ([]api.Service, error)
	Lifecycle(context.Context, string, string) (api.LifecycleResult, error)
}

type screen int

const (
	servicesScreen screen = iota
	detailScreen
)

type model struct {
	client           runtimeClient
	ctx              context.Context
	screen           screen
	services         []api.Service
	selected         int
	health           api.Health
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
	return model{client: client, ctx: ctx, loading: true}
}

func (m model) Init() tea.Cmd { return m.refresh() }

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
	case loadedMsg:
		m.loading = false
		m.err = message.err
		if message.err == nil {
			m.health, m.services = message.health, message.services
			if m.selected >= len(m.services) {
				m.selected = max(0, len(m.services)-1)
			}
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
		switch message.String() {
		case "ctrl+c", "q":
			return m, tea.Quit
		case "r":
			if m.pendingAction != "" {
				return m, nil
			}
			m.loading, m.err = true, nil
			return m, m.refresh()
		case "esc", "backspace":
			if m.pendingAction != "" {
				m.pendingAction = ""
			} else {
				m.screen = servicesScreen
			}
		case "y":
			if m.pendingAction != "" && !m.submittingAction {
				m.loading, m.err = true, nil
				m.submittingAction = true
				return m, m.runLifecycle()
			}
		case "i", "c", "s", "x", "R", "l":
			if m.screen == detailScreen && len(m.services) > 0 && m.pendingAction == "" {
				m.pendingAction = map[string]string{
					"i": "install", "c": "config", "s": "start", "x": "stop", "R": "restart", "l": "reload",
				}[message.String()]
			}
		case "down", "j":
			if m.screen == servicesScreen && m.selected < len(m.services)-1 {
				m.selected++
			}
		case "up", "k":
			if m.screen == servicesScreen && m.selected > 0 {
				m.selected--
			}
		case "enter":
			if m.screen == servicesScreen && len(m.services) > 0 {
				m.screen = detailScreen
			}
		}
	}
	return m, nil
}

func (m model) View() string {
	var b strings.Builder
	b.WriteString("Service Lasso TUI\n")
	if m.loading {
		b.WriteString("Connecting to runtime API…\n")
	} else if m.err != nil {
		b.WriteString("Runtime API unavailable: " + m.err.Error() + "\nPress r to retry.\n")
	} else {
		b.WriteString(fmt.Sprintf("Runtime: %s (API %s)\n", m.health.Status, m.health.API.Status))
	}
	b.WriteString("\n")
	if m.screen == detailScreen {
		return m.detailView(&b)
	}
	for index, service := range m.services {
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
	b.WriteString("\n↑/k ↓/j navigate • enter details • r refresh • q quit\n")
	return b.String()
}

func (m model) detailView(b *strings.Builder) string {
	if len(m.services) == 0 {
		b.WriteString("No service selected.\n")
	} else {
		service := m.services[m.selected]
		fmt.Fprintf(b, "%s (%s)\n\n%s\n\nLifecycle: %s\nHealth: %s\nEnabled: %t\n", service.Name, service.ID, service.Description, service.Lifecycle.State, service.Health.Status, service.Enabled)
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
