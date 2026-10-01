package api

import (
	"net/http"
	"net/http/httptest"
	"net/url"
	"os"
	"path/filepath"
	"strings"
	"sync/atomic"
	"testing"
)

func TestResolveConnectionsUsesFlagEnvAndProfilePrecedenceWithoutSavingSecrets(t *testing.T) {
	dir := t.TempDir()
	path := filepath.Join(dir, "connections.json")
	const sentinel = "CONNECTION_SECRET_SENTINEL"
	contents := `{"defaultProfile":"remote","profiles":{"remote":{"url":"https://profile.example.test","tokenEnv":"PROFILE_TOKEN","authMode":"oauth-bearer","scopes":["service-lasso:read","service-lasso:lifecycle:write"]}}}`
	if err := os.WriteFile(path, []byte(contents), 0o600); err != nil {
		t.Fatal(err)
	}
	env := map[string]string{"SERVICE_LASSO_API_URL": "https://env.example.test", "SERVICE_LASSO_API_TOKEN_ENV": "ENV_TOKEN", "ENV_TOKEN": sentinel, "PROFILE_TOKEN": "wrong"}
	manager, client, err := ResolveConnections(ConnectionOptions{ConfigPath: path, APIURL: "https://flag.example.test", Env: func(key string) string { return env[key] }})
	if err != nil {
		t.Fatal(err)
	}
	if manager.Current() != "remote" || client.baseURL != "https://flag.example.test" || client.operatorToken != sentinel {
		t.Fatalf("wrong resolved connection: %#v %#v", manager, client)
	}
	if strings.Contains(contents, sentinel) {
		t.Fatal("test fixture saved a credential")
	}
}

func TestConnectionManagerSelectsNamedProfileAndRejectsMissingCredential(t *testing.T) {
	env := map[string]string{"LOCAL_TOKEN": "local", "REMOTE_TOKEN": "remote"}
	manager := &ConnectionManager{profiles: map[string]ConnectionProfile{"local": {URL: "http://127.0.0.1:17883", TokenEnv: "LOCAL_TOKEN"}, "remote": {URL: "https://remote.example.test", TokenEnv: "REMOTE_TOKEN", AuthMode: AuthModeOAuthBearer, Scopes: []string{"service-lasso:read", "service-lasso:lifecycle:write"}}}, current: "local", env: func(key string) string { return env[key] }}
	client, err := manager.Switch("remote")
	if err != nil || manager.Current() != "remote" || client.operatorToken != "remote" {
		t.Fatalf("switch = %#v %v", client, err)
	}
	delete(env, "REMOTE_TOKEN")
	if _, err := manager.Client("remote"); err == nil || strings.Contains(err.Error(), "REMOTE_TOKEN") {
		t.Fatalf("missing credential error leaked secret or was absent: %v", err)
	}
}

func TestResolveConnectionsRejectsUnknownSelectedProfile(t *testing.T) {
	dir := t.TempDir()
	path := filepath.Join(dir, "connections.json")
	if err := os.WriteFile(path, []byte(`{"defaultProfile":"remote","profiles":{"remote":{"url":"https://profile.example.test","tokenEnv":"PROFILE_TOKEN","authMode":"oauth-bearer","scopes":["service-lasso:read","service-lasso:lifecycle:write"]}}}`), 0o600); err != nil {
		t.Fatal(err)
	}

	tests := []struct {
		name    string
		options ConnectionOptions
		env     map[string]string
	}{
		{
			name:    "flag",
			options: ConnectionOptions{ConfigPath: path, Profile: "typo"},
		},
		{
			name:    "environment",
			options: ConnectionOptions{ConfigPath: path},
			env:     map[string]string{"SERVICE_LASSO_API_PROFILE": "typo"},
		},
		{
			name:    "configured default",
			options: ConnectionOptions{ConfigPath: writeConnectionFile(t, dir, `{"defaultProfile":"typo","profiles":{"remote":{"url":"https://profile.example.test","tokenEnv":"PROFILE_TOKEN","authMode":"oauth-bearer","scopes":["service-lasso:read","service-lasso:lifecycle:write"]}}}`)},
		},
	}

	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			_, _, err := ResolveConnections(ConnectionOptions{
				ConfigPath: test.options.ConfigPath,
				Profile:    test.options.Profile,
				Env: func(key string) string {
					return test.env[key]
				},
			})
			if err == nil || !strings.Contains(err.Error(), "unknown connection profile") {
				t.Fatalf("ResolveConnections() error = %v, want unknown profile", err)
			}
		})
	}
}

func TestResolveConnectionsRejectsMissingOrMalformedProfiles(t *testing.T) {
	dir := t.TempDir()
	for _, contents := range []string{
		`{}`,
		`{"profiles":null}`,
		`{"profiles":[]}`,
		`{`,
	} {
		path := writeConnectionFile(t, dir, contents)
		if _, _, err := ResolveConnections(ConnectionOptions{ConfigPath: path, Env: func(string) string { return "" }}); err == nil {
			t.Fatalf("ResolveConnections(%q) succeeded", contents)
		}
	}
}

