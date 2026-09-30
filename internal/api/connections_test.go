package api

import (
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func TestResolveConnectionsUsesFlagEnvAndProfilePrecedenceWithoutSavingSecrets(t *testing.T) {
	dir := t.TempDir()
	path := filepath.Join(dir, "connections.json")
	const sentinel = "CONNECTION_SECRET_SENTINEL"
	contents := `{"defaultProfile":"remote","profiles":{"remote":{"url":"https://profile.example.test","tokenEnv":"PROFILE_TOKEN"}}}`
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
	manager := &ConnectionManager{profiles: map[string]ConnectionProfile{"local": {URL: "http://127.0.0.1:17883", TokenEnv: "LOCAL_TOKEN"}, "remote": {URL: "https://remote.example.test", TokenEnv: "REMOTE_TOKEN"}}, current: "local", env: func(key string) string { return env[key] }}
	client, err := manager.Switch("remote")
	if err != nil || manager.Current() != "remote" || client.operatorToken != "remote" {
		t.Fatalf("switch = %#v %v", client, err)
	}
	delete(env, "REMOTE_TOKEN")
	if _, err := manager.Client("remote"); err == nil || strings.Contains(err.Error(), "REMOTE_TOKEN") {
		t.Fatalf("missing credential error leaked secret or was absent: %v", err)
	}
}
