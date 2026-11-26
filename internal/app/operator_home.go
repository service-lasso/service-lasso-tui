package app

import (
	"fmt"
	"strings"
	"time"

	"github.com/charmbracelet/lipgloss"
	"github.com/charmbracelet/x/ansi"
	"github.com/service-lasso/service-lasso-tui/internal/api"
)

const operatorStackMinWidth, operatorStackMinHeight = 60, 18

var stackShortcuts = [...]struct{ label, journey string }{
	{"Services", "detail"},
	{"Operations", "history"},
	{"Logs", "logs"},
}

// operatorStackView is strictly a presentation of the admitted overview page.
// All selection consumers use overviewServices in its original order.
func (m operatorModel) operatorStackView() string {
	w := m.width
	now := time.Now()
	wide := w >= 110
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
		if m.services.Stale || operatorSnapshotExpired(m.services.Expiry, now) {
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
	age := m.overviewReadAge(now)
	if w < 80 {
		age = strings.ReplaceAll(age, "read age unavailable", "age unavailable")
		if current {
			state = "current"
			if !m.services.Complete {
				state = "partial · n next"
			}
			if m.services.Truncated {
				state = "truncated"
			}
			if m.services.Stale || operatorSnapshotExpired(m.services.Expiry, now) {
				state = "stale · refresh"
			}
		}
		if m.loading {
			state = "Loading · " + state
		}
		if m.errorText != "" {
			state = "Read failed · " + state
		}
	}
	lines := []string{
		m.operatorConnectionHeader(inner),
		operatorRender(operatorBaseStyle.Foreground(operatorMuted), padLine(" "+age+" · "+m.overviewMonitorMode()+" · "+state, inner)),
	}
	if wide {
		lines = m.stackHeader(inner, state, age, current, now)
		if m.height >= 30 {
			lines = append([]string{""}, lines...)
		}
	}
	lines = append(lines, stackNavigation(inner, current && m.focus == 1 && m.row >= 0 && m.row < len(services)))
	if m.height >= 26 && w >= 110 {
		lines = append(lines, "")
	}
	attentionCount := fmt.Sprintf("%d services", attention)
	if attention == 1 {
		attentionCount = "1 service"
	}
	values := []string{"Unavailable", "Unavailable", "Count unavailable", "Unavailable"}
	labels := []string{"HEALTH", "NEEDS ATTENTION", "ACTIVE OPERATIONS", "CORE UPTIME"}
	colors := []lipgloss.Color{operatorGreen, operatorCoral, operatorTeal, operatorTextColor}
	if !current {
		colors[0] = operatorMuted
	} else {
		checked := len(services) - unverified
		values[0] = fmt.Sprintf("%d healthy · %d/%d checked", healthy, checked, len(services))
		if checked == 0 {
			values[0] = fmt.Sprintf("0 of %d checked", len(services))
		}
		values[1] = fmt.Sprintf("%d stopped or unhealthy", attention)
		if !wide {
			values[0] = fmt.Sprintf("%d/%d checked", checked, len(services))
			values[1] = attentionCount
		}
		if checked == 0 {
			colors[0] = operatorMuted
		} else if healthy < checked {
			colors[0] = operatorAmber
		}
	}
	if !wide {
		labels[1], labels[2] = "ATTENTION", "OPERATION"
		values[2] = "Unavailable"
	}
	if w < 80 {
		labels[3] = "UPTIME"
	}
	tileHeight := 2
	if wide && m.height >= 30 {
		tileHeight = 3
	}
	for line := 0; line < tileHeight; line++ {
		row := ""
		for i, label := range labels {
			tw := (inner - 3) / 4
			if i == 3 {
				tw = inner - 3 - 3*tw
			}
			style := operatorBaseStyle.Foreground(operatorMuted).Bold(true)
			text := label
			if line == 1 {
				style = style.Foreground(colors[i])
				text = values[i]
			} else if line == 2 {
				text = ""
			}
			if i > 0 {
				row += operatorRender(operatorCanvasStyle, " ")
			}
			rail := operatorBaseStyle.Foreground(colors[i]).Background(colors[i])
			row += operatorRender(rail, "▌") + operatorRender(style, padLine(" "+text, tw-1))
		}
		lines = append(lines, row)
	}
	if w >= 110 && m.height >= 24 {
		lines = append(lines, "")
	}
	// Reserve a separate attention panel, footer and gaps before sizing the list.
	feedLimit := 6
	if wide && m.height >= 30 {
		feedLimit = 8
	}
	feedHeight := min(feedLimit, max(4, len(attentionRows)+2))
	if m.height < 24 || !wide {
		feedHeight = 3
	}
	bodyHeight := m.height - len(lines) - feedHeight - 3
	// At the reference height, use spare rows inside the data panels. Keep
	// the attention feed near the data on tall, sparse canvases.
	if wide && m.height > 36 {
		bodyHeight = min(bodyHeight, max(10, len(services)+3))
	}
	if bodyHeight < 3 {
		bodyHeight = 3
	}
	leftWidth := (inner - 2) * 58 / 100
	rightWidth := inner - leftWidth - 2
	listHeight, inspectorHeight := bodyHeight, bodyHeight
	if !wide {
		leftWidth, rightWidth = inner, inner
		inspectorHeight = 4
		listHeight = max(3, bodyHeight-inspectorHeight)
	}
	// Reserve the service rows here. Render only the visible rows once the
	// viewport and selection offset are known.
	listRows := make([]string, len(services))
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
	if m.focus == 1 {
		if wide {
			name, order := " SERVICES", "Runtime order "
			list[0] = operatorRender(operatorBandStyle.Bold(true), padLine(name, leftWidth-len(order))) +
				operatorRender(operatorBandStyle.Foreground(operatorMuted), order)
		}
		for j := 0; j < visible && start+j < len(services); j++ {
			i := start + j
			s := services[i]
			style := operatorBaseStyle
			if i%2 == 1 {
				style = style.Background(operatorStripe)
			}
			rail, marker := operatorRender(style, " "), "  "
			if i == m.row {
				style = operatorFocusStyle.Foreground(operatorTextColor)
				rail, marker = operatorRender(operatorFocusStyle, "▌"), "> "
			}
			prefix := marker + padLine(operatorText(s.ServiceID), max(8, (leftWidth-5)*60/100)) + "  "
			tail := padLine(stackObservedStatus(s), leftWidth-1-ansi.StringWidth(prefix))
			list[1+j] = rail + operatorRender(style, prefix) + operatorRender(style.Foreground(stackStatusColor(s)), tail)
		}
	}
	if columnHeader {
		header := "   " + padLine("NAME", max(8, (leftWidth-5)*60/100)) + "  STATE"
		list = append(list[:1], append([]string{operatorRender(operatorBaseStyle.Foreground(operatorMuted), padLine(header, leftWidth))}, list[1:]...)...)
	}
	footer := fmt.Sprintf(" running %d · stopped %d · unknown %d", running, stopped, unknown)
	list[len(list)-1] = operatorRender(operatorBandStyle.Foreground(operatorMuted), padLine(footer, leftWidth))
	inspector := []string{"No selected service observation."}
	inspectorTitle := "INSPECTOR"
	inspectorStatus := ""
	inspectorColor := operatorMuted
	if m.focus == 1 && m.row >= 0 && m.row < len(services) {
		s := services[m.row]
		inspectorTitle = operatorText(s.DisplayName)
		inspectorStatus = strings.ToUpper(stackObservedStatus(s))
		inspectorColor = stackStatusColor(s)
		inspector = []string{"ID: " + operatorText(s.ServiceID), "Runtime: " + stackRunningLabel(s.Running) + " · Health: " + operatorText(s.HealthStatus), "Installed: " + operatorText(s.Installed) + " · Configured: " + operatorText(s.Configured), "Update: " + operatorText(s.UpdateStatus), operatorText(s.Description), "Latency / check history: unavailable", "Uptime / restarts: unavailable"}
		if wide && inspectorHeight >= 13 {
			inspector = append(inspector[:5], "", "TELEMETRY", "Latency / check history: unavailable", "Uptime / restarts: unavailable")
		}
	}
	if m.focus == 0 {
		inspector = []string{"Focus: Views", "↑↓ select a declared view", "Enter opens · Tab returns to services", "Unavailable views retain API admission."}
	}
	if !wide && m.focus == 1 && m.row >= 0 && m.row < len(services) {
		inspector = []string{inspector[0], inspector[1], "Latency / history / uptime / restarts: unavailable"}
	}
	// Keep unavailable telemetry explicit at compact heights too.
	if len(inspector) > inspectorHeight-1 && inspectorHeight >= 4 {
		inspector = append(inspector[:inspectorHeight-3], "Latency / history: unavailable", "Uptime / restarts: unavailable")
	}
	detail := operatorPane(inspectorTitle, inspector, rightWidth, inspectorHeight, -1, operatorTeal)
	if inspectorStatus != "" {
		band := operatorFocusStyle
		nameWidth := rightWidth - ansi.StringWidth(inspectorStatus) - 2
		detail[0] = operatorRender(band, padLine(" "+inspectorTitle, nameWidth)) +
			operatorRender(band.Foreground(inspectorColor), " "+inspectorStatus+" ")
	}
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
	feed := operatorPane("NEEDS ATTENTION", attentionRows, inner, feedHeight, -1, operatorCoral)
	alert := operatorBandStyle.Background(operatorAlertBand).Foreground(operatorCoral)
	heading := " NEEDS ATTENTION · " + attentionCount
	count := fmt.Sprintf(" · unverified %d", unverified)
	if wide {
		count = fmt.Sprintf("unverified %d ", unverified)
		heading = padLine(heading, inner-1-len(count))
	}
	feed[0] = operatorRender(alert, "▌") + operatorRender(alert.Bold(true), heading) +
		operatorRender(alert, padLine(count, inner-1-ansi.StringWidth(heading)))
	lines = append(lines, feed...)
	// Keep recovery commands at the physical bottom without separating data
	// from its attention feed on a tall terminal.
	for len(lines) < m.height-2 {
		lines = append(lines, "")
	}
	keys := [][2]string{{"↑↓", "Select"}, {"Enter", "Inspect"}, {"Tab", "Focus"}, {"r", "Refresh"}}
	if w >= 80 {
		keys = append(keys, [2]string{"f", "Filter"})
	}
	if wide && m.focus == 1 && (m.overviewPaused || m.overviewMonitorEligible()) {
		label := "Pause"
		if m.overviewPaused {
			label = "Resume"
		}
		keys = append(keys, [2]string{"Space", label})
	}
	lines = append(lines, stackKeycaps(inner, keys))
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
		lines[i] = operatorRender(operatorCanvasStyle, strings.Repeat(" ", gutter)) + line + operatorRender(operatorCanvasStyle, strings.Repeat(" ", max(gutter, inner+gutter-ansi.StringWidth(line))))
	}
	return strings.Join(lines, "\n")
}

