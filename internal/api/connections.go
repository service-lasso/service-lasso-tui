package api

import (
	"encoding/json"
	"fmt"
	"net/url"
	"os"
	"sort"
	"strings"
)

// ConnectionProfile is deliberately metadata-only. tokenEnv names the process
// environment variable which holds the Core local-admin token; the token is
// never written to the connection file or accepted as a command-line value.
type ConnectionProfile struct {
	URL      string   `json:"url"`
	TokenEnv string   `json:"tokenEnv"`
	AuthMode AuthMode `json:"authMode,omitempty"`
	Scopes   []string `json:"scopes,omitempty"`
}

type connectionFile struct {
	Default  string                       `json:"defaultProfile"`
	Profiles map[string]ConnectionProfile `json:"profiles"`
}

// ConnectionOptions applies deterministic launch precedence:
// flags > SERVICE_LASSO_API_* environment > selected profile > loopback default.
// Token values themselves only ever come from a named process environment variable.
type ConnectionOptions struct {
	ConfigPath string
	Profile    string
	APIURL     string
	TokenEnv   string
	Env        func(string) string
}

type ConnectionManager struct {
	profiles map[string]ConnectionProfile
	current  string
	env      func(string) string
}

func ResolveConnections(options ConnectionOptions) (*ConnectionManager, *Client, error) {
	env := options.Env
	if env == nil {
		env = os.Getenv
	}
	path := firstNonEmpty(options.ConfigPath, env("SERVICE_LASSO_CONNECTIONS_CONFIG"))
	profiles := map[string]ConnectionProfile{}
	defaultProfile := ""
	if path != "" {
		contents, err := os.ReadFile(path)
		if err != nil {
			return nil, nil, fmt.Errorf("read connection configuration: %w", err)
		}
		var config connectionFile
		if err := json.Unmarshal(contents, &config); err != nil {
			return nil, nil, fmt.Errorf("decode connection configuration: %w", err)
		}
		if len(config.Profiles) == 0 {
			return nil, nil, fmt.Errorf("connection configuration has no profiles")
		}
		profiles, defaultProfile = config.Profiles, config.Default
	}
	selected := firstNonEmpty(options.Profile, env("SERVICE_LASSO_API_PROFILE"), defaultProfile)
	profile := ConnectionProfile{}
	if selected != "" {
		var ok bool
		profile, ok = profiles[selected]
		if !ok {
			return nil, nil, fmt.Errorf("unknown connection profile %q", selected)
		}
	} else {
		selected = "default"
		profile = profiles[selected]
	}
	apiURL := firstNonEmpty(options.APIURL, env("SERVICE_LASSO_API_URL"), profile.URL, "http://127.0.0.1:17883")
	tokenEnv := firstNonEmpty(options.TokenEnv, env("SERVICE_LASSO_API_TOKEN_ENV"), profile.TokenEnv, "SERVICE_LASSO_API_TOKEN")
	profiles[selected] = ConnectionProfile{URL: apiURL, TokenEnv: tokenEnv, AuthMode: profile.AuthMode, Scopes: profile.Scopes}
	manager := &ConnectionManager{profiles: profiles, current: selected, env: env}
	client, err := manager.Client(selected)
	if err != nil {
		return nil, nil, err
	}
	return manager, client, nil
}

func firstNonEmpty(values ...string) string {
	for _, value := range values {
		if strings.TrimSpace(value) != "" {
			return strings.TrimSpace(value)
		}
	}
	return ""
}

func (m *ConnectionManager) Names() []string {
	names := make([]string, 0, len(m.profiles))
	for name := range m.profiles {
		names = append(names, name)
	}
	sort.Strings(names)
	return names
}

func (m *ConnectionManager) Current() string { return m.current }

func (m *ConnectionManager) Client(name string) (*Client, error) {
	profile, ok := m.profiles[name]
	if !ok {
		return nil, fmt.Errorf("unknown connection profile %q", name)
	}
	if profile.TokenEnv == "" {
		return nil, fmt.Errorf("connection profile %q has no credential environment reference", name)
	}
	token := m.env(profile.TokenEnv)
	if token == "" {
		return nil, fmt.Errorf("connection profile %q credential is unavailable", name)
	}
	mode := profile.AuthMode
	if mode == "" {
		mode = AuthModeLocalAdmin
	}
	if mode == AuthModeOAuthBearer && !isLoopbackURL(profile.URL) && !hasRequiredScopes(profile.Scopes) {
		return nil, fmt.Errorf("oauth-bearer profile %q lacks required lifecycle scopes", name)
	}
	return NewClientWithAuth(profile.URL, nil, token, mode)
}

func isLoopbackURL(raw string) bool {
	client, err := NewClient(raw, nil, "")
	if err != nil {
		return false
	}
	parsed, _ := url.Parse(client.baseURL)
	return isLoopbackHost(parsed.Hostname())
}

func hasRequiredScopes(scopes []string) bool {
	needed := map[string]bool{"service-lasso:read": false, "service-lasso:lifecycle:write": false}
	for _, scope := range scopes {
		if _, ok := needed[scope]; ok {
			needed[scope] = true
		}
	}
	return needed["service-lasso:read"] && needed["service-lasso:lifecycle:write"]
}

func (m *ConnectionManager) Switch(name string) (*Client, error) {
	client, err := m.Client(name)
	if err != nil {
		return nil, err
	}
	m.current = name
	return client, nil
}
