package api

import (
	"context"
	"net/http"
	"net/http/httptest"
	"strings"
	"sync/atomic"
	"testing"
)

func TestLegacyServiceReadDenialHasSpecificSafeTitle(t *testing.T) {
	for _, tc := range []struct {
		status int
		title  string
	}{
		{http.StatusUnauthorized, "Runtime authentication denied"},
		{http.StatusForbidden, "Runtime permission denied"},
		{http.StatusServiceUnavailable, "Runtime API unavailable"},
	} {
		t.Run(http.StatusText(tc.status), func(t *testing.T) {
			const privateBody = "private-runtime-body-must-stay-private"
			var calls atomic.Int32
			server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				calls.Add(1)
				if r.Method != http.MethodGet || r.URL.Path != "/api/services" {
					t.Errorf("unexpected request method or route")
				}
				w.WriteHeader(tc.status)
				_, _ = w.Write([]byte(privateBody))
			}))
			defer server.Close()
			client, err := NewClient(server.URL, server.Client(), "fixture-invalid-token")
			if err != nil {
				t.Fatal(err)
			}
			services, err := client.Services(context.Background())
			if err == nil || len(services) != 0 {
				t.Fatal("denied service read yielded usable services")
			}
			if got := ErrorTitle(err); got != tc.title {
				t.Fatalf("denial title = %q, want %q", got, tc.title)
			}
			if strings.Contains(SafeError(err), privateBody) {
				t.Fatal("private response body entered safe presentation")
			}
			if calls.Load() != 1 {
				t.Fatal("denied read retried or attempted another request")
			}
		})
	}
}
