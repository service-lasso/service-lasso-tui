package app

import (
	"context"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"strings"
	"sync/atomic"
	"testing"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/service-lasso/service-lasso-tui/internal/api"
)

func TestLegacyDeniedActionReadsAvailabilityTwiceWithoutMutation(t *testing.T) {
	var availabilityReads, mutations, unexpected atomic.Int32
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet {
			mutations.Add(1)
			http.Error(w, "unexpected mutation", 500)
			return
		}
		switch r.URL.Path {
		case "/api/operator/lifecycle/services/echo/availability":
			availabilityReads.Add(1)
			var actions []map[string]any
			for _, action := range []string{"install", "config", "start", "stop", "restart", "reload"} {
				permission := "service-lasso:lifecycle:write"
				if action == "install" || action == "config" {
					permission = "service-lasso:config:write"
				}
				reason := "insufficient_scope"
				if action == "reload" {
					reason = "durable_operation_unavailable"
				}
				actions = append(actions, map[string]any{"action": action, "available": false, "reason": reason, "permission": permission, "requiresConfirmation": true})
			}
			_ = json.NewEncoder(w).Encode(map[string]any{"contractVersion": "service-lasso-durable-lifecycle-operation.v1", "serviceId": "echo", "actions": actions})
		case "/api/services/echo/health/history":
			_, _ = w.Write([]byte(`{"serviceId":"echo","history":{"transitions":[]}}`))
		case "/api/operator/lifecycle/reconciliation-context":
			// The qualified historical Core does not provide this endpoint.
			http.NotFound(w, r)
		default:
			unexpected.Add(1)
			http.NotFound(w, r)
		}
	}))
	defer server.Close()
	client, err := api.NewClientWithAuth(server.URL, server.Client(), "fixture-read-token", api.AuthModeOAuthBearer, "service-lasso:read")
	if err != nil {
		t.Fatal(err)
	}
	m := New(client, context.Background()).(model)
	m.services = []api.Service{{ID: "echo", Name: "Echo"}}
	m.selectedServiceID = "echo"
	m.screen = servicesScreen
	m.loading = false
	next, command := m.Update(tea.KeyMsg{Type: tea.KeyEnter})
	m = next.(model)
	for _, message := range runBatch(t, command) {
		next, _ = m.Update(message)
		m = next.(model)
	}
	if availabilityReads.Load() != 1 {
		t.Fatal("detail did not perform exactly one availability read")
	}
	next, command = m.Update(key('i'))
	m = next.(model)
	if command == nil {
		t.Fatal("install preparation skipped its fresh availability read")
	}
	next, _ = m.Update(command())
	m = next.(model)
	if availabilityReads.Load() != 2 {
		t.Fatal("action preparation did not perform exactly one additional availability read")
	}
	if mutations.Load() != 0 || unexpected.Load() != 0 {
		t.Fatal("denied preparation reached an unexpected request or mutation")
	}
	if m.operation != nil || m.pendingPreview != nil || m.pendingAction != "" {
		t.Fatal("denial created an operation or confirmation")
	}
	if !strings.Contains(m.View(), "Core action unavailable:") {
		t.Fatal("denial lost its precise permission-stage presentation")
	}
}
