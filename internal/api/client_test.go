package api

import (
	"context"
	"encoding/json"
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

func TestClientPostsConfirmedLifecycleAction(t *testing.T) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodPost || r.URL.Path != "/api/services/echo/start" {
			t.Fatalf("unexpected request %s %s", r.Method, r.URL.Path)
		}
		if got := r.Header.Get("Content-Type"); got != "application/json" {
			t.Fatalf("content type %q", got)
		}
		if got := r.Header.Get("x-service-lasso-admin-token"); got != "test-token" {
			t.Fatalf("operator token header %q", got)
		}
		var body struct {
			Confirm bool `json:"confirm"`
		}
		if err := json.NewDecoder(r.Body).Decode(&body); err != nil || !body.Confirm {
			t.Fatalf("expected explicit confirmation, body=%#v err=%v", body, err)
		}
		_, _ = w.Write([]byte(`{"ok":true,"action":"start","serviceId":"echo","message":"started"}`))
	}))
	defer server.Close()

	client, err := NewClient(server.URL, server.Client(), "test-token")
	if err != nil {
		t.Fatal(err)
	}
	result, err := client.Lifecycle(context.Background(), "echo", "start")
	if err != nil || !result.OK || result.Message != "started" {
		t.Fatalf("unexpected lifecycle result %#v, %v", result, err)
	}
}

func TestClientRejectsInvalidBaseURL(t *testing.T) {
	if _, err := NewClient("not a URL", nil, ""); err == nil {
		t.Fatal("expected URL validation error")
	}
}

func TestClientProtectsTokenTransport(t *testing.T) {
	if _, err := NewClient("http://runtime.example.test", nil, "token"); err == nil {
		t.Fatal("expected non-loopback HTTP token transport to be rejected")
	}
	if _, err := NewClient("http://127.0.0.1:17883", nil, "token"); err != nil {
		t.Fatalf("expected loopback HTTP token transport to be allowed: %v", err)
	}
	if _, err := NewClient("https://runtime.example.test", nil, "token"); err != nil {
		t.Fatalf("expected HTTPS token transport to be allowed: %v", err)
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

func TestClientDoesNotFollowLifecycleRedirect(t *testing.T) {
	var redirectedRequests atomic.Int32
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path == "/api/services/echo/start" {
			http.Redirect(w, r, "/api/services/echo/redirect-target", http.StatusTemporaryRedirect)
			return
		}
		redirectedRequests.Add(1)
		w.WriteHeader(http.StatusOK)
	}))
	defer server.Close()

	provided := &http.Client{CheckRedirect: func(_ *http.Request, _ []*http.Request) error { return nil }}
	client, err := NewClient(server.URL, provided, "test-token")
	if err != nil {
		t.Fatal(err)
	}
	_, err = client.Lifecycle(context.Background(), "echo", "start")
	if err == nil || !strings.Contains(err.Error(), "307 Temporary Redirect") {
		t.Fatalf("expected lifecycle redirect to fail closed, got %v", err)
	}
	if got := redirectedRequests.Load(); got != 0 {
		t.Fatalf("redirect target received %d lifecycle request(s), want 0", got)
	}
}

func TestClientRejectsUnsafeLifecycleServiceID(t *testing.T) {
	var requests atomic.Int32
	server := httptest.NewServer(http.HandlerFunc(func(http.ResponseWriter, *http.Request) {
		requests.Add(1)
	}))
	defer server.Close()

	client, err := NewClient(server.URL, server.Client(), "")
	if err != nil {
		t.Fatal(err)
	}
	_, err = client.Lifecycle(context.Background(), "echo/start", "start")
	if err == nil || !strings.Contains(err.Error(), "invalid service ID") {
		t.Fatalf("expected unsafe service ID to be rejected, got %v", err)
	}
	if got := requests.Load(); got != 0 {
		t.Fatalf("received %d request(s) for rejected service ID, want 0", got)
	}
}
