package app

import (
	"fmt"
	"strings"
	"time"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/service-lasso/service-lasso-tui/internal/api"
)

type operatorPinnedRelease struct {
	Ticket                                        operatorTicket
	CurrentTag, CandidateTag, InstalledTag, Error string
	Busy, Installing                              bool
}
type operatorPinnedReleaseResult struct {
	Ticket     operatorTicket
	Check      api.PinnedReleaseCheck
	Installed  api.PinnedReleaseInstall
	Installing bool
	Err        error
}

func pinnedClientService(id string) bool { return id == "@tui" || id == "@cli" }
func (m operatorModel) pinnedReleaseRow() bool {
	return m.journey == "lifecycle" && pinnedClientService(m.service) && m.lifecycle != nil && m.row == len(m.lifecycle.Capabilities)
}
func (m *operatorModel) beginPinnedRelease() tea.Cmd {
	m.expirePages(time.Now())
	if m.focus != 1 || !m.pageSelectionCurrent() || m.formReadFailed() || m.mutationBlocked() || m.loading || m.discovery == nil || !m.binding.Valid() || m.draft.Dirty || m.draft.Sent || m.operation != nil && !m.operation.Operation.CanClear() {
		return nil
	}
	ticket := m.ticket()
	m.pinnedRelease = &operatorPinnedRelease{Ticket: ticket, Busy: true}
	client, ctx := m.client, m.requestContext()
	return func() tea.Msg {
		result := operatorPinnedReleaseResult{Ticket: ticket}
		if result.Err = operatorSameContext(ctx, client, ticket.Binding); result.Err != nil {
			return result
		}
		result.Check, result.Err = client.CheckPinnedRelease(ctx, ticket.Service)
		return result
	}
}
func (m operatorModel) pinnedReleaseCurrent() bool {
	return m.pinnedRelease != nil && m.pinnedRelease.Ticket == m.ticket() && m.pageSelectionCurrent()
}
func (m operatorModel) acceptPinnedRelease(msg operatorPinnedReleaseResult) (tea.Model, tea.Cmd) {
	if !m.pinnedReleaseCurrent() || m.pinnedRelease.Ticket != msg.Ticket {
		return m, nil
	}
	state := m.pinnedRelease
	state.Busy = false
	if msg.Err != nil {
		state.Error = api.SafeError(msg.Err)
		if msg.Installing {
			state.Error = "Install outcome unavailable. Check the service version before trying again."
		}
		return m, nil
	}
	if msg.Installing {
		state.InstalledTag = msg.Installed.InstalledTag
		state.CandidateTag = ""
	} else {
		state.CurrentTag = msg.Check.CurrentTag
		if msg.Check.CandidateTag != nil {
			state.CandidateTag = *msg.Check.CandidateTag
		}
	}
	return m, nil
}
func (m operatorModel) pinnedReleaseKey(key tea.KeyMsg) (tea.Model, tea.Cmd) {
	if key.String() == "ctrl+c" {
		m.invalidate()
		return m, tea.Quit
	}
	if !m.pinnedReleaseCurrent() {
		m.pinnedRelease = nil
		return m, nil
	}
	state := m.pinnedRelease
	if key.String() == "esc" || key.String() == "q" {
		if !state.Busy {
			m.pinnedRelease = nil
		}
		return m, nil
	}
	if key.String() != "enter" || state.Busy || state.Error != "" || state.CandidateTag == "" || state.Installing {
		return m, nil
	}
	// The confirmation displays the complete tag and service before dispatch.
	if len(m.pinnedReleaseLines()) > m.height {
		state.Error = "Enlarge the terminal to review the complete release before installing."
		return m, nil
	}
	for _, line := range m.pinnedReleaseLines() {
		if len([]rune(line)) > m.width {
			state.Error = "Enlarge the terminal to review the complete release before installing."
			return m, nil
		}
	}
	state.Busy = true
	state.Installing = true
	ticket, tag, client, ctx := state.Ticket, state.CandidateTag, m.client, m.requestContext()
	return m, func() tea.Msg {
		result := operatorPinnedReleaseResult{Ticket: ticket, Installing: true}
		if result.Err = operatorSameContext(ctx, client, ticket.Binding); result.Err != nil {
			return result
		}
		result.Installed, result.Err = client.InstallPinnedRelease(ctx, ticket.Service, tag)
		return result
	}
}
func (m operatorModel) pinnedReleaseLines() []string {
	state := m.pinnedRelease
	lines := []string{"Service Lasso · New releases", "Service " + state.Ticket.Service}
	if state.CurrentTag != "" {
		lines = append(lines, "Current "+state.CurrentTag)
	}
	if state.CandidateTag != "" {
		lines = append(lines, "New "+state.CandidateTag)
	}
	switch {
	case state.Error != "":
		lines = append(lines, state.Error, "Esc Back")
	case state.Busy:
		label := "Checking for releases..."
		if state.Installing {
			label = "Installing release..."
		}
		lines = append(lines, label)
	case state.InstalledTag != "":
		lines = append(lines, "Installed "+state.InstalledTag, "The new version is used on the next client launch.", "Esc Back")
	case state.CandidateTag != "":
		lines = append(lines, fmt.Sprintf("Install this release for %s?", state.Ticket.Service), "Enter Install · Esc Cancel")
	default:
		lines = append(lines, "The pinned release is current.", "Esc Back")
	}
	return lines
}
func (m operatorModel) pinnedReleaseView() string {
	lines := m.pinnedReleaseLines()
	for i, line := range lines {
		lines[i] = operatorText(line)
	}
	return strings.Join(lines, "\n")
}
