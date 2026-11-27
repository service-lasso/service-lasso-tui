package api

import (
	"context"
	"fmt"
	"net/http"
	"net/http/httptest"
	"reflect"
	"strings"
	"testing"
)

func TestOperatorStackRuntimeClosedProjection(t *testing.T) {
	const baseline = `{"observedAt":"2026-11-27T10:15:00.000Z","coreUptimeMs":0,"activeOperations":{"scope":"workspace","count":0},"rows":[{"serviceId":"demo-api","uptimeMs":120000,"restartCount":0},{"serviceId":"demo-web","uptimeMs":null,"restartCount":null}],"incidents":[{"at":"2026-11-27T10:14:00.000Z","serviceId":"demo-api","kind":"operation_active"},{"at":"2026-11-27T10:12:00.000Z","serviceId":"demo-web","kind":"health_failed"}]}`
	for _, tc := range []struct {
		name, before, after string
	}{
		{"closed valid frame", "", ""},
		{"negative count", `"count":0`, `"count":-1`},
		{"unknown scope", `"scope":"workspace"`, `"scope":"host"`},
		{"negative uptime", `"uptimeMs":120000`, `"uptimeMs":-1000`},
		{"missing restart count", `,"restartCount":0`, ""},
		{"foreign incident", `"demo-web","kind"`, `"hidden-service","kind"`},
		{"private incident detail", `"kind":"health_failed"`, `"kind":"health_failed","detail":"private-source"`},
		{"unknown incident kind", `"kind":"health_failed"`, `"kind":"arbitrary_text"`},
		{"future incident", "10:14:00.000Z", "10:15:00.001Z"},
		{"reversed incidents", "10:12:00.000Z", "10:14:01.000Z"},
		{"duplicate row", `"serviceId":"demo-web","uptimeMs"`, `"serviceId":"demo-api","uptimeMs"`},
	} {
		t.Run(tc.name, func(t *testing.T) {
			ids := []string{"demo-api", "demo-web"}
			payload := baseline
			if tc.before != "" {
				payload = strings.Replace(payload, tc.before, tc.after, 1)
			}
			server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				if r.Method != "GET" || r.URL.Path != "/api/operator/v2/stack-runtime" ||
					!reflect.DeepEqual(r.URL.Query()["serviceId"], ids) || r.Header.Get("x-service-lasso-admin-token") != "fixture-token" {
					t.Error("unexpected Stack runtime request")
				}
				fmt.Fprintf(w, `{"contractVersion":"%s","requestId":"request","generatedAt":"2026-11-27T10:15:00.000Z","data":%s}`, OperatorContract, payload)
			}))
			defer server.Close()
			client, err := NewClient(server.URL, server.Client(), "fixture-token")
			if err != nil {
				t.Fatal(err)
			}
			frame, err := client.OperatorStackRuntime(context.Background(), ids)
			if tc.before != "" {
				if err == nil {
					t.Fatal("accepted unsafe or malformed projection")
				}
				return
			}
			if err != nil {
				t.Fatalf("valid Core projection refused: %v", err)
			}
			if frame.CoreUptimeMS == nil || *frame.CoreUptimeMS != 0 || frame.ActiveOperations.Count != 0 ||
				frame.Rows[0].RestartCount == nil || *frame.Rows[0].RestartCount != 0 || frame.Rows[1].RestartCount != nil ||
				frame.Rows[1].UptimeMS != nil || frame.Incidents == nil || len(*frame.Incidents) != 2 {
				t.Fatal("lost verified zero, unavailable data or incidents")
			}
		})
	}
}

func TestOperatorStackRuntimeEmptyScopeAndNullHistory(t *testing.T) {
	for _, incidentJSON := range []string{"null", "[]"} {
		t.Run(incidentJSON, func(t *testing.T) {
			server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				if r.URL.RequestURI() != "/api/operator/v2/stack-runtime" {
					t.Error("empty workspace read produced an invented selector")
				}
				fmt.Fprintf(w, `{"contractVersion":"%s","requestId":"request","generatedAt":"2026-11-27T10:15:00.000Z","data":{"observedAt":"2026-11-27T10:15:00.000Z","coreUptimeMs":1200,"activeOperations":null,"rows":[],"incidents":%s}}`, OperatorContract, incidentJSON)
			}))
			defer server.Close()
			client, err := NewClient(server.URL, server.Client(), "fixture-token")
			if err != nil {
				t.Fatal(err)
			}
			frame, err := client.OperatorStackRuntime(context.Background(), nil)
			if err != nil || frame.CoreUptimeMS == nil || *frame.CoreUptimeMS != 1200 || frame.ActiveOperations != nil ||
				(frame.Incidents == nil) != (incidentJSON == "null") {
				t.Fatalf("lost nullable projection: %v", err)
			}
		})
	}
}
