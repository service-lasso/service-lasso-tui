package main

import (
	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/x/term"
	"github.com/service-lasso/service-lasso-tui/internal/lifetime"
	"io"
	"os"
	"strings"
	"testing"
)

func TestConnectFlagWithoutTerminalDoesNotResolveDefaultRuntime(t *testing.T) {
	if term.IsTerminal(os.Stdin.Fd()) || term.IsTerminal(os.Stdout.Fd()) {
		t.Skip("nonterminal contract")
	}
	receiver := lifetime.NewReceiver()
	startup := &startupOccurrence{}
	status := runPublicCommand(startup, receiver, "tui", []string{"--connect"}, nil)
	if status != 2 || startup.Manager != nil || startup.Client != nil || startup.Initial != nil || startup.ResolveFailure == nil {
		t.Fatal("nonterminal connect performed implicit connection")
	}
}

func TestExplicitConnectBypassesDefaultProfilesAndCredentials(t *testing.T) {
	t.Setenv("SERVICE_LASSO_CONNECTIONS_CONFIG", "/does-not-exist-private-profile")
	t.Setenv("SERVICE_LASSO_API_TOKEN", "private-environment-token")
	t.Setenv("SERVICE_LASSO_API_URL", "")
	receiver := lifetime.NewReceiver()
	startup := &startupOccurrence{}
	terminal := func(program *tea.Program) {
		tea.WithInput(strings.NewReader("\x03"))(program)
		tea.WithOutput(io.Discard)(program)
		tea.WithoutRenderer()(program)
	}
	status := runPublicCommand(startup, receiver, "tui", []string{"--connect"}, terminal)
	if status != 0 || startup.Manager != nil || startup.Client != nil || startup.Initial == nil {
		t.Fatalf("forced connect resolved implicit profile or failed: %v", startup.Failure)
	}
	view := startup.Initial.View()
	if !strings.Contains(view, "Edit the runtime URL") || strings.Contains(view, "17883") || strings.Contains(view, "private-environment-token") {
		t.Fatal("connect used default or disclosed credential")
	}
}
