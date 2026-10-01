package api

import (
	"context"
	"crypto/tls"
	"encoding/json"
	"errors"
	"net"
	"net/http"
	"net/http/httptest"
	"strings"
	"sync/atomic"
	"testing"
)

func TestClientReadsRuntimeSurfaces(t *testing.T) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		switch r.URL.Path {
		case "/api/health":
			_, _ = w.Write([]byte(`{"status":"ok","api":{"status":"up","version":"test"}}`))
		case "/api/services":
			_, _ = w.Write([]byte(`{"services":[{"id":"echo","name":"Echo","description":"test","enabled":true,"health":{"status":"healthy"},"lifecycle":{"state":"running"}}]}`))
		default:
			http.NotFound(w, r)
		}
	}))
	defer server.Close()

	client, err := NewClient(server.URL, server.Client(), "")
	if err != nil {
		t.Fatal(err)
	}
	health, err := client.Health(context.Background())
	if err != nil || health.Status != "ok" || health.API.Status != "up" {
		t.Fatalf("unexpected health %#v, %v", health, err)
	}
	services, err := client.Services(context.Background())
	if err != nil || len(services) != 1 || services[0].ID != "echo" {
		t.Fatalf("unexpected services %#v, %v", services, err)
	}
}

func TestClientReadsBoundedDashboardSurfaces(t *testing.T) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		switch r.URL.Path {
		case "/api/runtime/capabilities":
			_, _ = w.Write([]byte(`{"capabilities":{"runtime":{"version":"test"},"api":{"contractVersion":"service-lasso.runtime-capabilities.v1"}}}`))
		case "/api/setup/status":
			_, _ = w.Write([]byte(`{"setup":{"state":"setup_complete","setupMode":false,"vault":{"ready":true}}}`))
		case "/api/runtime/instance":
			_, _ = w.Write([]byte(`{"instance":{"status":"active","phase":"running","workspaceRoot":"SENSITIVE"}}`))
		case "/api/operator/inbox":
			if r.URL.Query().Get("limit") != "20" {
				t.Fatalf("inbox limit = %q", r.URL.Query().Get("limit"))
			}
			_, _ = w.Write([]byte(`{"inbox":{"items":[{"id":"one","title":"Needs attention","severity":"warning","state":"unread","createdAt":"now","details":"SENSITIVE"}],"pagination":{"total":1,"nextCursor":null}}}`))
		case "/api/services/echo/health/history":
			_, _ = w.Write([]byte(`{"serviceId":"echo","history":{"transitions":[{},{}]}}`))
		default:
			http.NotFound(w, r)
		}
	}))
	defer server.Close()
	client, err := NewClient(server.URL, server.Client(), "")
	if err != nil {
		t.Fatal(err)
	}
	capabilities, err := client.Capabilities(context.Background())
	if err != nil || capabilities.ContractVersion == "" {
		t.Fatalf("capabilities = %#v, %v", capabilities, err)
	}
	setup, err := client.SetupStatus(context.Background())
	if err != nil || setup.State != "setup_complete" {
		t.Fatalf("setup = %#v, %v", setup, err)
	}
	identity, err := client.RuntimeIdentity(context.Background())
	if err != nil || identity.Status != "active" {
		t.Fatalf("identity = %#v, %v", identity, err)
	}
	inbox, err := client.Inbox(context.Background(), "")
	if err != nil || len(inbox.Items) != 1 || inbox.Items[0].Title != "Needs attention" {
		t.Fatalf("inbox = %#v, %v", inbox, err)
	}
	history, err := client.HealthHistory(context.Background(), "echo")
	if err != nil || history.Entries != 2 {
		t.Fatalf("history = %#v, %v", history, err)
	}
}