func (m operatorModel) stackHeader(width int, state, age string, current bool, now time.Time) []string {
	rightWidth := min(46, width/2)
	leftWidth := width - rightWidth
	left := operatorRender(operatorBrandStyle, " SL ") + operatorRender(operatorBandStyle.Bold(true), padLine(" Service Lasso  operator/v2", leftWidth-4))
	status, color := "CONNECTING", operatorMuted
	switch {
	case m.errorText != "":
		status, color = "READ FAILED", operatorCoral
	case m.loading:
		status, color = "READING", operatorTeal
	case !current && m.discovery != nil:
		status = "API CONNECTED"
	case !current:
	case m.services.Stale || operatorSnapshotExpired(m.services.Expiry, now):
		status, color = "STALE", operatorAmber
	case m.overviewPaused:
		status, color = "PAUSED", operatorAmber
	case m.overviewMonitorEligible() && !m.overviewObservedAt.IsZero() && !now.Before(m.overviewObservedAt) && now.Sub(m.overviewObservedAt) <= 2*overviewRefreshInterval:
		status, color = "LIVE", operatorGreen
	default:
		status = "OBSERVED"
	}
	profile := operatorText(m.connection)
	if profile == "" {
		profile = "default"
	}
	profile = ansi.Truncate(profile, min(20, leftWidth/3), "…")
	subtitle := operatorBandStyle.Foreground(operatorMuted)
	right := operatorRender(operatorBandStyle.Foreground(color).Bold(true), padLine(" ● "+status+" · "+age, rightWidth))
	return []string{
		left + right,
		operatorRender(subtitle, padLine("     "+profile+" · "+state, leftWidth)) +
			operatorRender(subtitle, padLine("   "+m.overviewMonitorMode(), rightWidth)),
	}
}

func stackNavigation(width int, hasService bool) string {
	line := operatorRender(operatorBrandStyle, " 1 [Stack] ")
	if hasService {
		for i, shortcut := range stackShortcuts {
			line += operatorRender(operatorCanvasStyle.Foreground(operatorMuted), fmt.Sprintf("  %d %s", i+2, shortcut.label))
		}
	}
	if width >= 78 {
		line += operatorRender(operatorCanvasStyle.Foreground(operatorMuted), "   Tab / ←→ All views")
	}
	return padLine(line, width)
}

func stackKeycaps(width int, keys [][2]string) string {
	line := ""
	for _, key := range keys {
		line += operatorRender(operatorBandStyle.Bold(true), " "+key[0]+" ")
		line += operatorRender(operatorBaseStyle.Foreground(operatorMuted), key[1]+"  ")
	}
	return line + operatorRender(operatorBaseStyle, padLine("", width-ansi.StringWidth(line)))
}

func stackStatusColor(s api.OperatorService) lipgloss.Color {
	if s.Running == "no" {
		return operatorCoral
	}
	if s.HealthStatus == "unhealthy" {
		return operatorAmber
	}
	if s.HealthStatus == "healthy" {
		return operatorGreen
	}
	return operatorMuted
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
