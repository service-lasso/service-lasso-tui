package main

import (
	"context"
	"errors"
	"flag"
	"github.com/charmbracelet/x/term"
	"os"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/service-lasso/service-lasso-tui/internal/api"
	"github.com/service-lasso/service-lasso-tui/internal/app"
	"github.com/service-lasso/service-lasso-tui/internal/lifetime"
)

type startupOccurrence struct {
	Original                                          lifetime.Original
	Receiver                                          *lifetime.Receiver
	PublicZero                                        string
	PublicTail                                        []string
	Terminal                                          tea.ProgramOption
	Context                                           context.Context
	Flags                                             *flag.FlagSet
	Manager                                           *api.ConnectionManager
	Client                                            *api.Client
	Initial, Final                                    tea.Model
	ParseFailure, ResolveFailure, RunFailure, Failure error
	Diagnostic                                        startupDiagnosticObservation
	ParserDiagnostic                                  parserDiagnosticWriter
	Status                                            int
	Attempted, Returned                               bool
}

// This actual command operation returns its outcome to its owning caller.
// Native LOCAL custody/disposition must eventually consume the SAME occurrence
// before the outer exit; returning a status here does not authorize that exit.
func runPublicCommand(startup *startupOccurrence, receiver *lifetime.Receiver, publicZero string, publicTail []string, terminal tea.ProgramOption) int {
	if startup == nil || startup.Attempted {
		return 2
	}
	startup.Attempted = true
	startup.Receiver = receiver
	startup.PublicZero = publicZero
	startup.PublicTail = publicTail
	startup.Terminal = terminal
	if err := lifetime.Require(receiver); err != nil {
		startup.Failure = err
		startup.Status = 2
		startup.Returned = true
		return startup.Status
	}
	if err := receiver.Enroll(&startup.Original, "main.startup", startup); err != nil {
		startup.Failure = err
		startup.Status = 2
		startup.Returned = true
		return startup.Status
	}
	startup.Flags = flag.NewFlagSet(publicZero, flag.ContinueOnError)
	if startup.ParserDiagnostic.Writer == nil {
		startup.ParserDiagnostic.Writer = os.Stderr
	}
	startup.Flags.SetOutput(&startup.ParserDiagnostic)
	startup.Context = lifetime.WithReceiver(context.Background(), receiver)
	connect := startup.Flags.Bool("connect", false, "confirm a runtime target and log in interactively")
	apiURL := startup.Flags.String("api", "", "Service Lasso runtime API base URL")
	configPath := startup.Flags.String("connections", "", "connection profile configuration file")
	profile := startup.Flags.String("profile", "", "configured connection profile")
	tokenEnv := startup.Flags.String("token-env", "", "credential environment variable name")
	startup.ParseFailure = startup.Flags.Parse(publicTail)
	if startup.ParseFailure != nil {
		startup.Failure = startup.ParseFailure
		startup.Status = 2
		startup.Returned = true
		return startup.Status
	}
	var model tea.Model
	interactive := terminal != nil || term.IsTerminal(os.Stdin.Fd()) && term.IsTerminal(os.Stdout.Fd())
	if *connect {
		if !interactive {
			startup.ResolveFailure = &api.OperatorError{Stage: "configuration"}
		} else {
			// An explicit connection screen deliberately starts without an implicit
			// default runtime; --api or the Core-selected workspace URL can prefill it.
			target := *apiURL
			if target == "" {
				target = os.Getenv("SERVICE_LASSO_API_URL")
			}
			model = app.NewOperatorConnect(startup.Context, receiver, target, api.AuthModeLocalAdmin)
		}
	} else {
		startup.Manager, startup.Client, startup.ResolveFailure = api.ResolveOperatorConnections(api.ConnectionOptions{
			Receiver: receiver, ConfigPath: *configPath, Profile: *profile, APIURL: *apiURL, TokenEnv: *tokenEnv,
		})
		if startup.ResolveFailure == nil {
			if interactive {
				model = app.NewOperatorWithLogin(startup.Client, startup.Manager, startup.Context, receiver)
			} else {
				model = app.NewOperatorWithConnections(startup.Client, startup.Manager, startup.Context)
			}
		} else {
			var configured *api.ConfigurationError
			if interactive && startup.Manager != nil && errors.As(startup.ResolveFailure, &configured) && configured.Kind == api.ConfigurationErrorMissingCredential {
				selected := startup.Manager.CurrentProfile()
				model = app.NewOperatorConnect(startup.Context, receiver, selected.URL, selected.AuthMode)
				startup.ResolveFailure = nil
			}
		}
	}
	if startup.ResolveFailure != nil {
		startup.Failure = startup.ResolveFailure
		reportStartupFailureObserved(&startup.Diagnostic, os.Stderr, startupBoundaryForAPIError(startup.ResolveFailure), startup.ResolveFailure)
		startup.Status = 2
		startup.Returned = true
		return startup.Status
	}

	startup.Initial = model
	if terminal == nil {
		startup.Final, startup.RunFailure = app.RunOwned(model, receiver, tea.WithAltScreen())
	} else {
		startup.Final, startup.RunFailure = app.RunOwned(model, receiver, tea.WithAltScreen(), terminal)
	}
	if startup.RunFailure != nil {
		startup.Failure = startup.RunFailure
		reportStartupFailureObserved(&startup.Diagnostic, os.Stderr, startupBoundaryForProgramError(startup.RunFailure), startup.RunFailure)
		startup.Status = 1
		startup.Returned = true
		return startup.Status
	}
	startup.Status = 0
	startup.Returned = true
	return startup.Status
}
