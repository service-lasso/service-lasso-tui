package app

import (
	"time"

	tea "github.com/charmbracelet/bubbletea"
)

const overviewRefreshInterval = 5 * time.Second

// Refresh only a whole first page. Never replace a manually accumulated page,
// interrupt a foreground read/editor, or share an operation's polling lifetime.
func (m operatorModel) overviewMonitorEligible() bool {
	if m.width < operatorStackMinWidth || m.height < operatorStackMinHeight || m.journey != "overview" || m.focus != 1 || m.favoritesOnly || m.discovery == nil || m.client == nil || m.ctx == nil || !m.pageSelectionCurrent() || m.services.Snapshot == "" || !m.services.Complete || m.services.Truncated || m.services.Next != nil || len(m.services.seenCursors) != 0 {
		return false
	}
	if m.bootInFlight || m.draft.Dirty || m.draft.Preview != nil || m.draft.Sent || m.draft.Retired || m.operation != nil || m.warning != "" || m.custodyWarning != "" || m.terminalRefreshWarning != "" || len(m.fieldErrors) != 0 {
		return false
	}
	allowed, _ := operatorReadAdmission(m.discovery, "overview")
	return allowed
}
func (m *operatorModel) refreshOverviewIfDue(now time.Time) tea.Cmd {
	if m.overviewPaused || m.loading || !m.overviewMonitorEligible() {
		return nil
	}
	since := m.overviewObservedAt
	if m.overviewAttemptAt.After(since) {
		since = m.overviewAttemptAt
	}
	if since.IsZero() || now.Sub(since) < overviewRefreshInterval {
		return nil
	}
	m.overviewAttemptAt = now
	return m.load(nil)
}
func (m operatorModel) overviewMonitorMode() string {
	if m.focus != 1 {
		return "View navigation · monitoring suspended"
	}
	if m.overviewPaused {
		return "Paused · Space Resume"
	}
	if !m.overviewMonitorEligible() {
		return "manual refresh"
	}
	return "Auto-refresh 5s · Space Pause"
}
