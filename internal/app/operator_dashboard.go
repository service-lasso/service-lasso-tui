package app

import (
	"fmt"
	"strconv"
	"strings"

	"github.com/charmbracelet/x/ansi"
)

var (
	dashboardTitle  = operatorBandStyle.Bold(true).Foreground(operatorTeal)
	dashboardMeta   = operatorBaseStyle.Foreground(operatorMuted)
	dashboardSelect = operatorFocusStyle
	dashboardBar    = operatorBandStyle
	dashboardBorder = operatorBaseStyle.Foreground(operatorMuted)
	dashboardGood   = operatorBaseStyle.Foreground(operatorGreen)
	dashboardWarn   = operatorBaseStyle.Foreground(operatorAmber)
	dashboardMuted  = operatorBaseStyle.Foreground(operatorMuted)
)

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
	tabs := []string{"Overview", "Dependencies", "Endpoints"}
	for i := range tabs {
		if i == m.detailTab {
			tabs[i] = "[" + tabs[i] + "]"
		}
	}
	lines := []string{dashboardTitle.Render(padLine(head, mainWidth)), "", dashboardMeta.Render(" " + strings.Join(tabs, "   ")), ""}
	switch m.detailTab {
	case 1:
		if len(d.Dependencies) == 0 {
			lines = append(lines, "  No dependencies reported.")
		}
		for _, edge := range d.Dependencies {
			lines = append(lines, padLine(fmt.Sprintf("  %s → %s  [%s %s]", edge.FromServiceID, edge.ToServiceID, edge.Kind, edge.State), mainWidth))
		}
	case 2:
		if len(d.Endpoints) == 0 {
			lines = append(lines, "  No endpoints reported.")
		}
		for _, ep := range d.Endpoints {
			port := "—"
			if ep.Port != nil {
				port = fmt.Sprintf("%d", *ep.Port)
			}
			lines = append(lines, padLine(fmt.Sprintf("  %s  %s  port %s  %s", ep.Label, ep.Protocol, port, operatorReason(ep.Availability)), mainWidth))
		}
	default:
		lines = append(lines, padLine(" Provider: "+d.Provider.Role+" · "+operatorReason(d.Provider.Availability), mainWidth), "")
		lines = append(lines, dashboardMeta.Render(" Actions"), "  → Lifecycle to choose a service action   1/2/3 tabs   r refresh   esc back")
	}
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
	total := len(services)
	filled := 0
	if total > 0 {
		filled = running * 12 / total
	}
	gauge := "[" + dashboardGood.Render(strings.Repeat("█", filled)) + dashboardMuted.Render(strings.Repeat("░", 12-filled)) + "]"
	line := fmt.Sprintf(" SERVICES %s  %d/%d running  ·  %d stopped  ·  %d degraded", gauge, running, total, stopped, degraded)
	if degraded > 0 {
		return dashboardWarn.Render(padLine(line, mainWidth))
	}
	return dashboardMuted.Render(padLine(line, mainWidth))
}

func (m operatorModel) dashboardRows(mainWidth int) []string {
	var lines []string
	if m.journey == "detail" {
		return m.dashboardDetailRows(mainWidth)
	}
	if m.journey == "overview" {
		lines = append(lines, m.dashboardSummary(mainWidth), "")
		header := fmt.Sprintf("  %-22s %-8s %-12s %-10s %-10s", "SERVICE", "RUNNING", "HEALTH", "UPDATES", "FAVORITE")
		lines = append(lines, dashboardMeta.Render(padLine(header, mainWidth)))
		for i, service := range m.overviewServices() {
			name := "@" + service.ServiceID
			dot := "○ "
			if service.Running == "yes" && service.HealthStatus == "unhealthy" {
				dot = dashboardWarn.Render("▲ ")
			} else if service.Running == "yes" {
				dot = dashboardGood.Render("● ")
			} else {
				dot = dashboardMuted.Render("○ ")
			}
			row := fmt.Sprintf("%s%-22s %-8s %-12s %-10s %-10s", dot, name, service.Running, service.HealthStatus, service.UpdateStatus, m.favoriteLabel(service.ServiceID, service.Generation))
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

func (m operatorModel) dashboardNotices() []string {
	var lines []string
	if m.loading {
		lines = append(lines, dashboardMeta.Render(" Loading…"))
	}
	if m.errorText != "" {
		lines = append(lines, dashboardWarn.Render(" Error: "+m.errorText))
	}
	if m.warning != "" {
		lines = append(lines, dashboardWarn.Render(" Warning: "+m.warning))
	}
	if m.custodyWarning != "" {
		lines = append(lines, dashboardWarn.Render(" Storage: "+m.custodyWarning))
	}
	if m.operation != nil {
		op := m.operation.Operation
		lines = append(lines, dashboardMeta.Render(fmt.Sprintf(" Operation %s  %s/%s", op.OperationID, op.Status, op.Phase)))
	}
	return lines
}

func dashboardPanel(title string, content []string, width, height int) []string {
	if height < 3 {
		height = 3
	}
	inner := width - 2
	if inner < 1 {
		inner = 1
	}
	label := dashboardTitle.Render(" " + title + " ")
	used := ansi.StringWidth(label)
	border := dashboardBorder
	line := border.Render("╭─") + label
	if pad := inner - used - 1; pad > 0 {
		line += border.Render(strings.Repeat("─", pad))
	}
	line += border.Render("╮")
	out := []string{line}
	for i := 0; i < height-2; i++ {
		text := ""
		if i < len(content) {
			text = content[i]
		}
		out = append(out, border.Render("│")+padLine(text, inner)+border.Render("│"))
	}
	out = append(out, border.Render("╰"+strings.Repeat("─", inner)+"╯"))
	return out
}

func (m operatorModel) dashboardView() string {
	sidebarWidth := 24
	if m.width < 110 {
		sidebarWidth = 20
	}
	mainWidth := m.width - sidebarWidth - 1
	if mainWidth < 25 {
		return m.dashboardHeader()
	}
	bodyHeight := m.height - 2
	if bodyHeight < 5 {
		return m.dashboardHeader()
	}

	sidebar := dashboardPanel("NAV", m.dashboardSidebar(sidebarWidth-2, bodyHeight-2), sidebarWidth, bodyHeight)
	content := []string{
		dashboardMeta.Render(" " + m.snapshotStatus()),
	}
	content = append(content, m.dashboardNotices()...)
	content = append(content, "")
	content = append(content, m.dashboardRows(mainWidth-4)...)
	main := dashboardPanel(strings.ToUpper(m.journey), content, mainWidth, bodyHeight)

	out := []string{m.dashboardHeader()}
	for i := 0; i < bodyHeight; i++ {
		out = append(out, padLine(sidebar[i], sidebarWidth)+" "+padLine(main[i], mainWidth))
	}
	footer := " ←/→ views   ↑/↓ select   enter open   r refresh   / filter   ? help   q quit"
	out = append(out, dashboardBar.Render(padLine(footer, m.width)))
	return strings.Join(out, "\n")
}

// Keep imports used by the dashboard in one place; this also documents that
// no favicon or URL is synthesized by the renderer.
var _ = strconv.Itoa
