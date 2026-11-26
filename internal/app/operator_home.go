package app

import (
	"fmt"
	"strings"
	"time"

	"github.com/charmbracelet/lipgloss"
	"github.com/service-lasso/service-lasso-tui/internal/api"
)

// operatorStackView is strictly a presentation of the admitted overview page.
// All selection consumers use overviewServices in its original order.
func (m operatorModel) operatorStackView() string {
	w := m.width
	gutter := 2
	if w < 100 {
		gutter = 1
	}
	inner := w - 2*gutter
	if inner < 1 || m.height < 1 {
		return ""
	}
	current := m.pageSelectionCurrent() && m.services.Snapshot != ""
	var services []api.OperatorService
	if current {
		services = m.overviewServices()
	}
	running, stopped, unknown, healthy, unverified, attention := 0, 0, 0, 0, 0, 0
	attentionRows := []string{}
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
		default:
			unverified++
		}
		if s.Running == "no" || s.HealthStatus == "unhealthy" {
			attention++
			attentionRows = append(attentionRows, operatorText(s.ServiceID)+"  ·  "+stackObservedStatus(s)+"  ·  observed summary")
		}
	}
	state := "Pending: no snapshot loaded for this selection."
	if current {
		state = "complete observation"
		if len(services) == 0 {
			state = "complete; no rows"
		}
		if !m.services.Complete {
			state = "incomplete observation; n next"
		}
		if m.services.Truncated {
			state = "truncated; omitted rows remain unknown"
		}
		if m.services.Stale || operatorSnapshotExpired(m.services.Expiry, time.Now()) {
			state = "stale; retained rows need refresh"
		}
	}
	if m.loading {
		state = "Loading… " + state
	}
	if m.errorText != "" {
		state = "Read failed · " + state
		attentionRows = append([]string{"Error: " + operatorText(m.errorText)}, attentionRows...)
		if m.discovery == nil {
			state = "Runtime API unavailable"
		}
	}
	focus := "Services"
	if m.focus == 0 {
		focus = "Views"
	}
	lines := []string{m.operatorConnectionHeader(inner), operatorRender(operatorBaseStyle.Foreground(operatorMuted), padLine(" "+m.overviewReadAge(time.Now())+" · "+m.overviewMonitorMode()+" · "+operatorText(state), inner)), m.operatorTabs(inner)}
	if m.height >= 26 && w >= 110 {
		lines = append(lines, "")
	}
	values := []string{"Unavailable", "Unavailable", "None known", "Unavailable"}
	if current {
		values[0] = fmt.Sprintf("healthy %d / %d", healthy, len(services))
		values[1] = fmt.Sprintf("%d stopped or unhealthy", attention)
	}
	if m.operation != nil {
		values[2] = operatorText(m.operation.Operation.Status)
	}
	labels := []string{"HEALTH / OBSERVED", "NEEDS ATTENTION", "KNOWN OPERATION", "CORE UPTIME"}
	if w < 110 {
		labels = []string{"HEALTH", "ATTENTION", "OPERATION", "CORE UPTIME"}
		if current {
			values[0] = fmt.Sprintf("healthy %d / %d", healthy, len(services))
			values[1] = fmt.Sprintf("%d services", attention)
		}
	}
	if w < 80 {
		labels = []string{"HEALTH", "ATTENTION", "OPERATION", "UPTIME"}
		if current {
			values[0] = fmt.Sprintf("%d / %d", healthy, len(services))
			values[1] = fmt.Sprintf("%d services", attention)
		}
	}
	colors := []lipgloss.Color{operatorGreen, operatorCoral, operatorTeal, operatorMuted}
	for line := 0; line < 2; line++ {
		row := ""
		for i, label := range labels {
			tw := (inner - 3) / 4
			if i == 3 {
				tw = inner - 3 - 3*tw
			}
			text := label
			if line == 1 {
				text = values[i]
			}
			if i > 0 {
				row += " "
			}
			style := operatorBaseStyle.Foreground(operatorMuted)
			prefix := "▌ "
			if line == 1 {
				style = style.Foreground(colors[i]).Bold(true)
				prefix = "  "
			}
			row += operatorRender(style, padLine(prefix+text, tw))
		}
		lines = append(lines, row)
	}
	if w >= 110 && m.height >= 24 {
		lines = append(lines, "")
	}
	// Reserve a separate attention panel, footer and gaps before sizing the list.
	feedHeight := min(6, max(4, len(attentionRows)+2))
	if m.height < 24 || w < 110 {
		feedHeight = 3
	}
	bodyHeight := m.height - len(lines) - feedHeight - 3
	// Tall canvases should not stretch a small observation into empty panels.
	if w >= 110 {
		bodyHeight = min(bodyHeight, max(10, len(services)+3))
	}
	if bodyHeight < 3 {
		bodyHeight = 3
	}
	wide := w >= 110
	leftWidth := (inner - 2) * 57 / 100
	rightWidth := inner - leftWidth - 2
	listHeight, inspectorHeight := bodyHeight, bodyHeight
	if !wide {
		leftWidth, rightWidth = inner, inner
		inspectorHeight = 4
		listHeight = max(3, bodyHeight-inspectorHeight)
	}
	listRows := []string{}
	for i, s := range services {
		marker := "  "
		if i == m.row {
			marker = "> "
		}
		nameWidth := max(8, (leftWidth-5)*60/100)
		listRows = append(listRows, marker+padLine(operatorText(s.ServiceID), nameWidth)+"  "+stackObservedStatus(s))
	}
	selected := m.row
	title := "SERVICES · runtime order"
	if m.focus == 0 {
		title = "VIEWS · Enter opens"
		listRows = m.displayRows()
		for i := range listRows {
			mark := "  "
			if i == m.row {
				mark = "> "
			}
			listRows[i] = mark + operatorText(listRows[i])
		}
	}
	if len(listRows) == 0 {
		if current {
			listRows = []string{"No services in this observation."}
		} else {
			listRows = []string{"Waiting for a current observation."}
		}
	}
	// Keep column labels fixed while the selected observation scrolls.
	columnHeader := m.focus == 1 && listHeight >= 5
	visible := listHeight - 2
	if columnHeader {
		visible--
	}
	start := operatorVisibleStart(selected, len(listRows), visible)
	paneHeight := listHeight
	if columnHeader {
		paneHeight--
	}
	list := operatorPane(title, listRows[start:], leftWidth, paneHeight, selected-start, operatorMuted)
	if columnHeader {
		header := "   " + padLine("NAME", max(8, (leftWidth-5)*60/100)) + "  STATE"
		list = append(list[:1], append([]string{operatorRender(operatorBaseStyle.Foreground(operatorMuted), padLine(header, leftWidth))}, list[1:]...)...)
	}
	footer := fmt.Sprintf(" running %d · stopped %d · unknown %d", running, stopped, unknown)
	list[len(list)-1] = operatorRender(operatorBandStyle.Foreground(operatorMuted), padLine(footer, leftWidth))
	inspector := []string{"No selected service observation."}
	inspectorTitle := "INSPECTOR"
	if m.focus == 1 && m.row >= 0 && m.row < len(services) {
		s := services[m.row]
		inspectorTitle = operatorText(s.DisplayName) + " · " + stackObservedStatus(s)
		inspector = []string{"ID: " + operatorText(s.ServiceID), "Runtime: " + stackRunningLabel(s.Running) + " · Health: " + operatorText(s.HealthStatus), "Installed: " + operatorText(s.Installed) + " · Configured: " + operatorText(s.Configured), "Update: " + operatorText(s.UpdateStatus), operatorText(s.Description), "Latency / check history: unavailable", "Uptime / restarts: unavailable"}
	}
	if m.focus == 0 {
		inspector = []string{"Focus: Views", "↑↓ select a declared view", "Enter opens · Tab returns to services", "Unavailable views retain API admission."}
	}
	if !wide && m.focus == 1 && m.row >= 0 && m.row < len(services) {
		inspector = []string{inspector[0], inspector[1], "Latency / health history / uptime / restarts: unavailable"}
	}
	// Keep unavailable telemetry explicit at compact heights too.
	if len(inspector) > inspectorHeight-1 && inspectorHeight >= 4 {
		inspector = append(inspector[:inspectorHeight-3], "Latency / history: unavailable", "Uptime / restarts: unavailable")
	}
	detail := operatorPane(inspectorTitle, inspector, rightWidth, inspectorHeight, -1, operatorTeal)
	if wide {
		for i := range list {
			lines = append(lines, list[i]+"  "+detail[i])
		}
	} else {
		lines = append(lines, list...)
		lines = append(lines, detail...)
	}
	lines = append(lines, "")
	if len(attentionRows) == 0 {
		text := "No stopped or unhealthy summaries observed."
		if !current {
			text = "Observed service conditions unavailable."
		}
		attentionRows = append(attentionRows, text)
	}
	attentionRows = append(attentionRows, "Incident history unavailable · summary observations only")
	// Keep the incident provenance visible even when many services need attention.
	if len(attentionRows) > feedHeight-1 {
		attentionRows = append(attentionRows[:feedHeight-2], attentionRows[len(attentionRows)-1])
	}
	lines = append(lines, operatorPane(fmt.Sprintf("NEEDS ATTENTION · %d services · unverified %d", attention, unverified), attentionRows, inner, feedHeight, -1, operatorCoral)...)
	keys := " ↑↓ Select  Enter Inspect  ←→ Views  Tab Focus  r Refresh  f Filter"
	if w < 80 {
		keys = " ↑↓ Select  Enter Inspect  Tab Focus  r Refresh"
	}
	lines = append(lines, operatorRender(operatorBandStyle, padLine(keys, inner)))
	recovery := " Focus: " + focus + " · filter: " + m.filter() + " · n next · f filter · q Quit"
	if w < 80 {
		recovery = " " + focus + " · " + m.filter() + " · n Next · f Filter · q Quit"
	}
	lines = append(lines, operatorRender(operatorBaseStyle.Foreground(operatorMuted), padLine(recovery, inner)))
	if len(lines) > m.height {
		lines = lines[:m.height]
	}
	// Apply outer canvas gutters only after all cell-bounded surfaces are composed.
	for i, line := range lines {
		lines[i] = operatorRender(operatorCanvasStyle, strings.Repeat(" ", gutter)+padLine(line, inner)+strings.Repeat(" ", gutter))
	}
	return strings.Join(lines, "\n")
}

