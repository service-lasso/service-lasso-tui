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
}

type screen int

const (
	servicesScreen screen = iota
	detailScreen
)

type model struct {
	client   runtimeClient
	ctx      context.Context
	screen   screen
	services []api.Service
	selected int
	health   api.Health
	loading  bool
	err      error
	width    int
	height   int
}

type loadedMsg struct {
	health   api.Health
	services []api.Service
	err      error
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
	case tea.KeyMsg:
		switch message.String() {
		case "ctrl+c", "q":
			return m, tea.Quit
		case "r":
			m.loading, m.err = true, nil
			return m, m.refresh()
		case "esc", "backspace":
			m.screen = servicesScreen
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
	b.WriteString("\nesc back • r refresh • q quit\n")
	return b.String()
}
