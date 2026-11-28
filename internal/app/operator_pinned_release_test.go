package app

import (
	"context"
	"encoding/json"
	tea "github.com/charmbracelet/bubbletea"
	"github.com/service-lasso/service-lasso-tui/internal/api"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
)

func pinnedModel() operatorModel {
	m := stackFixture()
	m.service = "@tui"
	m.journey = "lifecycle"
	m.lifecycle = &api.OperatorLifecycleAvailability{}
	m.acceptPageSelection(m.ticket())
	return m
}
func TestPinnedReleaseCancelAndStaleResponseNeverInstall(t *testing.T) {
	m := pinnedModel()
	ticket := m.ticket()
	m.pinnedRelease = &operatorPinnedRelease{Ticket: ticket, CurrentTag: "old", CandidateTag: "new"}
	if !strings.Contains(m.View(), "Install this release for @tui?") {
		t.Fatal("missing exact confirmation")
	}
	updated, cmd := m.Update(tea.KeyMsg{Type: tea.KeyEsc})
	if cmd != nil || updated.(operatorModel).pinnedRelease != nil {
		t.Fatal("cancel dispatched")
	}
	m.pinnedRelease = &operatorPinnedRelease{Ticket: ticket, Busy: true}
	m.selection++
	updated, cmd = m.Update(operatorPinnedReleaseResult{Ticket: ticket, Check: api.PinnedReleaseCheck{ServiceID: "@tui", CurrentTag: "old", CandidateTag: pointerTag("new")}})
	if cmd != nil || updated.(operatorModel).pinnedRelease.CandidateTag != "" {
		t.Fatal("stale check supplied candidate")
	}
}
func pointerTag(tag string) *string { return &tag }
func TestPinnedReleaseRefusesHiddenConfirmationAndUnsupportedRows(t *testing.T) {
	m := pinnedModel()
	m.width = 15
	m.height = 24
	m.pinnedRelease = &operatorPinnedRelease{Ticket: m.ticket(), CurrentTag: "old", CandidateTag: strings.Repeat("x", 100)}
	updated, cmd := m.Update(tea.KeyMsg{Type: tea.KeyEnter})
	if cmd != nil || updated.(operatorModel).pinnedRelease.Installing {
		t.Fatal("hidden tag installed")
	}
	m = pinnedModel()
	m.service = "@java"
	if m.pinnedReleaseRow() {
		t.Fatal("runtime provider offered client updates")
	}
}

func TestPinnedReleaseRequiresCheckThenExplicitConfirmation(t *testing.T) {
	m := pinnedModel()
	m.ctx = context.Background()
	m.width = 120
	m.height = 32
	var checked, installed int
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		switch r.URL.Path {
		case "/api/operator/v2/reconciliation-context":
			json.NewEncoder(w).Encode(map[string]any{"contractVersion": "service-lasso-durable-reconciliation-context.v1", "context": m.binding})
		case "/api/services/@tui/pinned-release/check":
			checked++
			json.NewEncoder(w).Encode(api.PinnedReleaseCheck{ServiceID: "@tui", CurrentTag: "old", Status: "update_available", CandidateTag: pointerTag("new")})
		case "/api/services/@tui/pinned-release/install":
			installed++
			var body struct {
				Tag     string `json:"tag"`
				Confirm bool   `json:"confirm"`
			}
			json.NewDecoder(r.Body).Decode(&body)
			if body.Tag != "new" || !body.Confirm {
				t.Error("wrong confirmed candidate")
			}
			json.NewEncoder(w).Encode(api.PinnedReleaseInstall{ServiceID: "@tui", InstalledTag: "new"})
		default:
			t.Error("unexpected request", r.URL.Path)
			w.WriteHeader(404)
		}
	}))
	defer server.Close()
	m.client, _ = api.NewClient(server.URL, server.Client(), "private")
	cmd := m.beginPinnedRelease()
	if cmd == nil {
		t.Fatal("check was not offered")
	}
	updated, _ := m.Update(cmd())
	m = updated.(operatorModel)
	if checked != 1 || installed != 0 || m.pinnedRelease.CandidateTag != "new" {
		t.Fatal("check dispatched install or lost candidate")
	}
	updated, cmd = m.Update(tea.KeyMsg{Type: tea.KeyEnter})
	m = updated.(operatorModel)
	if cmd == nil || installed != 0 {
		t.Fatal("confirmation did not defer network command")
	}
	updated, _ = m.Update(cmd())
	m = updated.(operatorModel)
	if installed != 1 || m.pinnedRelease.InstalledTag != "new" || !strings.Contains(m.View(), "next client launch") {
		t.Fatal("installed result missing")
	}
	_, cmd = m.Update(tea.KeyMsg{Type: tea.KeyEnter})
	if cmd != nil {
		t.Fatal("installed release replayed")
	}
}
