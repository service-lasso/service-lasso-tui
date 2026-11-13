package api

import (
    "bytes"
    "context"
    "crypto/sha256"
    "encoding/hex"
    "encoding/json"
    "errors"
    "io"
    "unicode/utf8"
    "net/http"
    "regexp"
)

// ReconciliationContext admits only the reviewed server-issued context. Core
//2633 lacks this endpoint, so persistent restoration remains unavailable there.
// No URL, profile or credential participates in the durable binding.
func (c *Client) ReconciliationContext() (binding string, admitted bool, failure error) {
    ctx, cancel := context.WithTimeout(context.Background(), requestTimeout)
    defer cancel()
 occurrence,enrollmentErr:=c.beginRequest(ctx,http.MethodGet,"/api/operator/lifecycle/reconciliation-context",nil);if enrollmentErr!=nil{return "",false,enrollmentErr};defer func(){occurrence.Result=binding;occurrence.Failure=failure}()
    req, err := http.NewRequestWithContext(ctx, http.MethodGet, c.baseURL+"/api/operator/lifecycle/reconciliation-context", nil)
    if err != nil { return "", false, &OperatorError{Stage:"context",cause:err} }
    if c.operatorToken != "" && c.authMode == AuthModeLocalAdmin { req.Header.Set("x-service-lasso-admin-token", c.operatorToken) } else if c.operatorToken != "" && c.authMode == AuthModeOAuthBearer { req.Header.Set("Authorization", "Bearer "+c.operatorToken) }
    response, err := c.http.Do(req)
    occurrence.Response=response
    if err != nil { return "", false, operatorTransportFailure("context",response,err) }
    observation:=&operatorResponseObservation{Response:response,Body:response.Body,Status:response.StatusCode}
    defer func(){
        failure=retainOperatorResponse(failure,observation)
        observation.CloseAttempted=true
        closeErr:=response.Body.Close();observation.CloseError=closeErr
        if closeErr!=nil{binding="";admitted=false;failure=errors.Join(failure,&OperatorError{Stage:"context",cause:closeErr,response:observation})}
    }()
    // The historical qualified Core has no such endpoint. Absence is distinct
    // from an actual failed context read and does not disable useful local work.
    if response.StatusCode == http.StatusNotFound { return "", false, nil }
    observation.Limit=4096;observation.ReadAttempted=true
    raw, err := io.ReadAll(io.LimitReader(response.Body, 4097));observation.Raw=raw;observation.ReadError=err
    if response.StatusCode != http.StatusOK { return "", false, &OperatorError{Stage:"http",Status:response.StatusCode,cause:err} }
    if err != nil { return "", false, &OperatorError{Stage:"context",cause:err} }
    if len(raw) > 4096 || !utf8.Valid(raw) { return "", false, &OperatorError{Stage:"context"} }
    closed,err:=closedContextJSON(raw)
    if !closed { return "", false, &OperatorError{Stage:"context",cause:err} }
    var value struct {
        ContractVersion string `json:"contractVersion"`
        Context struct {
            Instance string `json:"instanceBinding"`
            Workspace string `json:"workspaceBinding"`
            Actor string `json:"actorBinding"`
            Client string `json:"clientBinding"`
        } `json:"context"`
    }
    occurrence.Destination=&value
    decoder := json.NewDecoder(bytes.NewReader(raw)); decoder.DisallowUnknownFields()
    if err:=decoder.Decode(&value);err!=nil{return "",false,&OperatorError{Stage:"context",cause:err}}
    if value.ContractVersion != "service-lasso-durable-reconciliation-context.v1" { return "", false, &OperatorError{Stage:"context"} }
    pattern := regexp.MustCompile(`^slrc_[a-f0-9]{64}$`)
    for _, binding := range []string{value.Context.Instance, value.Context.Workspace, value.Context.Actor, value.Context.Client} { if !pattern.MatchString(binding) { return "", false, &OperatorError{Stage:"context"} } }
    canonical, err := json.Marshal(value)
    if err != nil { return "", false, &OperatorError{Stage:"context",cause:err} }
    digest := sha256.Sum256(canonical)
    return "slrc1_"+hex.EncodeToString(digest[:]), true, nil
}

// Context has exactly one outer object and one nested object. Count keys before
// decoding so repeated identical keys cannot silently overwrite authority.
func closedContextJSON(raw []byte) (bool,error) {
    decoder := json.NewDecoder(bytes.NewReader(raw))
    token, err := decoder.Token(); if err != nil || token != json.Delim('{') { return false,err }
    outer := map[string]bool{}
    for decoder.More() {
        token, err = decoder.Token(); key, ok := token.(string)
        if err != nil || !ok || outer[key] { return false,err }; outer[key] = true
        if key == "contractVersion" { token, err = decoder.Token(); if _, ok = token.(string); err != nil || !ok { return false,err } } else if key == "context" {
            token, err = decoder.Token(); if err != nil || token != json.Delim('{') { return false,err }
            nested := map[string]bool{}
            for decoder.More() {
                token, err = decoder.Token(); name, valid := token.(string)
                if err != nil || !valid || nested[name] { return false,err }; nested[name] = true
                if name != "instanceBinding" && name != "workspaceBinding" && name != "actorBinding" && name != "clientBinding" { return false,nil }
                token, err = decoder.Token(); if _, valid = token.(string); err != nil || !valid { return false,err }
            }
            token, err = decoder.Token(); if err != nil || token != json.Delim('}') || len(nested) != 4 { return false,err }
        } else { return false,nil }
    }
    token, err = decoder.Token(); if err != nil || token != json.Delim('}') || len(outer) != 2 { return false,err }
    _, err = decoder.Token(); if err == io.EOF{return true,nil}; return false,err
}
