package app

import (
	"fmt"
	"strings"

	"github.com/charmbracelet/lipgloss"
	"github.com/service-lasso/service-lasso-tui/internal/api"
)

// stackView uses only the current overview observation. Its inspector deliberately
// shares the exact filtered row index used by Enter, not a retained detail read.
func (m operatorModel) stackView() string {
	w := m.width
	lines := []string{stackBand.Render(padLine(" SL  Service Lasso  operator/v2 / STACK MONITOR", w))}
	focus := "Services"
	if m.focus == 0 {
		focus = "Views"
	}
	lines = append(lines, padLine(" Focus: "+focus+" · Tab changes focus · filter: "+m.filter(), w))
	state := "Pending: no snapshot loaded for this selection."
	current := m.pageSelectionCurrent()
	if current {
		state = m.snapshotStatus()
		if state == "" {
			state = "Pending: no snapshot loaded."
		}
	}
	if m.loading {
		state = "Loading… " + state
	}
	if m.errorText != "" {
		state = "Read failed: " + m.errorText + " · " + state
		if m.discovery == nil {
			state = "Runtime API unavailable · " + m.errorText
		}
	}
	lines = append(lines, padLine(operatorText(state), w))
	var services []api.OperatorService
	if current && m.services.Snapshot != "" {
		services = m.overviewServices()
	}
	running, stopped, unknown, healthy, unhealthy, healthUnknown := 0, 0, 0, 0, 0, 0
	for _, s := range services {
		switch s.Running {
		case "yes":
			running++
		case "no":
			stopped++
		default:
			unknown++
		}
		switch s.HealthStatus {
		case "healthy":
			healthy++
		case "unhealthy":
			unhealthy++
		default:
			healthUnknown++
		}
	}
	attention := 0
	for _, s := range services {
		if s.Running == "no" || s.HealthStatus == "unhealthy" {
			attention++
		}
	}
	runtimeText, healthText, attentionText := "Unavailable", "Unavailable", "Unavailable"
	if services != nil {
		runtimeText = fmt.Sprintf("running %d · stopped %d · unknown %d", running, stopped, unknown)
		healthText = fmt.Sprintf("healthy %d · unhealthy %d", healthy, unhealthy)
		attentionText = fmt.Sprintf("%d stopped or unhealthy", attention)
	}
	if w >= 110 {
		tileWidth := (w - 4) / 3
		lastWidth := w - 4 - 2*tileWidth
		lines = append(lines, stackRuntime.Render(padLine(" RUNTIME / OBSERVED", tileWidth))+"  "+stackHealth.Render(padLine(" HEALTH / OBSERVED", tileWidth))+"  "+stackAttention.Render(padLine(" NEEDS ATTENTION", lastWidth)))
		lines = append(lines, stackRuntime.Render(padLine(" "+runtimeText, tileWidth))+"  "+stackHealth.Render(padLine(" "+healthText, tileWidth))+"  "+stackAttention.Render(padLine(" "+attentionText, lastWidth)))
		extra := " Unverified: unavailable"
		if services != nil {
			extra = fmt.Sprintf(" unverified %d", healthUnknown)
		}
		lines = append(lines, stackRuntime.Render(padLine(" Current filtered observation", tileWidth))+"  "+stackHealth.Render(padLine(extra, tileWidth))+"  "+stackAttention.Render(padLine(" Summary states only", lastWidth)))
	} else {
		lines = append(lines, stackRuntime.Render(padLine(" Runtime · "+runtimeText, w)), stackHealth.Render(padLine(fmt.Sprintf(" Health · %s · unverified %d", healthText, healthUnknown), w)), stackAttention.Render(padLine(" NEEDS ATTENTION · "+attentionText, w)))
	}
	bodyHeight := m.height - len(lines) - 1
	if bodyHeight < 3 {
		return strings.Join(lines[:min(len(lines), m.height)], "\n")
	}
	if m.focus == 0 {
		rows := make([]string, len(operatorViews))
		for i, v := range operatorViews {
			mark := "  "
			if i == m.row {
				mark = "> "
			}
			rows[i] = mark + v
		}
		start := stackStart(m.row, len(rows), bodyHeight-2)
		lines = append(lines, stackPanel("VIEWS", rows[start:], w, bodyHeight, m.row-start)...)
	} else {
		inspector := []string{"No selected service observation."}
		if m.row >= 0 && m.row < len(services) {
			s := services[m.row]
			inspector = []string{operatorText(s.DisplayName), "ID: " + operatorText(s.ServiceID), "Runtime: " + stackRunning(s.Running) + " · Health: " + s.HealthStatus, "Installed: " + s.Installed + " · Configured: " + s.Configured, "Update: " + s.UpdateStatus, operatorText(s.Description)}
		}
		listWidth := w
		listHeight := bodyHeight
		wide := w >= 110
		if wide {
			listWidth = w * 55 / 100
		} else {
			listHeight = bodyHeight - 8
			if listHeight < 3 {
				listHeight = 3
			}
		}
		rows := []string{}
		for i, s := range services {
			mark := "  "
			if i == m.row {
				mark = "> "
			}
			row := mark + operatorText(s.ServiceID) + " · " + stackRunning(s.Running) + " · " + s.HealthStatus
			rows = append(rows, row)
		}
		if len(rows) == 0 {
			if current && m.services.Snapshot != "" {
				rows = append(rows, "No services in this observation.")
			} else {
				rows = append(rows, "Waiting for a current observation.")
			}
		}
		start := stackStart(m.row, len(rows), listHeight-2)
		list := stackPanel(fmt.Sprintf("SERVICES %d/%d", min(m.row+1, len(services)), len(services)), rows[start:], listWidth, listHeight, m.row-start)
		if wide {
			detail := stackPanel("INSPECTOR", inspector, w-listWidth-1, bodyHeight, -1)
			for i := range list {
				lines = append(lines, list[i]+" "+detail[i])
			}
		} else {
			lines = append(lines, list...)
			remaining := m.height - len(lines) - 1
			if remaining >= 3 {
				lines = append(lines, stackPanel("INSPECTOR", inspector, w, remaining, -1)...)
			}
		}
	}
	footer := " ↑↓ Select  Enter Inspect  ←→ Views  Tab Focus  r Refresh  f Filter  q Quit"
	lines = append(lines, dashboardBar.Render(padLine(footer, w)))
	return strings.Join(lines, "\n")
}

