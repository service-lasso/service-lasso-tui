package app

import (
	"fmt"
	"strconv"
	"strings"

	"github.com/charmbracelet/lipgloss"
	"github.com/charmbracelet/x/ansi"
)

var (
	dashboardTitle  = lipgloss.NewStyle().Bold(true).Foreground(lipgloss.Color("87"))
	dashboardMeta   = lipgloss.NewStyle().Foreground(lipgloss.Color("245"))
	dashboardSelect = lipgloss.NewStyle().Bold(true).Foreground(lipgloss.Color("16")).Background(lipgloss.Color("87"))
	dashboardBar    = lipgloss.NewStyle().Foreground(lipgloss.Color("250")).Background(lipgloss.Color("236"))
	dashboardBorder = lipgloss.NewStyle().Foreground(lipgloss.Color("238"))
	dashboardGood   = lipgloss.NewStyle().Foreground(lipgloss.Color("114"))
	dashboardWarn   = lipgloss.NewStyle().Foreground(lipgloss.Color("214"))
	dashboardMuted  = lipgloss.NewStyle().Foreground(lipgloss.Color("244"))
)

func (m operatorModel) dashboardEnabled() bool {
	return tuiColorEnabled() && m.width >= 80 && m.height >= 20 && !m.draft.Dirty && m.draft.Preview == nil && !m.draft.Sent
}

func padLine(text string, width int) string {
	if width <= 0 {
		return ""
	}
	text = ansi.Truncate(text, width, "…")
	if w := ansi.StringWidth(text); w < width {
		text += strings.Repeat(" ", width-w)
	}
	return text
}

func (m operatorModel) dashboardHeader() string {
	status := "connecting"
	if m.discovery != nil {
		status = "connected"
	}
	if m.loading {
		status = "loading"
	}
	left := " Service Lasso  Service Admin"
	right := status
	gap := m.width - ansi.StringWidth(left) - ansi.StringWidth(right) - 2
	if gap < 1 {
		gap = 1
	}
	return dashboardTitle.Render(padLine(left+strings.Repeat(" ", gap)+right+" ", m.width))
}

func (m operatorModel) dashboardSidebar(width, height int) []string {
	items := []struct{ label, journey string }{
		{"Dashboard", "overview"}, {"Services", "overview"}, {"Dependencies", "graph"},
		{"Logs", "logs"}, {"Inbox", "inbox"}, {"Lifecycle", "lifecycle"}, {"Runtime", "detail"},
		{"Installed", "detail"}, {"Variables", "variables"}, {"Network", "endpoints"},
		{"Security", "metadata"}, {"Settings", "metadata"}, {"Help Center", "overview"},
	}
	lines := []string{dashboardMeta.Render(" SERVICE ADMIN"), ""}
	selected := -1
	for i, item := range items {
		if item.journey == m.journey {
			selected = i
			break
		}
	}
	for i, item := range items {
		marker := "  "
		if i == selected {
			marker = "▌ "
		}
		label := marker + item.label
		if i == selected {
			lines = append(lines, dashboardSelect.Render(padLine(label, width)))
		} else {
			lines = append(lines, padLine(label, width))
		}
	}
	for len(lines) < height {
		lines = append(lines, "")
	}
	return lines[:height]
}

func (m operatorModel) dashboardDetailRows(mainWidth int) []string {
	d := m.detail
	if d == nil {
		return []string{"", "  Loading service detail…"}
	}
	r := d.Summary
	head := fmt.Sprintf("%s  [%s]  running=%s  health=%s  update=%s", r.DisplayName, r.ServiceID, r.Running, r.HealthStatus, r.UpdateStatus)
	lines := []string{dashboardTitle.Render(padLine(head, mainWidth)), "", dashboardMeta.Render(" Overview"), ""}
	lines = append(lines, padLine(" Provider: "+d.Provider.Role+" · "+operatorReason(d.Provider.Availability), mainWidth), "")
	if len(d.Dependencies) > 0 {
		lines = append(lines, dashboardMeta.Render(" Dependencies"))
		for _, edge := range d.Dependencies {
			lines = append(lines, padLine(fmt.Sprintf("  %s → %s  [%s %s]", edge.FromServiceID, edge.ToServiceID, edge.Kind, edge.State), mainWidth))
		}
	} else {
		lines = append(lines, "  No dependencies reported.")
	}
	if len(d.Endpoints) > 0 {
		lines = append(lines, "", dashboardMeta.Render(" Endpoints"))
		for _, ep := range d.Endpoints {
			port := "—"
			if ep.Port != nil {
				port = fmt.Sprintf("%d", *ep.Port)
			}
			lines = append(lines, padLine(fmt.Sprintf("  %s  %s  port %s  %s", ep.Label, ep.Protocol, port, operatorReason(ep.Availability)), mainWidth))
		}
	}
	lines = append(lines, "", dashboardMeta.Render(" Actions"), "  → Lifecycle to choose a service action   r refresh   esc back")
	return lines
}

