package app

import (
	"fmt"
	"strings"
	"time"

	"github.com/charmbracelet/lipgloss"
	"github.com/charmbracelet/x/ansi"
)

// One semantic palette for reads, editors, confirmation and retained outcomes.
// Words and selection markers carry the same meaning without terminal colour.
var (
	operatorCanvas      = lipgloss.Color("#0B1420")
	operatorSurface     = lipgloss.Color("#152233")
	operatorBand        = lipgloss.Color("#223249")
	operatorTextColor   = lipgloss.Color("#D7E1EF")
	operatorMuted       = lipgloss.Color("#96A9C0")
	operatorTeal        = lipgloss.Color("#54DFD0")
	operatorGreen       = lipgloss.Color("#ADE88E")
	operatorAmber       = lipgloss.Color("#FFCC70")
	operatorCoral       = lipgloss.Color("#FF8797")
	operatorBaseStyle   = lipgloss.NewStyle().Foreground(operatorTextColor).Background(operatorSurface)
	operatorCanvasStyle = lipgloss.NewStyle().Foreground(operatorTextColor).Background(operatorCanvas)
	operatorBandStyle   = lipgloss.NewStyle().Foreground(operatorTextColor).Background(operatorBand)
	operatorFocusStyle  = lipgloss.NewStyle().Bold(true).Foreground(operatorTeal).Background(lipgloss.Color("#1B4650"))
)

func operatorRender(style lipgloss.Style, text string) string {
	if !tuiColorEnabled() {
		return text
	}
	return style.Render(text)
}

func operatorViewLabel(name string) string {
	switch name {
	case "overview":
		return "Stack"
	case "detail":
		return "Services"
	case "declared_actions":
		return "Actions"
	case "health_history":
		return "Health"
	}
	return strings.ToUpper(name[:1]) + strings.ReplaceAll(name[1:], "_", " ")
}

// The active tab stays visible in a bounded window of actual keyboard journeys.
// This is presentation only: it never fabricates shortcut keys or destinations.
func (m operatorModel) operatorTabs(width int) string {
	selected := 0
	for i, name := range operatorViews {
		if name == m.journey {
			selected = i
			break
		}
	}
	start := selected
	used := ansi.StringWidth(operatorViewLabel(operatorViews[selected])) + 4
	for start > 0 {
		n := ansi.StringWidth(operatorViewLabel(operatorViews[start-1])) + 4
		if used+n > width {
			break
		}
		used += n
		start--
	}
	parts := []string{}
	for i := start; i < len(operatorViews); i++ {
		label := " " + operatorViewLabel(operatorViews[i]) + " "
		if i == selected {
			label = "[" + operatorViewLabel(operatorViews[i]) + "]"
		}
		if ansi.StringWidth(strings.Join(parts, "  "))+ansi.StringWidth(label)+2 > width {
			break
		}
		if i == selected {
			label = operatorRender(operatorFocusStyle, label)
		}
		parts = append(parts, label)
	}
	return padLine(strings.Join(parts, "  "), width)
}

func (m operatorModel) operatorConnectionHeader(width int) string {
	status := "CONNECTING"
	if m.discovery != nil {
		status = "API CONNECTED"
	}
	if m.loading {
		status = "READING"
	}
	if m.errorText != "" {
		status = "READ FAILED"
	}
	profile := ansi.Truncate(operatorText(m.connection), max(8, width/4), "…")
	if profile == "" {
		profile = "default"
	}
	return operatorRender(operatorBandStyle.Bold(true), padLine(fmt.Sprintf(" SL  Service Lasso  operator/v2 · %s · %s", profile, status), width))
}

// Read receipt age is a local successful observation, never probe latency,
// runtime uptime, or evidence that the underlying snapshot remains fresh.
func (m operatorModel) overviewReadAge(now time.Time) string {
	if !m.pageSelectionCurrent() || m.overviewObservedAt.IsZero() {
		return "read age unavailable"
	}
	age := now.Sub(m.overviewObservedAt)
	if age < 0 {
		age = 0
	}
	return "updated " + age.Truncate(time.Second).String() + " ago"
}