func TestResolveConnectionsKeepsNoConfigDefault(t *testing.T) {
	env := map[string]string{"SERVICE_LASSO_API_TOKEN": "local-token"}
	manager, client, err := ResolveConnections(ConnectionOptions{Env: func(key string) string { return env[key] }})
	if err != nil {
		t.Fatal(err)
	}
	if manager.Current() != "default" || client.baseURL != "http://127.0.0.1:17883" || client.operatorToken != "local-token" {
		t.Fatalf("no-config default = %#v %#v", manager, client)
	}
}

func TestNonLoopbackProfilesFailClosedBeforeCredentialReadOrProxyRequest(t *testing.T) {
	var proxyRequests atomic.Int32
	proxy := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, _ *http.Request) {
		proxyRequests.Add(1)
		w.WriteHeader(http.StatusBadGateway)
	}))
	defer proxy.Close()
	proxyURL, err := url.Parse(proxy.URL)
	if err != nil {
		t.Fatal(err)
	}
	transport := &http.Transport{Proxy: http.ProxyURL(proxyURL)}

	tests := []struct {
		name    string
		profile ConnectionProfile
	}{
		{name: "legacy omitted mode", profile: ConnectionProfile{URL: "https://remote.example.test", TokenEnv: "REMOTE_TOKEN"}},
		{name: "explicit local admin", profile: ConnectionProfile{URL: "https://remote.example.test", TokenEnv: "REMOTE_TOKEN", AuthMode: AuthModeLocalAdmin}},
		{name: "missing lifecycle scope", profile: ConnectionProfile{URL: "https://remote.example.test", TokenEnv: "REMOTE_TOKEN", AuthMode: AuthModeOAuthBearer, Scopes: []string{"service-lasso:read"}}},
		{name: "plain HTTP bearer", profile: ConnectionProfile{URL: "http://remote.example.test", TokenEnv: "REMOTE_TOKEN", AuthMode: AuthModeOAuthBearer, Scopes: []string{"service-lasso:read", "service-lasso:lifecycle:write"}}},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			var credentialReads atomic.Int32
			manager := &ConnectionManager{profiles: map[string]ConnectionProfile{"local": {URL: "http://127.0.0.1:17883", TokenEnv: "LOCAL_TOKEN"}, "remote": test.profile}, current: "local", env: func(string) string {
				credentialReads.Add(1)
				return "must-not-be-read"
			}}
			if _, err := manager.Client("remote"); err == nil {
				t.Fatal("inadmissible remote profile constructed a client")
			}
			if _, err := manager.Switch("remote"); err == nil {
				t.Fatal("inadmissible remote profile switched")
			}
			if manager.Current() != "local" {
				t.Fatalf("failed profile switch changed active profile to %q", manager.Current())
			}
			if got := credentialReads.Load(); got != 0 {
				t.Fatalf("credential was read %d time(s)", got)
			}
			if _, err := NewClientWithAuth(test.profile.URL, &http.Client{Transport: transport}, "literal-token", test.profile.AuthMode, test.profile.Scopes...); err == nil {
				t.Fatal("literal remote profile bypassed constructor admission")
			}
		})
	}
	if got := proxyRequests.Load(); got != 0 {
		t.Fatalf("inadmissible profiles reached proxy %d time(s)", got)
	}
}

func TestProfileLoadingAndRemoteOverrideFailBeforeCredentialRead(t *testing.T) {
	dir := t.TempDir()
	config := writeConnectionFile(t, dir, `{"defaultProfile":"local","profiles":{"local":{"url":"http://127.0.0.1:17883","tokenEnv":"LOCAL_TOKEN"},"legacy-remote":{"url":"https://remote.example.test","tokenEnv":"REMOTE_TOKEN"}}}`)
	var credentialReads atomic.Int32
	env := func(key string) string {
		if key == "REMOTE_TOKEN" || key == "SERVICE_LASSO_API_TOKEN" {
			credentialReads.Add(1)
		}
		return "must-not-be-read"
	}
	if _, _, err := ResolveConnections(ConnectionOptions{ConfigPath: config, Env: env}); err == nil {
		t.Fatal("loading a legacy remote profile succeeded")
	}
	if got := credentialReads.Load(); got != 0 {
		t.Fatalf("profile loading read a credential %d time(s)", got)
	}

	credentialReads.Store(0)
	if _, _, err := ResolveConnections(ConnectionOptions{APIURL: "https://remote.example.test", Env: env}); err == nil {
		t.Fatal("remote URL override without explicit OAuth profile succeeded")
	}
	if got := credentialReads.Load(); got != 0 {
		t.Fatalf("remote URL override read a credential %d time(s)", got)
	}
}

func writeConnectionFile(t *testing.T, dir, contents string) string {
	t.Helper()
	path := filepath.Join(dir, strings.ReplaceAll(t.Name(), "/", "-")+".json")
	if err := os.WriteFile(path, []byte(contents), 0o600); err != nil {
		t.Fatal(err)
	}
	return path
}