func (m operatorModel) dashboardSummary(mainWidth int) string {
	services := m.overviewServices()
	running, stopped, degraded := 0, 0, 0
	for _, service := range services {
		switch service.Running {
		case "yes":
			running++
		case "no":
			stopped++
		}
		if service.HealthStatus == "unhealthy" {
			degraded++
		}
	}
	line := fmt.Sprintf(" %d services  ·  %d running  ·  %d stopped  ·  %d degraded", len(services), running, stopped, degraded)
	if degraded > 0 {
		return dashboardWarn.Render(padLine(line, mainWidth))
	}
	if stopped > 0 {
		return dashboardMuted.Render(padLine(line, mainWidth))
	}
	return dashboardGood.Render(padLine(line, mainWidth))
}

func (m operatorModel) dashboardRows(mainWidth int) []string {
	var lines []string
	if m.journey == "detail" {
		return m.dashboardDetailRows(mainWidth)
	}
	if m.journey == "overview" {
		lines = append(lines, m.dashboardSummary(mainWidth), "")
		header := fmt.Sprintf("%-22s %-8s %-12s %-10s %-10s", "SERVICE", "RUNNING", "HEALTH", "UPDATES", "FAVORITE")
		lines = append(lines, dashboardMeta.Render(padLine(header, mainWidth)))
		for i, service := range m.overviewServices() {
			name := "@" + service.ServiceID
			row := fmt.Sprintf("%-22s %-8s %-12s %-10s %-10s", name, service.Running, service.HealthStatus, service.UpdateStatus, m.favoriteLabel(service.ServiceID, service.Generation))
			row = padLine(row, mainWidth)
			if i == m.row {
				lines = append(lines, dashboardSelect.Render(row))
			} else {
				lines = append(lines, row)
			}
		}
		if len(m.overviewServices()) == 0 {
			lines = append(lines, "", "  No services discovered for this runtime.")
		}
	} else {
		for i, row := range m.rowLines() {
			row = padLine("  "+row, mainWidth)
			if i == m.row {
				row = dashboardSelect.Render(row)
			}
			lines = append(lines, row)
		}
		if len(lines) == 0 {
			lines = append(lines, "", "  No data loaded for this view.")
		}
	}
	return lines
}

func (m operatorModel) dashboardView() string {
	sidebarWidth := 24
	if m.width < 110 {
		sidebarWidth = 20
	}
	mainWidth := m.width - sidebarWidth - 3
	if mainWidth < 20 {
		return m.dashboardHeader()
	}
	bodyHeight := m.height - 2
	if bodyHeight < 1 {
		return m.dashboardHeader()
	}
	sidebar := m.dashboardSidebar(sidebarWidth, bodyHeight)
	rows := m.dashboardRows(mainWidth)
	if len(rows) > bodyHeight-3 {
		rows = rows[:bodyHeight-3]
	}
	main := []string{dashboardTitle.Render(" " + strings.ToUpper(m.journey)), dashboardMeta.Render(" " + m.snapshotStatus()), ""}
	main = append(main, rows...)
	for len(main) < bodyHeight {
		main = append(main, "")
	}
	var out []string
	out = append(out, m.dashboardHeader())
	for i := 0; i < bodyHeight; i++ {
		left := padLine(sidebar[i], sidebarWidth)
		right := padLine(main[i], mainWidth)
		out = append(out, left+dashboardBorder.Render(" │ ")+right)
	}
	footer := " ←/→ views   ↑/↓ select   enter open   r refresh   / filter   ? help   q quit"
	out = append(out, dashboardBar.Render(padLine(footer, m.width)))
	return strings.Join(out, "\n")
}

// Keep imports used by the dashboard in one place; this also documents that
// no favicon or URL is synthesized by the renderer.
var _ = strconv.Itoa
