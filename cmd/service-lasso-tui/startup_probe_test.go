package main

import (
	"bytes"
	"errors"
	"strings"
	"testing"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/service-lasso/service-lasso-tui/internal/api"
)

const probeNonce = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"

func TestStartupProbeMarkerRequiresAnExactNonceAndKnownBoundary(t *testing.T) {
	marker, ok := startupProbeMarker(probeNonce, startupBoundaryProgramRunError)
	if !ok || marker != "\x1eSERVICE_LASSO_TUI_STARTUP_BOUNDARY:"+probeNonce+":program_run_error\x1f" {
		t.Fatalf("unexpected marker: %q, %t", marker, ok)
	}
	if marker, ok := startupProbeMarker("not-a-nonce", startupBoundaryProgramRunError); ok || marker != "" {
		t.Fatalf("invalid nonce produced a marker: %q", marker)
	}
	if marker, ok := startupProbeMarker(probeNonce, startupBoundaryUnclassified); ok || marker != "" {
		t.Fatalf("unknown boundary produced a marker: %q", marker)
	}
}

func TestStartupProbeClassifiesOnlyBubbleTeaSentinels(t *testing.T) {
	if got := startupBoundaryForProgramError(errors.Join(tea.ErrProgramKilled, tea.ErrProgramPanic)); got != startupBoundaryProgramRunPanic {
		t.Fatalf("panic boundary = %q", got)
	}
	if got := startupBoundaryForProgramError(tea.ErrProgramKilled); got != startupBoundaryProgramRunKilled {
		t.Fatalf("killed boundary = %q", got)
	}
	if got := startupBoundaryForProgramError(tea.ErrInterrupted); got != startupBoundaryProgramRunInterrupted {
		t.Fatalf("interrupted boundary = %q", got)
	}
	if got := startupBoundaryForProgramError(errors.New("SYNTHETIC_SECRET")); got != startupBoundaryProgramRunError {
		t.Fatalf("generic boundary = %q", got)
	}
}

func TestStartupProbeClassifiesOnlyTypedAPIConfigurationErrors(t *testing.T) {
	cases := []struct {
		kind     api.ConfigurationErrorKind
		boundary startupBoundary
	}{
		{api.ConfigurationErrorInvalidURL, startupBoundaryAPIURLInvalid},
		{api.ConfigurationErrorUnsupportedScheme, startupBoundaryAPIURLScheme},
		{api.ConfigurationErrorUserinfo, startupBoundaryAPIURLUserinfo},
		{api.ConfigurationErrorQueryOrFragment, startupBoundaryAPIURLQueryOrFragment},
		{api.ConfigurationErrorInsecureTokenTransport, startupBoundaryAPITokenTransport},
	}
	for _, test := range cases {
		t.Run(string(test.kind), func(t *testing.T) {
			if got := startupBoundaryForAPIError(&api.ConfigurationError{Kind: test.kind}); got != test.boundary {
				t.Fatalf("boundary = %q, want %q", got, test.boundary)
			}
		})
	}
	if got := startupBoundaryForAPIError(errors.New("SYNTHETIC_SECRET")); got != startupBoundaryAPIClientError {
		t.Fatalf("generic API boundary = %q", got)
	}
}

func TestStartupProbeDoesNotEmitRawErrorWhenExplicitlyEnabled(t *testing.T) {
	t.Setenv(startupProbeNonceEnvironment, probeNonce)
	var output bytes.Buffer
	reportStartupFailure(&output, startupBoundaryProgramRunError, errors.New("SYNTHETIC_SECRET"))
	if strings.Contains(output.String(), "SYNTHETIC_SECRET") || !strings.Contains(output.String(), probeNonce) {
		t.Fatalf("unsafe probe output: %q", output.String())
	}
}

func TestNormalStartupFailureRetainsTheOperatorError(t *testing.T) {
	t.Setenv(startupProbeNonceEnvironment, "")
	var output bytes.Buffer
	reportStartupFailure(&output, startupBoundaryProgramRunError, errors.New("operator-visible error"))
	if output.String() != "operator-visible error\n" {
		t.Fatalf("normal error output = %q", output.String())
	}
}