func stackStart(row, count, visible int) int {
	if visible < 1 {
		return 0
	}
	start := row - visible + 1
	if start < 0 {
		start = 0
	}
	if maxStart := count - visible; maxStart >= 0 && start > maxStart {
		start = maxStart
	}
	if start >= count {
		return 0
	}
	return start
}

var (
	stackBand      = lipgloss.NewStyle().Bold(true).Foreground(lipgloss.Color("159")).Background(lipgloss.Color("24"))
	stackRuntime   = lipgloss.NewStyle().Foreground(lipgloss.Color("87")).Background(lipgloss.Color("236"))
	stackHealth    = lipgloss.NewStyle().Foreground(lipgloss.Color("150")).Background(lipgloss.Color("236"))
	stackAttention = lipgloss.NewStyle().Foreground(lipgloss.Color("217")).Background(lipgloss.Color("236"))
	stackSurface   = lipgloss.NewStyle().Foreground(lipgloss.Color("252")).Background(lipgloss.Color("235"))
	stackSelected  = lipgloss.NewStyle().Bold(true).Foreground(lipgloss.Color("87")).Background(lipgloss.Color("23"))
)

// Filled surfaces avoid vertical rule clutter while keeping monochrome headings.
func stackPanel(title string, content []string, width, height, selected int) []string {
	out := []string{stackBand.Render(padLine(" "+title, width))}
	for i := 0; i < height-1; i++ {
		text := ""
		if i < len(content) {
			text = content[i]
		}
		style := stackSurface
		if i == selected {
			style = stackSelected
		}
		out = append(out, style.Render(padLine(text, width)))
	}
	return out
}
func stackRunning(value string) string {
	switch value {
	case "yes":
		return "Running"
	case "no":
		return "Stopped"
	default:
		return "Unknown"
	}
}
