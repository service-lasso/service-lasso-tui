package api

import (
    "bytes"
    "context"
    "crypto/sha256"
    "encoding/hex"
    "encoding/json"
    "io"
    "unicode/utf8"
    "net/http"
    "regexp"
)

// ReconciliationContext admits only the reviewed server-issued context. Core
//2633 lacks this endpoint, so persistent restoration remains unavailable there.
// No URL, profile or credential participates in the durable binding.
func (c *Client) ReconciliationContext() (string, bool) {
    ctx, cancel := context.WithTimeout(context.Background(), requestTimeout)
    defer cancel()
    req, err := http.NewRequestWithContext(ctx, http.MethodGet, c.baseURL+"/api/operator/lifecycle/reconciliation-context", nil)
    if err != nil { return "", false }
    if c.operatorToken != "" && c.authMode == AuthModeLocalAdmin { req.Header.Set("x-service-lasso-admin-token", c.operatorToken) } else if c.operatorToken != "" && c.authMode == AuthModeOAuthBearer { req.Header.Set("Authorization", "Bearer "+c.operatorToken) }
    response, err := c.http.Do(req)
    if err != nil { return "", false }
    defer response.Body.Close()
    if response.StatusCode != http.StatusOK { return "", false }
    raw, err := io.ReadAll(io.LimitReader(response.Body, 4097))
    if err != nil || len(raw) > 4096 || !utf8.Valid(raw) || !closedContextJSON(raw) { return "", false }
    var value struct {
        ContractVersion string `json:"contractVersion"`
        Context struct {
            Instance string `json:"instanceBinding"`
            Workspace string `json:"workspaceBinding"`
            Actor string `json:"actorBinding"`
            Client string `json:"clientBinding"`
        } `json:"context"`
    }
    decoder := json.NewDecoder(bytes.NewReader(raw)); decoder.DisallowUnknownFields()
    if decoder.Decode(&value) != nil || value.ContractVersion != "service-lasso-durable-reconciliation-context.v1" { return "", false }
    pattern := regexp.MustCompile(`^slrc_[a-f0-9]{64}$`)
    for _, binding := range []string{value.Context.Instance, value.Context.Workspace, value.Context.Actor, value.Context.Client} { if !pattern.MatchString(binding) { return "", false } }
    canonical, err := json.Marshal(value)
    if err != nil { return "", false }
    digest := sha256.Sum256(canonical)
    return "slrc1_"+hex.EncodeToString(digest[:]), true
}

// Context has exactly one outer object and one nested object. Count keys before
// decoding so repeated identical keys cannot silently overwrite authority.
func closedContextJSON(raw []byte) bool {
    decoder := json.NewDecoder(bytes.NewReader(raw))
    token, err := decoder.Token(); if err != nil || token != json.Delim('{') { return false }
    outer := map[string]bool{}
    for decoder.More() {
        token, err = decoder.Token(); key, ok := token.(string)
        if err != nil || !ok || outer[key] { return false }; outer[key] = true
        if key == "contractVersion" { token, err = decoder.Token(); if _, ok = token.(string); err != nil || !ok { return false } } else if key == "context" {
            token, err = decoder.Token(); if err != nil || token != json.Delim('{') { return false }
            nested := map[string]bool{}
            for decoder.More() {
                token, err = decoder.Token(); name, valid := token.(string)
                if err != nil || !valid || nested[name] { return false }; nested[name] = true
                if name != "instanceBinding" && name != "workspaceBinding" && name != "actorBinding" && name != "clientBinding" { return false }
                token, err = decoder.Token(); if _, valid = token.(string); err != nil || !valid { return false }
            }
            token, err = decoder.Token(); if err != nil || token != json.Delim('}') || len(nested) != 4 { return false }
        } else { return false }
    }
    token, err = decoder.Token(); if err != nil || token != json.Delim('}') || len(outer) != 2 { return false }
    _, err = decoder.Token(); return err == io.EOF
}