func TestClientDoesNotExposeRuntimeErrorBody(t *testing.T) {
	const marker = "SYNTHETIC_SENSITIVE_MARKER_DO_NOT_DISPLAY"
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if got := r.Header.Get("x-service-lasso-admin-token"); got != "test-token" {
			t.Fatalf("Core token header = %q", got)
		}
		w.WriteHeader(http.StatusForbidden)
		_, _ = w.Write([]byte(`{"error":"permission_denied","message":"` + marker + `"}`))
	}))
	defer server.Close()

	client, err := NewClient(server.URL, server.Client(), "test-token")
	if err != nil {
		t.Fatal(err)
	}
	_, err = client.Services(context.Background())
	if err == nil {
		t.Fatal("expected runtime error")
	}
	if strings.Contains(err.Error(), marker) {
		t.Fatalf("runtime error body leaked through client: %v", err)
	}
	if !strings.Contains(err.Error(), "403 Forbidden") {
		t.Fatalf("runtime status was not retained: %v", err)
	}
}

func TestSafeHTTPErrorUsesCanonicalStatusTextOrNumericFallback(t *testing.T) {
	const marker = "SYNTHETIC_REASON_PHRASE_SECRET"
	for _, test := range []struct {
		name       string
		statusCode int
		want       string
	}{
		{name: "known status", statusCode: http.StatusForbidden, want: "403 Forbidden"},
		{name: "unknown status", statusCode: 599, want: "599"},
	} {
		t.Run(test.name, func(t *testing.T) {
			err := safeHTTPError(http.MethodGet, "/api/services", test.statusCode)
			if !strings.Contains(err.Error(), test.want) {
				t.Fatalf("safe error = %q, want %q", err, test.want)
			}
			if strings.Contains(err.Error(), marker) {
				t.Fatalf("safe error leaked reason phrase marker: %q", err)
			}
		})
	}
}

func TestClientRejectsInvalidBaseURL(t *testing.T) {
	if _, err := NewClient("not a URL", nil, ""); err == nil {
		t.Fatal("expected URL validation error")
	} else {
		var configurationError *ConfigurationError
		if !errors.As(err, &configurationError) || configurationError.Kind != ConfigurationErrorInvalidURL {
			t.Fatalf("unexpected configuration error: %#v", err)
		}
	}
}

func TestClientConfigurationErrorsHaveClosedKinds(t *testing.T) {
	cases := []struct {
		url   string
		token string
		kind  ConfigurationErrorKind
	}{
		{"ftp://runtime.example.test", "", ConfigurationErrorUnsupportedScheme},
		{"https://operator:secret@runtime.example.test", "", ConfigurationErrorUserinfo},
		{"https://runtime.example.test?next=/other", "", ConfigurationErrorQueryOrFragment},
		{"http://runtime.example.test", "token", ConfigurationErrorRemoteProfileAdmission},
	}
	for _, test := range cases {
		t.Run(string(test.kind), func(t *testing.T) {
			_, err := NewClient(test.url, nil, test.token)
			var configurationError *ConfigurationError
			if !errors.As(err, &configurationError) || configurationError.Kind != test.kind {
				t.Fatalf("configuration error = %#v, want %q", err, test.kind)
			}
		})
	}
}

func TestClientProtectsTokenTransport(t *testing.T) {
	if _, err := NewClient("http://runtime.example.test", nil, "token"); err == nil {
		t.Fatal("expected non-loopback local-admin client to be rejected")
	}
	if _, err := NewClient("http://127.0.0.1:17883", nil, "token"); err != nil {
		t.Fatalf("expected loopback HTTP token transport to be allowed: %v", err)
	}
	if _, err := NewClient("https://runtime.example.test", nil, "token"); err == nil {
		t.Fatal("expected non-loopback local-admin client to be rejected")
	}
	if _, err := NewClientWithAuth("http://runtime.example.test", nil, "token", AuthModeOAuthBearer, "service-lasso:read", "service-lasso:lifecycle:write"); err == nil {
		t.Fatal("expected remote HTTP bearer transport to be rejected")
	}
	if _, err := NewClientWithAuth("https://runtime.example.test", nil, "token", AuthModeOAuthBearer, "service-lasso:read", "service-lasso:lifecycle:write"); err != nil {
		t.Fatalf("expected explicit HTTPS bearer client to be allowed: %v", err)
	}
	if _, err := NewClient("https://operator:secret@runtime.example.test", nil, ""); err == nil {
		t.Fatal("expected URL userinfo to be rejected")
	}
	if _, err := NewClient("ftp://runtime.example.test", nil, ""); err == nil {
		t.Fatal("expected unsupported URL scheme to be rejected")
	}
	if _, err := NewClient("https://runtime.example.test?next=/other", nil, ""); err == nil {
		t.Fatal("expected URL query to be rejected")
	}
}

