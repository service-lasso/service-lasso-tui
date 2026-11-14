package app

import (
	"context"
	"io"
	"strings"
	"testing"
	"time"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/service-lasso/service-lasso-tui/internal/api"
)

type legacyEscapeEvent struct {
	operation, terminal, frozen bool
	key                         string
	pending, cancelled          bool
	searching, detail           bool
	selected                    string
	command                     bool
}
type legacyEscapeFixture struct {
	expectedPreview                 *api.LifecyclePreview
	expectedAction, expectedService string
	state                           model
	events                          chan legacyEscapeEvent
}

func (f *legacyEscapeFixture) Init() tea.Cmd { return nil }
func (f *legacyEscapeFixture) View() string  { return f.state.View() }
func (f *legacyEscapeFixture) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	next, command := f.state.Update(msg)
	f.state = next.(model)
	if key, ok := msg.(tea.KeyMsg); ok {
		f.events <- legacyEscapeEvent{operation: f.state.operation != nil, frozen: f.state.pendingPreview == f.expectedPreview && f.state.pendingAction == f.expectedAction && f.state.pendingServiceID == f.expectedService, key: key.String(), pending: f.state.pendingAction != "", cancelled: strings.Contains(f.state.View(), "Confirmation cancelled."), searching: f.state.searching, detail: f.state.screen == detailScreen, selected: f.state.selectedServiceID, command: command != nil}
	}
	if _, ok := msg.(operationMsg); ok {
		f.events <- legacyEscapeEvent{key: "operation-result", operation: f.state.operation != nil, terminal: strings.Contains(f.state.lastResult, "succeeded.")}
	}
	return f, command
}

func TestLegacyEscapeAcknowledgementSeparatesQuitInActualInputDecoder(t *testing.T) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()
	reader, writer := io.Pipe()
	defer reader.Close()
	defer writer.Close()
	m := New(durableFake{}, context.Background()).(model)
	m.pendingAction = "install"
	m.pendingPreview = currentPreview()
	m.preparingAction = false
	m.loading = false
	fixture := &legacyEscapeFixture{state: m, events: make(chan legacyEscapeEvent, 4), expectedPreview: m.pendingPreview, expectedAction: m.pendingAction, expectedService: m.pendingServiceID}
	program := tea.NewProgram(fixture, tea.WithInput(reader), tea.WithOutput(io.Discard), tea.WithoutRenderer(), tea.WithoutSignalHandler(), tea.WithContext(ctx))
	done := make(chan error, 1)
	go func() { _, err := program.Run(); done <- err }()
	receive := func() legacyEscapeEvent {
		t.Helper()
		select {
		case event := <-fixture.events:
			return event
		case <-ctx.Done():
			t.Fatal("actual input decoder did not acknowledge key")
			return legacyEscapeEvent{}
		}
	}
	// Printable macro input is one rune message; it must not confirm.
	if _, err := writer.Write([]byte("yr")); err != nil {
		t.Fatal(err)
	}
	if event := receive(); event.key != "yr" || !event.pending || event.command {
		t.Fatal("batched printable input unexpectedly confirmed")
	}
	if _, err := writer.Write([]byte("r\rj\r?\rq\r")); err != nil {
		t.Fatal(err)
	}
	for _, key := range []string{"r", "enter", "j", "enter", "?", "enter", "q", "enter"} {
		if event := receive(); event.key != key || !event.pending || event.command || !event.frozen || event.operation {
			t.Fatal("delimited ignored key changed the frozen confirmation")
		}
	}
	// Quit alone is deliberately ignored by a pending confirmation.
	if _, err := writer.Write([]byte("q")); err != nil {
		t.Fatal(err)
	}
	if event := receive(); event.key != "q" || !event.pending {
		t.Fatal("pending confirmation did not ignore ordinary quit")
	}
	// Wait for the actual decoder/model acknowledgement before writing q. Adjacent
	// Escape+q bytes could instead be decoded as Alt+q.
	if _, err := writer.Write([]byte{27}); err != nil {
		t.Fatal(err)
	}
	if event := receive(); event.key != "esc" || event.pending || !event.cancelled {
		t.Fatal("Escape did not cancel the unclaimed confirmation")
	}
	if _, err := writer.Write([]byte("q")); err != nil {
		t.Fatal(err)
	}
	if event := receive(); event.key != "q" || event.pending {
		t.Fatal("normal quit was not decoded separately after Escape acknowledgement")
	}
	// Retire the fixture-owned pipe reader after the final decoded input.
	if err := writer.Close(); err != nil {
		t.Fatal(err)
	}
	select {
	case err := <-done:
		if err != nil {
			t.Fatal(err)
		}
	case <-ctx.Done():
		t.Fatal("normal quit did not complete after Escape acknowledgement")
	}
	if fixture.state.operation != nil || fixture.state.pendingPreview != nil {
		t.Fatal("keyboard recovery created an operation or retained a confirmation")
	}
}

