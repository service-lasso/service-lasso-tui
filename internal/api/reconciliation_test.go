package api

import (
    "fmt"
    "net/http"
    "net/http/httptest"
    "strings"
    "testing"
)

func TestReconciliationContextUsesOnlyClosedServerAuthority(t *testing.T) {
    a := "slrc_"+strings.Repeat("a",64)
    b := "slrc_"+strings.Repeat("b",64)
    body := fmt.Sprintf(`{"contractVersion":"service-lasso-durable-reconciliation-context.v1","context":{"instanceBinding":%q,"workspaceBinding":%q,"actorBinding":%q,"clientBinding":%q}}`,a,a,a,a)
    status := http.StatusOK
    server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter,r *http.Request) {
        if r.URL.Path!="/api/operator/lifecycle/reconciliation-context" || r.Header.Get("Authorization")!="Bearer inert-fixture" { t.Error("wrong context authority request") }
        w.WriteHeader(status); _,_ = w.Write([]byte(body))
    }))
    defer server.Close()
    client, err := NewClientWithAuth(server.URL,server.Client(),"inert-fixture",AuthModeOAuthBearer)
    if err!=nil { t.Fatal(err) }
    first, ok := client.ReconciliationContext()
    if !ok || !strings.HasPrefix(first,"slrc1_") { t.Fatal("closed server context was not admitted") }
    body = strings.Replace(body,`"actorBinding":"`+a+`"`,`"actorBinding":"`+b+`"`,1)
    second, ok := client.ReconciliationContext()
    if !ok || first==second { t.Fatal("actor change failed to change durable binding") }
    validBody := body
    for _, invalid := range []string{
        strings.Replace(validBody,`"context":`, `"unknown":true,"context":`,1),
        strings.Replace(validBody,`"actorBinding":`, `"actorBinding":"`+a+`","actorBinding":`,1),
        strings.Replace(validBody,`"instanceBinding":`, `"instanceBinding":null,"instanceBinding":`,1),
        strings.Replace(validBody,`"clientBinding":"`+a+`"`,`"clientBinding":"host-or-credential-derived"`,1),
        body+` {}`, strings.Repeat(" ",4097),
    } {
        body = invalid
        if binding, admitted := client.ReconciliationContext(); admitted || binding!="" { t.Fatal("invalid context admitted") }
    }
    status=http.StatusNotFound
    if binding, admitted := client.ReconciliationContext(); admitted || binding!="" { t.Fatal("Core2633 missing endpoint must deny persistent restoration") }
}
