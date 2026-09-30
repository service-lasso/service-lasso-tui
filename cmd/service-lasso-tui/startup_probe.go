package main

import (
	"crypto/sha256"
	"errors"
	"fmt"
	"io"
	"os"
	"regexp"
	"runtime/debug"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/service-lasso/service-lasso-tui/internal/api"
)

const startupProbeNonceEnvironment = "SERVICE_LASSO_STARTUP_PROBE_NONCE"

type startupBoundary string

const (
	startupBoundaryUnclassified          startupBoundary = "unclassified"
	startupBoundaryAPIURLInvalid         startupBoundary = "api_url_invalid"
	startupBoundaryAPIURLScheme          startupBoundary = "api_url_scheme"
	startupBoundaryAPIURLUserinfo        startupBoundary = "api_url_userinfo"
	startupBoundaryAPIURLQueryOrFragment startupBoundary = "api_url_query_or_fragment"
	startupBoundaryAPITokenTransport     startupBoundary = "api_token_transport"
	startupBoundaryAPIClientError        startupBoundary = "api_client_error"
	startupBoundaryProgramRunError       startupBoundary = "program_run_error"
	startupBoundaryProgramRunKilled      startupBoundary = "program_run_killed"
	startupBoundaryProgramRunPanic       startupBoundary = "program_run_panic"
	startupBoundaryProgramRunInterrupted startupBoundary = "program_run_interrupted"
)

func startupBoundaryForAPIError(err error) startupBoundary {
	var configurationError *api.ConfigurationError
	if !errors.As(err, &configurationError) {
		return startupBoundaryAPIClientError
	}
	switch configurationError.Kind {
	case api.ConfigurationErrorInvalidURL:
		return startupBoundaryAPIURLInvalid
	case api.ConfigurationErrorUnsupportedScheme:
		return startupBoundaryAPIURLScheme
	case api.ConfigurationErrorUserinfo:
		return startupBoundaryAPIURLUserinfo
	case api.ConfigurationErrorQueryOrFragment:
		return startupBoundaryAPIURLQueryOrFragment
	case api.ConfigurationErrorInsecureTokenTransport:
		return startupBoundaryAPITokenTransport
	default:
		return startupBoundaryAPIClientError
	}
}

var startupProbeNoncePattern = regexp.MustCompile(`\A[a-f0-9]{64}\z`)
var startupProbeSourceCommitPattern = regexp.MustCompile(`\A[a-f0-9]{40}\z`)

func startupProbeSourceCommit(info *debug.BuildInfo) (string, bool) {
	if info == nil || info.Main.Path != "github.com/service-lasso/service-lasso-tui" {
		return "", false
	}
	var revision, modified string
	for _, setting := range info.Settings {
		switch setting.Key {
		case "vcs.revision":
			revision = setting.Value
		case "vcs.modified":
			modified = setting.Value
		}
	}
	return revision, modified == "false" && startupProbeSourceCommitPattern.MatchString(revision)
}

func startupProbeSelfSHA256() (string, bool) {
	path, err := os.Executable()
	if err != nil {
		return "", false
	}
	file, err := os.Open(path)
	if err != nil {
		return "", false
	}
	defer file.Close()
	hash := sha256.New()
	if _, err := io.Copy(hash, file); err != nil {
		return "", false
	}
	return fmt.Sprintf("%x", hash.Sum(nil)), true
}

func startupProbeIdentity() (string, string, bool) {
	info, ok := debug.ReadBuildInfo()
	if !ok {
		return "", "", false
	}
	sourceCommit, ok := startupProbeSourceCommit(info)
	if !ok {
		return "", "", false
	}
	binarySHA256, ok := startupProbeSelfSHA256()
	if !ok {
		return "", "", false
	}
	return sourceCommit, binarySHA256, true
}

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
	sourceCommit, binarySHA256, identityOK := startupProbeIdentity()
	if !identityOK {
		return "", false
	}
	return startupProbeMarkerWithIdentity(nonce, boundary, sourceCommit, binarySHA256)
}

func startupProbeMarkerWithIdentity(nonce string, boundary startupBoundary, sourceCommit string, binarySHA256 string) (string, bool) {
	if !startupProbeNoncePattern.MatchString(nonce) || !startupProbeSourceCommitPattern.MatchString(sourceCommit) || !startupProbeNoncePattern.MatchString(binarySHA256) {
		return "", false
	}
	switch boundary {
	case startupBoundaryAPIURLInvalid, startupBoundaryAPIURLScheme, startupBoundaryAPIURLUserinfo, startupBoundaryAPIURLQueryOrFragment, startupBoundaryAPITokenTransport, startupBoundaryAPIClientError, startupBoundaryProgramRunError, startupBoundaryProgramRunKilled, startupBoundaryProgramRunPanic, startupBoundaryProgramRunInterrupted:
		return "\x1eSERVICE_LASSO_TUI_STARTUP_BOUNDARY:" + nonce + ":" + string(boundary) + ":" + sourceCommit + ":" + binarySHA256 + "\x1f", true
	default:
		return "", false
	}
}

func reportStartupFailure(writer io.Writer, boundary startupBoundary, err error) {
	nonce := os.Getenv(startupProbeNonceEnvironment)
	if startupProbeNoncePattern.MatchString(nonce) {
		if marker, ok := startupProbeMarker(nonce, boundary); ok {
			// This opt-in marker is consumed in memory by the owned ConPTY helper.
			// It intentionally contains neither the original error nor environment data.
			fmt.Fprint(writer, marker)
		}
		// A valid probe nonce opts out of operator-facing output even when this
		// binary cannot establish its own identity. A raw startup error could
		// otherwise expose a URL, token, or runtime detail to the probe capture.
		return
	}
	fmt.Fprintln(writer, err)
}
