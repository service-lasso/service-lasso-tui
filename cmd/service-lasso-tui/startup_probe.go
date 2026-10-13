package main

import (
	"crypto/sha256"
	"errors"
	"fmt"
 "hash"
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

type startupIdentityObservation struct {
 Info *debug.BuildInfo
 InfoRead bool
 Path string
 File *os.File
 Hash hash.Hash
 PathError,OpenError,ReadError,CloseError error
 ReadBytes int64
 CloseAttempted bool
 SourceCommit,BinarySHA256 string
}
func startupProbeSelfSHA256()(string,bool){return startupProbeSelfSHA256Observed(&startupIdentityObservation{})}
func startupProbeSelfSHA256Observed(observation *startupIdentityObservation)(string,bool){
 observation.Path,observation.PathError=os.Executable();if observation.PathError!=nil{return "",false}
 observation.File,observation.OpenError=os.Open(observation.Path);if observation.OpenError!=nil{return "",false}
 observation.Hash=sha256.New();observation.ReadBytes,observation.ReadError=io.Copy(observation.Hash,observation.File)
 observation.CloseAttempted=true;observation.CloseError=observation.File.Close()
 if observation.ReadError!=nil||observation.CloseError!=nil{return "",false}
 observation.BinarySHA256=fmt.Sprintf("%x",observation.Hash.Sum(nil));return observation.BinarySHA256,true
}
func startupProbeIdentity()(string,string,bool){return startupProbeIdentityObserved(&startupIdentityObservation{})}
func startupProbeIdentityObserved(observation *startupIdentityObservation)(string,string,bool){
 observation.Info,observation.InfoRead=debug.ReadBuildInfo();if !observation.InfoRead{return "","",false}
 var ok bool;observation.SourceCommit,ok=startupProbeSourceCommit(observation.Info);if !ok{return "","",false}
 _,ok=startupProbeSelfSHA256Observed(observation);if !ok{return "","",false}
 return observation.SourceCommit,observation.BinarySHA256,true
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

// The actual main embeds this cell before startup/Run. Late diagnostic writes
// therefore use an already owned source cell rather than enrolling after Join.
type startupDiagnosticObservation struct {
 Writer io.Writer
 Boundary startupBoundary
 Original error
 Nonce,Output string
 Identity startupIdentityObservation
 WriteAttempted bool
 Written int
 WriteError error
}
func reportStartupFailure(writer io.Writer,boundary startupBoundary,err error){reportStartupFailureObserved(&startupDiagnosticObservation{},writer,boundary,err)}
func reportStartupFailureObserved(observation *startupDiagnosticObservation,writer io.Writer,boundary startupBoundary,err error){
 observation.Writer=writer;observation.Boundary=boundary;observation.Original=err
 observation.Nonce=os.Getenv(startupProbeNonceEnvironment)
 if startupProbeNoncePattern.MatchString(observation.Nonce){
  source,digest,ok:=startupProbeIdentityObserved(&observation.Identity)
  if ok{if marker,valid:=startupProbeMarkerWithIdentity(observation.Nonce,boundary,source,digest);valid{
   observation.Output=marker;observation.WriteAttempted=true;observation.Written,observation.WriteError=fmt.Fprint(writer,marker)
  }}
  return
 }
 observation.Output=api.SafeError(err);observation.WriteAttempted=true
 observation.Written,observation.WriteError=fmt.Fprintln(writer,observation.Output)
}