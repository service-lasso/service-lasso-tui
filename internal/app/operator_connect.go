package app

import (
	"context"
	"errors"
	"net/http"
	"net/url"
	"strings"
	"time"
	"unicode"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/service-lasso/service-lasso-tui/internal/api"
	"github.com/service-lasso/service-lasso-tui/internal/lifetime"
)

type operatorConnectionResult struct {
	client  *api.Client
	manager *api.ConnectionManager
	err     error
}

// connectionModel admits a confirmed target before collecting its credential.
// It remains around the operator bootstrap so rejected credentials can be retried.
type connectionModel struct {
	ctx                           context.Context
	receiver                      *lifetime.Receiver
	child                         tea.Model
	target, credential, errorText string
	mode                          api.AuthMode
	step                          string
	width, height                 int
}

func NewOperatorConnect(ctx context.Context, receiver *lifetime.Receiver, target string, mode api.AuthMode) tea.Model {
	if mode == "" {
		mode = api.AuthModeLocalAdmin
	}
	return connectionModel{ctx: ctx, receiver: receiver, target: target, mode: mode, step: "target", width: 80, height: 24}
}

func NewOperatorWithLogin(client *api.Client, manager *api.ConnectionManager, ctx context.Context, receiver *lifetime.Receiver) tea.Model {
	profile := manager.CurrentProfile()
	m := NewOperatorConnect(ctx, receiver, profile.URL, profile.AuthMode).(connectionModel)
	m.child = NewOperatorWithConnections(client, manager, ctx)
	return m
}
func (m connectionModel) Init() tea.Cmd {
	if m.child != nil {
		return m.child.Init()
	}
	return nil
}
func connectionAuthFailure(err error) bool {
	var public *api.OperatorPublicError
	if errors.As(err, &public) {
		return public.Code == "authentication_required" || public.Code == "permission_denied"
	}
	var transport *api.OperatorError
	return errors.As(err, &transport) && (transport.Status == http.StatusUnauthorized || transport.Status == http.StatusForbidden)
}
func (m connectionModel) Update(message tea.Msg) (tea.Model, tea.Cmd) {
	if size, ok := message.(tea.WindowSizeMsg); ok {
		m.width, m.height = size.Width, size.Height
	}
	if m.child != nil {
		if boot, ok := message.(operatorBooted); ok && connectionAuthFailure(boot.Err) {
			m.child = nil
			m.step = "target"
			m.errorText = "Authentication rejected. Confirm the target and retry."
			m.credential = ""
			return m, nil
		}
		var command tea.Cmd
		m.child, command = m.child.Update(message)
		return m, command
	}
	switch msg := message.(type) {
	case operatorConnectionResult:
		if m.step != "checking" {
			return m, nil
		}
		if msg.err != nil {
			m.step = "credential"
			m.credential = ""
			m.errorText = "Connection failed. Check the target and credential, then retry."
			if connectionAuthFailure(msg.err) {
				m.errorText = "Authentication rejected. Check your credential and retry."
			}
			return m, nil
		}
		m.child = NewOperatorWithConnections(msg.client, msg.manager, m.ctx)
		m.child, _ = m.child.Update(tea.WindowSizeMsg{Width: m.width, Height: m.height})
		return m, m.child.Init()
	case tea.KeyMsg:
		switch msg.String() {
		case "ctrl+c":
			m.credential = ""
			return m, tea.Quit
		case "esc":
			if m.step == "checking" {
				return m, nil
			}
			m.step = "target"
			m.credential = ""
			m.errorText = ""
			return m, nil
		}
		if m.step == "checking" {
			return m, nil
		}
		switch msg.String() {
		case "tab":
			if m.step == "target" {
				if m.mode == api.AuthModeLocalAdmin {
					m.mode = api.AuthModeOAuthBearer
				} else {
					m.mode = api.AuthModeLocalAdmin
				}
			}
			return m, nil
		case "enter":
			switch m.step {
			case "target":
				m.target = strings.TrimSpace(m.target)
				if err := api.ValidateInteractiveOperatorTarget(m.target, m.mode); err != nil {
					m.errorText = api.SafeError(err)
					return m, nil
				}
				m.step = "confirm"
				m.errorText = ""
			case "confirm":
				m.step = "credential"
				m.errorText = ""
			case "credential":
				if m.credential == "" {
					m.errorText = "Enter a credential to authenticate."
					return m, nil
				}
				target, token, mode, ctx, receiver := m.target, m.credential, m.mode, m.ctx, m.receiver
				m.credential = ""
				m.step = "checking"
				m.errorText = ""
				return m, func() tea.Msg {
					manager, client, err := api.InteractiveOperatorConnection(receiver, target, token, mode)
					if err == nil {
						read, cancel := context.WithTimeout(ctx, 8*time.Second)
						defer cancel()
						_, err = client.OperatorDiscovery(read)
						if err == nil {
							_, err = client.OperatorContext(read)
						}
					}
					if err != nil {
						return operatorConnectionResult{err: err}
					}
					return operatorConnectionResult{client: client, manager: manager}
				}
			}
			return m, nil
		case "backspace", "ctrl+h":
			if m.step == "target" {
				r := []rune(m.target)
				if len(r) > 0 {
					m.target = string(r[:len(r)-1])
				}
			}
			if m.step == "credential" {
				r := []rune(m.credential)
				if len(r) > 0 {
					m.credential = string(r[:len(r)-1])
				}
			}
			return m, nil
		case "ctrl+u":
			if m.step == "target" {
				m.target = ""
			}
			if m.step == "credential" {
				m.credential = ""
			}
			return m, nil
		}
		if msg.Type == tea.KeyRunes && (m.step == "target" || m.step == "credential") {
			for _, r := range msg.Runes {
				if unicode.IsControl(r) {
					continue
				}
				if m.step == "target" && len(m.target) < 2048 {
					m.target += string(r)
				}
				if m.step == "credential" && len(m.credential) < 4096 {
					m.credential += string(r)
				}
			}
		}
	}
	return m, nil
}

