package api

import (
	"context"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
)

func TestPinnedReleaseUsesCoreAuthAndExactConfirmedTag(t *testing.T) {
	for _, mode := range []AuthMode{AuthModeLocalAdmin, AuthModeOAuthBearer} {
		t.Run(string(mode), func(t *testing.T) {
			var requests []string
			server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				requests = append(requests, r.Method+" "+r.URL.Path)
				if r.Method != "POST" {
					t.Fatal("wrong method")
				}
				if mode == AuthModeLocalAdmin && r.Header.Get("x-service-lasso-admin-token") != "private" || mode == AuthModeOAuthBearer && r.Header.Get("Authorization") != "Bearer private" {
					t.Fatal("missing selected credential")
				}
				var input map[string]any
				if json.NewDecoder(r.Body).Decode(&input) != nil {
					t.Fatal("invalid body")
				}
				if strings.HasSuffix(r.URL.Path, "check") {
					if len(input) != 0 {
						t.Fatal("check mutated input")
					}
					json.NewEncoder(w).Encode(PinnedReleaseCheck{ServiceID: "@tui", CurrentTag: "old", Status: "update_available", CandidateTag: pointerString("new")})
				} else {
					if input["tag"] != "new" || input["confirm"] != true || len(input) != 2 {
						t.Fatal("install not exactly confirmed")
					}
					json.NewEncoder(w).Encode(PinnedReleaseInstall{ServiceID: "@tui", InstalledTag: "new"})
				}
			}))
			defer server.Close()
			client, err := NewClientWithAuth(server.URL, server.Client(), "private", mode)
			if err != nil {
				t.Fatal(err)
			}
			checked, err := client.CheckPinnedRelease(context.Background(), "@tui")
			if err != nil || checked.CandidateTag == nil {
				t.Fatal(err)
			}
			if _, err = client.InstallPinnedRelease(context.Background(), "@tui", *checked.CandidateTag); err != nil {
				t.Fatal(err)
			}
			if len(requests) != 2 || requests[0] != "POST /api/services/@tui/pinned-release/check" || requests[1] != "POST /api/services/@tui/pinned-release/install" {
				t.Fatal(requests)
			}
		})
	}
}
func pointerString(value string) *string { return &value }
func TestPinnedReleaseRejectsWrongServiceAndRedactsOlderCore(t *testing.T) {
	for _, status := range []int{404, 403, 500} {
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.WriteHeader(status)
			w.Write([]byte("PRIVATE_INTERNAL_TOKEN"))
		}))
		client, _ := NewClient(server.URL, server.Client(), "secret")
		if _, err := client.CheckPinnedRelease(context.Background(), "@tui"); err == nil || strings.Contains(SafeError(err), "PRIVATE") {
			t.Fatal("unsafe error")
		}
		server.Close()
	}
	client, _ := NewClient("http://127.0.0.1:1", nil, "secret")
	if _, err := client.CheckPinnedRelease(context.Background(), "@java"); err == nil {
		t.Fatal("unsupported provider accepted")
	}
}