func TestClientDoesNotFollowCrossOriginRedirectWithOperatorToken(t *testing.T) {
	var sourceSawToken atomic.Bool
	var targetRequests atomic.Int32
	var targetSawToken atomic.Bool
	target := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		targetRequests.Add(1)
		targetSawToken.Store(r.Header.Get("x-service-lasso-admin-token") != "")
		w.WriteHeader(http.StatusOK)
	}))
	defer target.Close()
	source := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		sourceSawToken.Store(r.Header.Get("x-service-lasso-admin-token") == "test-token")
		http.Redirect(w, r, target.URL+"/redirect-target", http.StatusFound)
	}))
	defer source.Close()

	provided := &http.Client{CheckRedirect: func(_ *http.Request, _ []*http.Request) error { return nil }}
	client, err := NewClient(source.URL, provided, "test-token")
	if err != nil {
		t.Fatal(err)
	}
	_, err = client.Services(context.Background())
	if err == nil || !strings.Contains(err.Error(), "302 Found") {
		t.Fatalf("expected redirect to fail closed, got %v", err)
	}
	if !sourceSawToken.Load() {
		t.Fatal("source did not receive the operator token")
	}
	if targetSawToken.Load() {
		t.Fatal("operator token reached the cross-origin redirect target")
	}
	if got := targetRequests.Load(); got != 0 {
		t.Fatalf("cross-origin redirect target received %d request(s), want 0", got)
	}
}

func TestClientDoesNotFollowHTTPSDowngradeRedirectWithOperatorToken(t *testing.T) {
	var sourceSawToken atomic.Bool
	var targetRequests atomic.Int32
	var targetSawToken atomic.Bool
	target := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		targetRequests.Add(1)
		targetSawToken.Store(r.Header.Get("x-service-lasso-admin-token") != "")
		w.WriteHeader(http.StatusOK)
	}))
	defer target.Close()
	source := httptest.NewTLSServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		sourceSawToken.Store(r.Header.Get("x-service-lasso-admin-token") == "test-token")
		http.Redirect(w, r, target.URL+"/redirect-target", http.StatusFound)
	}))
	defer source.Close()

	provided := source.Client()
	provided.CheckRedirect = func(_ *http.Request, _ []*http.Request) error { return nil }
	client, err := NewClient(source.URL, provided, "test-token")
	if err != nil {
		t.Fatal(err)
	}
	_, err = client.Services(context.Background())
	if err == nil || !strings.Contains(err.Error(), "302 Found") {
		t.Fatalf("expected HTTPS downgrade redirect to fail closed, got %v", err)
	}
	if !sourceSawToken.Load() {
		t.Fatal("HTTPS source did not receive the operator token")
	}
	if targetSawToken.Load() {
		t.Fatal("operator token reached the HTTP redirect target")
	}
	if got := targetRequests.Load(); got != 0 {
		t.Fatalf("HTTP redirect target received %d request(s), want 0", got)
	}
}