func (m connectionModel) View() string {
	if m.child != nil {
		return m.child.View()
	}
	w, h := max(1, m.width), max(1, m.height)
	inner := max(1, w-4)
	// Unsafe unconfirmed input is displayed through operatorText; credentials are
	// never rendered, including failed authentication and error paths.
	mode := "Local admin · loopback only"
	if m.mode == api.AuthModeOAuthBearer {
		mode = "OAuth bearer · remote HTTPS · read admission"
	}
	lines := []string{"", " Service Lasso  ·  Connect", "", " Runtime target", " " + connectionDisplayTarget(m.target), "", " Authentication: " + mode, ""}
	switch m.step {
	case "target":
		lines = append(lines, " Edit the runtime URL, then press Enter.", " Tab changes authentication · Ctrl+U clears the URL.")
	case "confirm":
		lines = append(lines, " Confirm connection to this exact runtime.", " Enter confirms target · Esc edits target.")
	case "credential":
		mask := strings.Repeat("•", min(32, len([]rune(m.credential))))
		lines = append(lines, " Credential: "+mask, " Enter authenticates · Esc changes target.")
	case "checking":
		lines = append(lines, " Verifying runtime authentication…", " Waiting for real operator/v2 discovery and context.")
	}
	lines = append(lines, "", " "+m.errorText, "", " Credentials stay in memory for this session.")
	for len(lines) < h-1 {
		lines = append(lines, "")
	}
	lines = append(lines, " Ctrl+C Quit · Esc Back")
	if len(lines) > h {
		lines = lines[:h]
	}
	for i, line := range lines {
		style := operatorBaseStyle
		if i == 1 {
			style = operatorBrandStyle
		}
		if line == " "+m.errorText && m.errorText != "" {
			style = style.Foreground(operatorCoral)
		}
		lines[i] = operatorRender(operatorCanvasStyle, "  ") + operatorRender(style, padLine(line, inner)) + operatorRender(operatorCanvasStyle, strings.Repeat(" ", max(0, w-inner-2)))
	}
	return strings.Join(lines, "\n")
}

// URL userinfo/query/fragment are rejected at admission and hidden even while
// editing, so pasting a credential-bearing URL cannot echo the secret.
func connectionDisplayTarget(target string) string {
	parsed, err := url.Parse(target)
	if err != nil {
		return "Invalid URL · Ctrl+U clears target"
	}
	if parsed.User != nil {
		parsed.User = url.User("redacted")
	}
	if parsed.RawQuery != "" {
		parsed.RawQuery = "redacted"
	}
	if parsed.Fragment != "" {
		parsed.Fragment = "redacted"
	}
	return operatorText(parsed.String())
}
