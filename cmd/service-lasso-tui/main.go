package main

import (
	"context"
	"flag"
	"os"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/service-lasso/service-lasso-tui/internal/api"
	"github.com/service-lasso/service-lasso-tui/internal/app"
)

func main() {
	apiURL := flag.String("api", "", "Service Lasso runtime API base URL")
	configPath := flag.String("connections", "", "connection profile configuration file")
	profile := flag.String("profile", "", "configured connection profile")
	tokenEnv := flag.String("token-env", "", "credential environment variable name")
	flag.Parse()

	manager, client, err := api.ResolveConnections(api.ConnectionOptions{ConfigPath: *configPath, Profile: *profile, APIURL: *apiURL, TokenEnv: *tokenEnv})
	if err != nil {
		reportStartupFailure(os.Stderr, startupBoundaryForAPIError(err), err)
		os.Exit(2)
	}

	model := app.NewWithConnections(client, manager, context.Background())
	program := tea.NewProgram(model, tea.WithAltScreen())
	if _, err := program.Run(); err != nil {
		reportStartupFailure(os.Stderr, startupBoundaryForProgramError(err), err)
		os.Exit(1)
	}
}
