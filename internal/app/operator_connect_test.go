package app

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/x/ansi"
	"github.com/service-lasso/service-lasso-tui/internal/api"
	"github.com/service-lasso/service-lasso-tui/internal/lifetime"
)

func connectKey(m connectionModel, key tea.KeyMsg) (connectionModel, tea.Cmd) {
	next, cmd := m.Update(key)
	return next.(connectionModel), cmd
}
func TestConnectConfirmsTargetMasksCredentialAndAuthenticates(t *testing.T) {
	t.Setenv("NO_COLOR", "1")
	calls := 0
	bad := true
	_, discovery, _ := terminalRefreshFixture(t)
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		calls++
		if r.Header.Get("x-service-lasso-admin-token") != "secret-session-token" {
			t.Error("wrong credential header")
		}
		if bad {
			w.WriteHeader(401)
			return
		}
		if strings.HasSuffix(r.URL.Path, "/reconciliation-context") {
			json.NewEncoder(w).Encode(struct {
				ContractVersion string              `json:"contractVersion"`
				Context         api.OperatorContext `json:"context"`
			}{"service-lasso-durable-reconciliation-context.v1", selectedOperatorTestContext("a")})
			return
		}
		var data any = discovery
		raw, _ := json.Marshal(data)
		fmt.Fprintf(w, `{"contractVersion":"%s","requestId":"request","generatedAt":"2026-11-27T10:15:00.000Z","data":%s}`, api.OperatorContract, raw)
	}))
	defer server.Close()
	receiver := lifetime.NewReceiver()
	ctx := lifetime.WithReceiver(context.Background(), receiver)
	m := NewOperatorConnect(ctx, receiver, server.URL, api.AuthModeLocalAdmin).(connectionModel)
	for _, size := range [][2]int{{120, 32}, {80, 24}} {
		m.width, m.height = size[0], size[1]
		view := m.View()
		if !strings.Contains(view, server.URL) || !strings.Contains(view, "Edit the runtime URL") || calls != 0 {
			t.Fatal("target was not shown before network/secret")
		}
		if len(strings.Split(view, "\n")) != size[1] {
			t.Fatal("height")
		}
		for _, line := range strings.Split(view, "\n") {
			if ansi.StringWidth(line) > size[0] {
				t.Fatal("overflow")
			}
		}
	}
	m, _ = connectKey(m, tea.KeyMsg{Type: tea.KeyEnter})
	if m.step != "confirm" || calls != 0 {
		t.Fatal("confirmation skipped")
	}
	m, _ = connectKey(m, tea.KeyMsg{Type: tea.KeyEnter})
	if m.step != "credential" {
		t.Fatal("no credential step")
	}
	m, _ = connectKey(m, tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune("secret-session-token")})
	if strings.Contains(m.View(), "secret-session-token") || !strings.Contains(m.View(), "•••") {
		t.Fatal("credential not masked")
	}
	m, command := connectKey(m, tea.KeyMsg{Type: tea.KeyEnter})
	if m.credential != "" || command == nil {
		t.Fatal("credential retained in editor")
	}
	message := command()
	next, _ := m.Update(message)
	m = next.(connectionModel)
	if m.child != nil || m.step != "credential" || !strings.Contains(m.View(), "Authentication rejected") || strings.Contains(m.View(), "secret-session-token") {
		t.Fatal("failure did not allow safe retry")
	}
	bad = false
	m, _ = connectKey(m, tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune("secret-session-token")})
	m, command = connectKey(m, tea.KeyMsg{Type: tea.KeyEnter})
	message = command()
	if result := message.(operatorConnectionResult); result.err != nil {
		t.Fatalf("real contract read failed: %v", result.err)
	}
	next, command = m.Update(message)
	m = next.(connectionModel)
	if m.child == nil || command == nil {
		t.Fatal("verified connection not handed to real operator bootstrap")
	}
}

func TestConnectRejectsUnconfirmedInsecureTargetsAndClearsSecret(t *testing.T) {
	for _, target := range []string{"http://remote.example", "https://user:password@example.com", "https://example.com?token=secret", "file:///tmp/runtime"} {
		m := NewOperatorConnect(context.Background(), nil, target, api.AuthModeLocalAdmin).(connectionModel)
		m, command := connectKey(m, tea.KeyMsg{Type: tea.KeyEnter})
		if m.step != "target" || m.errorText == "" || command != nil {
			t.Fatal("unsafe target admitted")
		}
	}
	m := NewOperatorConnect(context.Background(), nil, "https://remote.example", api.AuthModeLocalAdmin).(connectionModel)
	m, _ = connectKey(m, tea.KeyMsg{Type: tea.KeyTab})
	m, _ = connectKey(m, tea.KeyMsg{Type: tea.KeyEnter})
	if m.step != "confirm" {
		t.Fatal("explicit OAuth HTTPS target rejected")
	}
	m, _ = connectKey(m, tea.KeyMsg{Type: tea.KeyEnter})
	m, _ = connectKey(m, tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune("private-token")})
	m, _ = connectKey(m, tea.KeyMsg{Type: tea.KeyEsc})
	if m.step != "target" || m.credential != "" || strings.Contains(m.View(), "private-token") {
		t.Fatal("escape retained credential")
	}
}

func TestConnectDoesNotEchoCredentialsInTargetURL(t *testing.T) {
	m := NewOperatorConnect(context.Background(), nil, "https://user:private-password@example.com?token=private-query#private-fragment", api.AuthModeOAuthBearer).(connectionModel)
	view := m.View()
	for _, secret := range []string{"private-password", "private-query", "private-fragment"} {
		if strings.Contains(view, secret) {
			t.Fatal("URL credential echoed")
		}
	}
}
