package api

import (
	"context"
	"fmt"
	"net/http"
	"net/http/httptest"
	"reflect"
	"testing"
)

func TestOperatorHealthChecksParsesOnlyRequestedChronologicalSamples(t *testing.T) {
	const a = `{"serviceId":"demo-api","state":"configured","samples":[{"at":"2026-11-26T12:00:00.000Z","passed":true,"durationMs":0},{"at":"2026-11-26T12:14:59.500Z","passed":false,"durationMs":231}]}`
	const b = `{"serviceId":"demo-worker","state":"not_configured","samples":[]}`
	for _, tc := range []struct {
		name string
		rows string
		ok   bool
	}{
		{"actual", a + "," + b, true},
		{"missing service", a, false},
		{"duplicate service", a + "," + a, false},
		{"foreign service", a + `,{"serviceId":"stranger","state":"not_configured","samples":[]}`, false},
		{"reversed samples", `{"serviceId":"demo-api","state":"configured","samples":[{"at":"2026-11-26T12:14:59.500Z","passed":true,"durationMs":0},{"at":"2026-11-26T12:00:00.000Z","passed":false,"durationMs":231}]},` + b, false},
		{"future", `{"serviceId":"demo-api","state":"configured","samples":[{"at":"2026-11-26T12:15:01.000Z","passed":false,"durationMs":231}]},` + b, false},
		{"old", `{"serviceId":"demo-api","state":"configured","samples":[{"at":"2026-11-26T11:59:59.999Z","passed":false,"durationMs":231}]},` + b, false},
		{"private field", `{"serviceId":"demo-api","state":"configured","samples":[{"at":"2026-11-26T12:00:00.000Z","passed":true,"durationMs":0,"url":"private"}]},` + b, false},
		{"null pass", `{"serviceId":"demo-api","state":"configured","samples":[{"at":"2026-11-26T12:00:00.000Z","passed":null,"durationMs":0}]},` + b, false},
		{"null duration", `{"serviceId":"demo-api","state":"configured","samples":[{"at":"2026-11-26T12:00:00.000Z","passed":true,"durationMs":null}]},` + b, false},
		{"missing pass field", `{"serviceId":"demo-api","state":"configured","samples":[{"at":"2026-11-26T12:00:00.000Z","durationMs":0}]},` + b, false},
	} {
		t.Run(tc.name, func(t *testing.T) {
			ids := []string{"demo-api", "demo-worker"}
			server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				if r.Method != "GET" || r.URL.Path != "/api/operator/v2/health-check-samples" ||
					!reflect.DeepEqual(r.URL.Query()["serviceId"], ids) || r.Header.Get("x-service-lasso-admin-token") != "fixture-token" {
					t.Error("unexpected health-check request")
				}
				fmt.Fprintf(w, `{"contractVersion":"%s","requestId":"request","generatedAt":"2026-11-26T12:15:00.000Z","data":{"observedAt":"2026-11-26T12:15:00.000Z","windowSeconds":900,"rows":[%s]}}`, OperatorContract, tc.rows)
			}))
			defer server.Close()
			client, err := NewClient(server.URL, server.Client(), "fixture-token")
			if err != nil {
				t.Fatal(err)
			}
			frame, err := client.OperatorHealthChecks(context.Background(), ids)
			if (err == nil) != tc.ok {
				t.Fatalf("accepted=%t expected=%t", err == nil, tc.ok)
			}
			if tc.ok && (len(frame.Rows[0].Samples) != 2 || frame.Rows[0].Samples[1].DurationMS != 231 || frame.Rows[0].Samples[1].Passed) {
				t.Fatal("lost runtime observations")
			}
		})
	}
}