func stackObservedStatus(s api.OperatorService) string {
	if s.Running == "no" {
		return "Stopped"
	}
	if s.HealthStatus == "unhealthy" {
		return "Degraded"
	}
	if s.HealthStatus == "healthy" {
		return "Healthy"
	}
	return stackRunningLabel(s.Running) + " / unknown"
}
func stackRunningLabel(value string) string {
	switch value {
	case "yes":
		return "Running"
	case "no":
		return "Stopped"
	default:
		return "Unknown"
	}
}
func operatorVisibleStart(row, count, visible int) int {
	if visible < 1 {
		return 0
	}
	start := row - visible + 1
	if start < 0 {
		start = 0
	}
	if limit := count - visible; limit >= 0 && start > limit {
		start = limit
	}
	if start >= count {
		return 0
	}
	return start
}
func operatorPane(title string, content []string, width, height, selected int, accent lipgloss.Color) []string {
	out := []string{operatorRender(operatorBandStyle.Bold(true).Foreground(accent), padLine(" "+title, width))}
	for i := 0; i < height-1; i++ {
		text := ""
		if i < len(content) {
			text = content[i]
		}
		style := operatorBaseStyle
		if i == selected {
			style = operatorFocusStyle
		}
		out = append(out, operatorRender(style, padLine(" "+text, width)))
	}
	return out
}
