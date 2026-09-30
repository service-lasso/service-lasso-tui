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
	defaultURL := os.Getenv("SERVICE_LASSO_API_URL")
	if defaultURL == "" {
		defaultURL = "http://127.0.0.1:17883"
	}

	apiURL := flag.String("api", defaultURL, "Service Lasso runtime API base URL")
	flag.Parse()

	client, err := api.NewClient(*apiURL, nil, os.Getenv("SERVICE_LASSO_API_TOKEN"))
	if err != nil {
		reportStartupFailure(os.Stderr, startupBoundaryAPIClientError, err)
		os.Exit(2)
	}

	model := app.New(client, context.Background())
	program := tea.NewProgram(model, tea.WithAltScreen())
	if _, err := program.Run(); err != nil {
		reportStartupFailure(os.Stderr, startupBoundaryForProgramError(err), err)
		os.Exit(1)
	}
}
