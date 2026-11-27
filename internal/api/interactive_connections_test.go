package api

import (
	"errors"
	"testing"

	"github.com/service-lasso/service-lasso-tui/internal/lifetime"
)

func TestMissingCredentialRetainsSelectedTargetMetadataForLogin(t *testing.T) {
	manager, client, err := ResolveOperatorConnections(ConnectionOptions{Receiver: lifetime.NewReceiver(), APIURL: "http://127.0.0.1:19001", Env: func(string) string { return "" }})
	var configured *ConfigurationError
	if manager == nil || client != nil || !errors.As(err, &configured) || configured.Kind != ConfigurationErrorMissingCredential {
		t.Fatal("missing credential lost target")
	}
	if profile := manager.CurrentProfile(); profile.URL != "http://127.0.0.1:19001" || profile.TokenEnv != "SERVICE_LASSO_API_TOKEN" {
		t.Fatal("target precedence changed")
	}
}

func TestInteractiveAdmissionKeepsRemoteTransportAndScopeBoundary(t *testing.T) {
	for _, target := range []string{"http://remote.example", "https://user:secret@remote.example", "https://remote.example?secret=value"} {
		if err := ValidateInteractiveOperatorTarget(target, AuthModeOAuthBearer); err == nil {
			t.Fatal("unsafe target admitted")
		}
	}
	if err := ValidateInteractiveOperatorTarget("https://remote.example", AuthModeLocalAdmin); err == nil {
		t.Fatal("local credential admitted remotely")
	}
	if err := ValidateInteractiveOperatorTarget("https://remote.example", AuthModeOAuthBearer); err != nil {
		t.Fatal(err)
	}
	manager, client, err := InteractiveOperatorConnection(lifetime.NewReceiver(), "https://remote.example", "session-secret", AuthModeOAuthBearer)
	if err != nil || client == nil || manager == nil || client.authMode != AuthModeOAuthBearer || manager.CurrentProfile().Scopes[0] != "service-lasso:read" {
		t.Fatal("OAuth read admission lost")
	}
	if client.http.CheckRedirect(nil, nil) == nil {
		t.Fatal("redirect could disclose credential")
	}
}