func TestDurableLifecycleUsesPreviewOneFrozenSubmitAndSafeReadback(t *testing.T) {
	workflows := []struct {
		action     string
		coreAction string
	}{
		{action: "install", coreAction: "service_install"},
		{action: "config", coreAction: "service_configure"},
		{action: "start", coreAction: "service_start"},
		{action: "stop", coreAction: "service_stop"},
		{action: "restart", coreAction: "service_restart"},
	}
	var submits atomic.Int32
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.Header.Get("x-service-lasso-admin-token") != "local-token" {
			t.Fatalf("wrong auth mode: %q", r.Header.Get("Authorization"))
		}
		switch r.URL.Path {
		case "/api/operator/lifecycle/services/echo/availability":
			_, _ = w.Write([]byte(`{"actions":[{"action":"install","available":true,"reason":null,"permission":"service-lasso:lifecycle:write","requiresConfirmation":true},{"action":"config","available":true,"reason":null,"permission":"service-lasso:lifecycle:write","requiresConfirmation":true},{"action":"start","available":true,"reason":null,"permission":"service-lasso:lifecycle:write","requiresConfirmation":true},{"action":"stop","available":true,"reason":null,"permission":"service-lasso:lifecycle:write","requiresConfirmation":true},{"action":"restart","available":true,"reason":null,"permission":"service-lasso:lifecycle:write","requiresConfirmation":true},{"action":"reload","available":false,"reason":"durable_operation_unavailable","permission":"service-lasso:lifecycle:write","requiresConfirmation":false}]}`))
		case "/api/operator/lifecycle/operations":
			var body map[string]any
			_ = json.NewDecoder(r.Body).Decode(&body)
			action, _ := body["action"].(string)
			coreAction := "service_" + action
			if action == "config" {
				coreAction = "service_configure"
			}
			if body["serviceId"] != "echo" {
				t.Fatalf("service target = %#v", body["serviceId"])
			}
			if body["execute"] == true {
				submits.Add(1)
				if body["confirmationPhrase"] != "confirm "+action || body["confirmationId"] != "mcp-confirmation-12345678" {
					t.Fatalf("missing frozen confirmation: %#v", body)
				}
				_, _ = w.Write([]byte(`{"accepted":true,"operation":{"operationId":"mcp-operation-12345678","action":"` + coreAction + `","status":"running","phase":"executing","progress":50,"outcome":null,"cancellationSupported":false,"ownership":"own","summary":"SECRET /private/path"}}`))
				return
			}
			_, _ = w.Write([]byte(`{"action":"` + coreAction + `","preflight":{"targets":["echo"],"effects":["The runtime will apply the requested lifecycle action."]},"confirmation":{"id":"mcp-confirmation-12345678","status":"pending","confirmationPhrase":"confirm ` + action + `"}}`))
		case "/api/operator/lifecycle/operations/mcp-operation-12345678":
			_, _ = w.Write([]byte(`{"operation":{"operationId":"mcp-operation-12345678","action":"service_start","status":"succeeded","phase":"completed","progress":100,"outcome":"succeeded","cancellationSupported":false,"ownership":"own","summary":"SECRET /private/path"}}`))
		default:
			http.NotFound(w, r)
		}
	}))
	defer server.Close()
	client, err := NewClient(server.URL, server.Client(), "local-token")
	if err != nil {
		t.Fatal(err)
	}
	actions, err := client.LifecycleAvailability(context.Background(), "echo")
	if err != nil || len(actions) != len(workflows)+1 || !actions[0].Available || actions[len(actions)-1].Action != "reload" || actions[len(actions)-1].Available {
		t.Fatalf("availability = %#v, %v", actions, err)
	}
	for _, workflow := range workflows {
		t.Run(workflow.action, func(t *testing.T) {
			preview, err := client.PreviewLifecycle(context.Background(), "echo", workflow.action)
			if err != nil || preview.ConfirmationPhrase != "confirm "+workflow.action || preview.Action != workflow.action {
				t.Fatalf("preview = %#v, %v", preview, err)
			}
			key, err := NewIdempotencyKey()
			if err != nil {
				t.Fatal(err)
			}
			operation, err := client.SubmitLifecycle(context.Background(), preview, key)
			if err != nil || operation.ID == "" || operation.Action != workflow.coreAction {
				t.Fatalf("submit = %#v, %v", operation, err)
			}
			operation, err = client.Operation(context.Background(), operation.ID)
			if err != nil || operation.Outcome != "succeeded" || operation.CancellationSupported {
				t.Fatalf("readback = %#v, %v", operation, err)
			}
		})
	}
	if got := submits.Load(); got != int32(len(workflows)) {
		t.Fatalf("submit count = %d, want %d", got, len(workflows))
	}
}

