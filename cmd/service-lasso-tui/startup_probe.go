package main

import (
	"errors"
	"fmt"
	"io"
	"os"
	"regexp"

	tea "github.com/charmbracelet/bubbletea"
)

const startupProbeNonceEnvironment = "SERVICE_LASSO_STARTUP_PROBE_NONCE"

type startupBoundary string

const (
	startupBoundaryUnclassified          startupBoundary = "unclassified"
	startupBoundaryAPIClientError        startupBoundary = "api_client_error"
	startupBoundaryProgramRunError       startupBoundary = "program_run_error"
	startupBoundaryProgramRunKilled      startupBoundary = "program_run_killed"
	startupBoundaryProgramRunPanic       startupBoundary = "program_run_panic"
	startupBoundaryProgramRunInterrupted startupBoundary = "program_run_interrupted"
)

var startupProbeNoncePattern = regexp.MustCompile(`\A[a-f0-9]{64}\z`)

func startupBoundaryForProgramError(err error) startupBoundary {
	switch {
	case errors.Is(err, tea.ErrProgramPanic):
		return startupBoundaryProgramRunPanic
	case errors.Is(err, tea.ErrProgramKilled):
		return startupBoundaryProgramRunKilled
	case errors.Is(err, tea.ErrInterrupted):
		return startupBoundaryProgramRunInterrupted
	default:
		return startupBoundaryProgramRunError
	}
}

func startupProbeMarker(nonce string, boundary startupBoundary) (string, bool) {
	if !startupProbeNoncePattern.MatchString(nonce) {
		return "", false
	}
	switch boundary {
	case startupBoundaryAPIClientError, startupBoundaryProgramRunError, startupBoundaryProgramRunKilled, startupBoundaryProgramRunPanic, startupBoundaryProgramRunInterrupted:
		return "\x1eSERVICE_LASSO_TUI_STARTUP_BOUNDARY:" + nonce + ":" + string(boundary) + "\x1f", true
	default:
		return "", false
	}
}

func reportStartupFailure(writer io.Writer, boundary startupBoundary, err error) {
	if marker, ok := startupProbeMarker(os.Getenv(startupProbeNonceEnvironment), boundary); ok {
		// This opt-in marker is consumed in memory by the owned ConPTY helper.
		// It intentionally contains neither the original error nor environment data.
		fmt.Fprint(writer, marker)
		return
	}
	fmt.Fprintln(writer, err)
}
