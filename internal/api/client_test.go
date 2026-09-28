package api

import (
	"context"
	"net/http"
	"net/http/httptest"
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

	client, err := NewClient(server.URL, server.Client())
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

func TestClientRejectsInvalidBaseURL(t *testing.T) {
	if _, err := NewClient("not a URL", nil); err == nil {
		t.Fatal("expected URL validation error")
	}
}
