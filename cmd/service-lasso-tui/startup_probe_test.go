package main

import (
	"bytes"
	"crypto/sha256"
	"errors"
	"fmt"
	"os"
	"runtime/debug"
	"testing"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/service-lasso/service-lasso-tui/internal/api"
)

const probeNonce = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"

func TestStartupProbeMarkerRequiresAnExactNonceAndKnownBoundary(t *testing.T) {
	source := "0123456789abcdef0123456789abcdef01234567"
	marker, ok := startupProbeMarkerWithIdentity(probeNonce, startupBoundaryProgramRunError, source, probeNonce)
	if !ok || marker != "\x1eSERVICE_LASSO_TUI_STARTUP_BOUNDARY:"+probeNonce+":program_run_error:"+source+":"+probeNonce+"\x1f" {
		t.Fatalf("unexpected marker: %q, %t", marker, ok)
	}
	if marker, ok := startupProbeMarkerWithIdentity("not-a-nonce", startupBoundaryProgramRunError, source, probeNonce); ok || marker != "" {
		t.Fatalf("invalid nonce produced a marker: %q", marker)
	}
	if marker, ok := startupProbeMarkerWithIdentity(probeNonce, startupBoundaryUnclassified, source, probeNonce); ok || marker != "" {
		t.Fatalf("unknown boundary produced a marker: %q", marker)
	}
}

func TestStartupProbeSourceCommitRequiresTheExpectedModuleAndCleanVCSBuildInfo(t *testing.T) {
	info := &debug.BuildInfo{Main: debug.Module{Path: "github.com/service-lasso/service-lasso-tui"}, Settings: []debug.BuildSetting{{Key: "vcs.revision", Value: "0123456789abcdef0123456789abcdef01234567"}, {Key: "vcs.modified", Value: "false"}}}
	if commit, ok := startupProbeSourceCommit(info); !ok || commit != "0123456789abcdef0123456789abcdef01234567" {
		t.Fatalf("source commit = %q, %t", commit, ok)
	}
	info.Settings[1].Value = "true"
	if _, ok := startupProbeSourceCommit(info); ok {
		t.Fatal("dirty build info was admitted")
	}
}

func TestStartupProbeSelfSHA256MatchesTheOpenedExecutable(t *testing.T) {
	got, ok := startupProbeSelfSHA256()
	if !ok {
		t.Fatal("self hash unavailable")
	}
	path, err := os.Executable()
	if err != nil {
		t.Fatal(err)
	}
	data, err := os.ReadFile(path)
	if err != nil {
		t.Fatal(err)
	}
	want := fmt.Sprintf("%x", sha256.Sum256(data))
	if got != want {
		t.Fatalf("self hash = %s, want %s", got, want)
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
	if got := startupBoundaryForAPIError(errors.New("invalid Service Lasso API URL")); got != startupBoundaryAPIClientError {
		t.Fatalf("fixed-message API boundary = %q", got)
	}
}

func TestStartupProbeDoesNotEmitRawErrorWhenExplicitlyEnabled(t *testing.T) {
	t.Setenv(startupProbeNonceEnvironment, probeNonce)
	var output bytes.Buffer
	reportStartupFailure(&output, startupBoundaryProgramRunError, errors.New("SYNTHETIC_SECRET"))
	if output.String() != "" {
		t.Fatalf("identity-unavailable probe output = %q", output.String())
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