func TestSafeTargetEffectsAcceptsCorePreviewTextAndRejectsTerminalControls(t *testing.T) {
	if !safeTargetEffects([]string{"@node"}, []string{"No materialized config file changes are expected."}) {
		t.Fatal("valid Core preview effect was rejected")
	}
	if safeTargetEffects([]string{"@not/a-service"}, []string{"No materialized config file changes are expected."}) {
		t.Fatal("invalid Core preview target was accepted")
	}
	for _, effect := range []string{"", "line one\nline two", "unsafe\x1b[2J", "\u0080"} {
		if safeTargetEffects([]string{"node-sample-service"}, []string{effect}) {
			t.Fatalf("unsafe preview effect was accepted: %q", effect)
		}
	}
}

func TestOAuthBearerIsExplicitAndMalformedOperationNeverRenders(t *testing.T) {
	server := httptest.NewTLSServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.Header.Get("Authorization") != "Bearer remote-token" || r.Header.Get("x-service-lasso-admin-token") != "" {
			t.Fatalf("wrong headers")
		}
		_, _ = w.Write([]byte(`{"operation":{"operationId":"mcp-operation-12345678","action":"service_start\u001b[2J","status":"running","phase":"executing","progress":50,"outcome":null,"cancellationSupported":false,"ownership":"own"}}`))
	}))
	defer server.Close()
	client, err := NewClientWithAuth(server.URL, server.Client(), "remote-token", AuthModeOAuthBearer, "service-lasso:read", "service-lasso:lifecycle:write")
	if err != nil {
		t.Fatal(err)
	}
	if _, err := client.Operation(context.Background(), "mcp-operation-12345678"); err == nil {
		t.Fatal("unsafe operation payload was accepted")
	}
}

func TestExplicitRemoteBearerSendsOnlyAuthorization(t *testing.T) {
	server := httptest.NewTLSServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.Host != "remote.example.test" {
			t.Fatalf("request host = %q", r.Host)
		}
		if got := r.Header.Get("Authorization"); got != "Bearer remote-token" {
			t.Fatalf("authorization = %q", got)
		}
		if got := r.Header.Get("x-service-lasso-admin-token"); got != "" {
			t.Fatalf("local-admin header leaked into bearer request: %q", got)
		}
		_, _ = w.Write([]byte(`{"status":"ok","api":{"status":"up","version":"test"}}`))
	}))
	defer server.Close()

	transport := server.Client().Transport.(*http.Transport).Clone()
	transport.TLSClientConfig = &tls.Config{InsecureSkipVerify: true} // test server is reached through a non-loopback authority.
	transport.DialContext = func(ctx context.Context, network, _ string) (net.Conn, error) {
		return (&net.Dialer{}).DialContext(ctx, network, server.Listener.Addr().String())
	}
	client, err := NewClientWithAuth("https://remote.example.test", &http.Client{Transport: transport}, "remote-token", AuthModeOAuthBearer, "service-lasso:read", "service-lasso:lifecycle:write")
	if err != nil {
		t.Fatal(err)
	}
	if _, err := client.Health(context.Background()); err != nil {
		t.Fatal(err)
	}
}
