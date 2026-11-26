package app

import (
	"fmt"
	"strings"

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
		state = m.snapshotStatus()
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
	focus := "Services"
	if m.focus == 0 {
		focus = "Views"
	}
	lines := []string{m.operatorConnectionHeader(inner), operatorRender(operatorBaseStyle.Foreground(operatorMuted), padLine(" Service Lasso  operator/v2 · "+operatorText(state), inner)), m.operatorTabs(inner)}
	if m.height >= 24 {
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
			style := operatorBaseStyle.Foreground(colors[i])
			row += operatorRender(style, padLine("▌ "+text, tw))
		}
		lines = append(lines, row)
	}
	lines = append(lines, "")
	// Reserve a separate attention panel, footer and gaps before sizing the list.
	feedHeight := 4
	if m.height < 24 {
		feedHeight = 3
	}
	bodyHeight := m.height - len(lines) - feedHeight - 3
	if bodyHeight < 3 {
		bodyHeight = 3
	}
	wide := w >= 110
	leftWidth := (inner - 2) * 57 / 100
	rightWidth := inner - leftWidth - 2
	listHeight, inspectorHeight := bodyHeight, bodyHeight
	if !wide {
		leftWidth, rightWidth = inner, inner
		listHeight = max(3, bodyHeight/2-1)
		inspectorHeight = bodyHeight - listHeight
	}
	listRows := []string{}
	for i, s := range services {
		marker := "  "
		if i == m.row {
			marker = "> "
		}
		nameWidth := leftWidth - 19
		if nameWidth < 8 {
			nameWidth = 8
		}
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
	visible := listHeight - 2
	start := operatorVisibleStart(m.row, len(listRows), visible)
	list := operatorPane(title, listRows[start:], leftWidth, listHeight, selected-start, operatorMuted)
	footer := fmt.Sprintf(" running %d · stopped %d · unknown %d", running, stopped, unknown)
	list[len(list)-1] = operatorRender(operatorBaseStyle.Foreground(operatorMuted), padLine(footer, leftWidth))
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
	lines = append(lines, operatorRender(operatorBandStyle, padLine(" ↑↓ Select  Enter Inspect  ←→ Views  Tab Focus  r Refresh  f Filter", inner)))
	lines = append(lines, operatorRender(operatorBaseStyle.Foreground(operatorMuted), padLine(" Focus: "+focus+" · filter: "+m.filter()+" · n next page · q Quit", inner)))
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
