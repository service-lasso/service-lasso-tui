package api

import (
	"encoding/json"
	"errors"
	"os"
	"path/filepath"
	"testing"
)

func TestOperatorReaderAdmissionReportsItsOwnRequirement(t *testing.T) {
	_, err := NewOperatorClientWithAuth("https://core.example.invalid", nil, "codec-only", AuthModeOAuthBearer, "service-lasso:lifecycle:write")
	var configured *ConfigurationError
	if !errors.As(err, &configured) || configured.Kind != ConfigurationErrorOperatorReaderAdmission || configured.Error() != "non-loopback Service Lasso API requires explicit oauth-bearer general-read scope" {
		t.Fatal("v2 reader diagnosis requested unrelated lifecycle authority")
	}
}

// Transport/profile admission source cases, not OAuth token or Core permission
// proof. Scope metadata cannot grant discovery, reads or mutations on a server.
func operatorReaderConfig(t *testing.T, profiles map[string]ConnectionProfile) string {
	t.Helper()
	raw, err := json.Marshal(connectionFile{Default: "reader", Profiles: profiles})
	if err != nil {
		t.Fatal(err)
	}
	path := filepath.Join(t.TempDir(), "connections.json")
	if err = os.WriteFile(path, raw, 0600); err != nil {
		t.Fatal(err)
	}
	return path
}

func TestOperatorRemoteReaderLaunchAndSwitchPreserveMode(t *testing.T) {
	profiles := map[string]ConnectionProfile{
		"reader":     {URL: "https://core.example.invalid", TokenEnv: "READER_TOKEN", AuthMode: AuthModeOAuthBearer, Scopes: []string{"service-lasso:read"}},
		"other":      {URL: "https://other.example.invalid", TokenEnv: "OTHER_TOKEN", AuthMode: AuthModeOAuthBearer, Scopes: []string{"service-lasso:read"}},
		"incomplete": {TokenEnv: "NEVER_TOKEN", AuthMode: AuthModeOAuthBearer, Scopes: []string{"service-lasso:read"}},
	}
	reads := map[string]int{}
	env := func(name string) string {
		reads[name]++
		switch name {
		case "READER_TOKEN":
			return "codec-only-reader"
		case "OTHER_TOKEN":
			return "codec-only-other"
		case "NEVER_TOKEN":
			t.Error("inadmissible profile read credential")
		}
		return ""
	}
	manager, client, err := ResolveOperatorConnections(ConnectionOptions{ConfigPath: operatorReaderConfig(t, profiles), Env: env})
	if err != nil || client == nil || !manager.operatorV2 || manager.Current() != "reader" || reads["READER_TOKEN"] != 1 {
		t.Fatal("v2 general reader launch denied")
	}
	if client.authMode != AuthModeOAuthBearer || client.baseURL != profiles["reader"].URL {
		t.Fatal("transport profile changed")
	}
	switched, err := manager.Switch("other")
	if err != nil || switched == nil || manager.Current() != "other" || reads["OTHER_TOKEN"] != 1 {
		t.Fatal("v2 reader switch lost mode")
	}
	if _, err = manager.Switch("incomplete"); err == nil || manager.Current() != "other" || reads["NEVER_TOKEN"] != 0 {
		t.Fatal("denied switch changed selection or read credential")
	}
	if _, err = manager.Switch("missing"); err == nil || manager.Current() != "other" {
		t.Fatal("unknown switch changed selection")
	}
}

func TestOperatorInvalidRemoteProfilesPrecedeCredentialLookup(t *testing.T) {
	for _, profile := range []ConnectionProfile{
		{URL: "https://core.example.invalid", TokenEnv: "NEVER_TOKEN", AuthMode: AuthModeOAuthBearer, Scopes: []string{"service-lasso:lifecycle:write"}},
		{URL: "https://core.example.invalid", TokenEnv: "NEVER_TOKEN", AuthMode: AuthModeLocalAdmin, Scopes: []string{"service-lasso:read"}},
		{URL: "http://core.example.invalid", TokenEnv: "NEVER_TOKEN", AuthMode: AuthModeOAuthBearer, Scopes: []string{"service-lasso:read"}},
	} {
		read := false
		_, _, err := ResolveOperatorConnections(ConnectionOptions{ConfigPath: operatorReaderConfig(t, map[string]ConnectionProfile{"reader": profile}), Env: func(name string) string {
			if name == "NEVER_TOKEN" {
				read = true
			}
			return ""
		}})
		if err == nil || read {
			t.Fatal("inadmissible remote profile reached credential")
		}
	}
}

func TestOperatorRemoteOverrideRejectedBeforeCredential(t *testing.T) {
	reader := ConnectionProfile{URL: "https://core.example.invalid", TokenEnv: "READER_TOKEN", AuthMode: AuthModeOAuthBearer, Scopes: []string{"service-lasso:read"}}
	read := false
	_, _, err := ResolveOperatorConnections(ConnectionOptions{ConfigPath: operatorReaderConfig(t, map[string]ConnectionProfile{"reader": reader}), APIURL: "http://insecure.example.invalid", TokenEnv: "NEVER_TOKEN", Env: func(name string) string {
		if name == "NEVER_TOKEN" || name == "READER_TOKEN" {
			read = true
		}
		return ""
	}})
	if err == nil || read {
		t.Fatal("insecure override read credential")
	}
}

func TestMissingCredentialHasSafeActionableDiagnosis(t *testing.T) {
	for _, tokenEnv := range []string{"SERVICE_LASSO_API_TOKEN", "private-profile-name"} {
		m := &ConnectionManager{profiles: map[string]ConnectionProfile{"reader": {URL: "http://127.0.0.1:9999", TokenEnv: tokenEnv}}, env: func(string) string { return "" }, operatorV2: true}
		client, err := m.Client("reader")
		var configured *ConfigurationError
		if client != nil || !errors.As(err, &configured) || configured.Kind != ConfigurationErrorMissingCredential {
			t.Fatalf("missing typed diagnosis: %v", err)
		}
		if got := err.Error(); got != "Service Lasso API token missing; set SERVICE_LASSO_API_TOKEN or the configured profile token environment variable" {
			t.Fatalf("unsafe or unhelpful diagnosis %q", got)
		}
	}
}