func TestLegacySearchTransportUsesStandaloneSlashAndOrderedEnterKeys(t *testing.T) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()
	reader, writer := io.Pipe()
	defer reader.Close()
	defer writer.Close()
	client := &recordingDurableClient{}
	m := New(client, context.Background()).(model)
	m.services = []api.Service{{ID: "other", Name: "Other"}, {ID: "echo", Name: "Echo"}}
	m.selectedServiceID = "other"
	m.screen = servicesScreen
	m.loading = false
	fixture := &legacyEscapeFixture{state: m, events: make(chan legacyEscapeEvent, 8)}
	program := tea.NewProgram(fixture, tea.WithInput(reader), tea.WithOutput(io.Discard), tea.WithoutRenderer(), tea.WithoutSignalHandler(), tea.WithContext(ctx))
	done := make(chan error, 1)
	go func() { _, err := program.Run(); done <- err }()
	receive := func() legacyEscapeEvent {
		t.Helper()
		select {
		case event := <-fixture.events:
			return event
		case <-ctx.Done():
			t.Fatal("search input was not decoded")
			return legacyEscapeEvent{}
		}
	}
	if _, err := writer.Write([]byte("/echo")); err != nil {
		t.Fatal(err)
	}
	if event := receive(); event.key != "/echo" || event.searching {
		t.Fatal("batched slash unexpectedly entered search")
	}
	if _, err := writer.Write([]byte("/")); err != nil {
		t.Fatal(err)
	}
	if event := receive(); event.key != "/" || !event.searching {
		t.Fatal("standalone slash did not enter search")
	}
	// Control characters delimit individual Enter keys even in the same read.
	if _, err := writer.Write([]byte("echo\r\r")); err != nil {
		t.Fatal(err)
	}
	if event := receive(); event.key != "echo" || !event.searching || event.selected != "echo" {
		t.Fatal("query did not select its filtered service")
	}
	if event := receive(); event.key != "enter" || event.searching || event.detail {
		t.Fatal("first Enter did not leave search")
	}
	if event := receive(); event.key != "enter" || !event.detail || event.selected != "echo" {
		t.Fatal("second Enter did not open the selected service")
	}
	if _, err := writer.Write([]byte("q")); err != nil {
		t.Fatal(err)
	}
	if event := receive(); event.key != "q" || event.pending {
		t.Fatal("search created a confirmation")
	}
	if err := writer.Close(); err != nil {
		t.Fatal(err)
	}
	select {
	case err := <-done:
		if err != nil {
			t.Fatal(err)
		}
	case <-ctx.Done():
		t.Fatal("search fixture did not quit")
	}
	_, previews, submits, _, cancels := client.calls()
	if previews != 0 || submits != 0 || cancels != 0 || fixture.state.operation != nil {
		t.Fatal("search transport caused a mutation")
	}
}

func TestLegacyStandaloneConfirmationIsDecodedAsOneSubmission(t *testing.T) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()
	reader, writer := io.Pipe()
	defer reader.Close()
	defer writer.Close()
	op := currentOperation()
	op.Status = "succeeded"
	op.Outcome = "succeeded"
	op.Phase = "complete"
	op.Progress = 100
	client := &recordingDurableClient{operation: op}
	m := preparedModel(t, client)
	m.loading = false
	fixture := &legacyEscapeFixture{state: m, events: make(chan legacyEscapeEvent, 8), expectedPreview: m.pendingPreview, expectedAction: m.pendingAction, expectedService: m.pendingServiceID}
	program := tea.NewProgram(fixture, tea.WithInput(reader), tea.WithOutput(io.Discard), tea.WithoutRenderer(), tea.WithoutSignalHandler(), tea.WithContext(ctx))
	done := make(chan error, 1)
	go func() { _, err := program.Run(); done <- err }()
	receive := func() legacyEscapeEvent {
		t.Helper()
		select {
		case event := <-fixture.events:
			return event
		case <-ctx.Done():
			t.Fatal("confirmation input was not acknowledged")
			return legacyEscapeEvent{}
		}
	}
	if _, err := writer.Write([]byte("yr")); err != nil {
		t.Fatal(err)
	}
	if event := receive(); event.key != "yr" || !event.frozen || event.command || event.operation {
		t.Fatal("batched printable input reached submission")
	}
	_, _, before, _, _ := client.calls()
	if before != 0 {
		t.Fatal("operation submitted before standalone confirmation")
	}
	if _, err := writer.Write([]byte("y")); err != nil {
		t.Fatal(err)
	}
	if event := receive(); event.key != "y" || event.pending || !event.command {
		t.Fatal("standalone confirmation was not decoded")
	}
	if event := receive(); event.key != "operation-result" || event.operation || !event.terminal {
		t.Fatal("controlled operation did not complete")
	}
	if _, err := writer.Write([]byte("q")); err != nil {
		t.Fatal(err)
	}
	if event := receive(); event.key != "q" || event.pending {
		t.Fatal("normal quit was not decoded")
	}
	if err := writer.Close(); err != nil {
		t.Fatal(err)
	}
	select {
	case err := <-done:
		if err != nil {
			t.Fatal(err)
		}
	case <-ctx.Done():
		t.Fatal("confirmation fixture did not quit")
	}
	_, previews, submits, _, cancels := client.calls()
	if submits != 1 || previews != 0 || cancels != 0 {
		t.Fatal("standalone confirmation did not make exactly one controlled submission")
	}
}
