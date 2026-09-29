package api

import (
	"context"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"strings"
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

func TestClientDoesNotExposeRuntimeErrorBody(t *testing.T) {
	const marker = "SYNTHETIC_SENSITIVE_MARKER_DO_NOT_DISPLAY"
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusForbidden)
		_, _ = w.Write([]byte(`{"error":"permission_denied","message":"` + marker + `"}`))
	}))
	defer server.Close()

	client, err := NewClient(server.URL, server.Client(), "")
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
}
