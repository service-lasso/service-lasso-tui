package api

import (
	"bytes"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net/http"
	"unicode/utf8"
)

// OperatorError presents only fixed safe fields. Its private cause preserves the
// original failure for owned diagnostics/Unwrap, never Error or JSON output.
type OperatorError struct { Stage string; Status int; Method, Route string; cause error; response *operatorResponseObservation; input *connectionInputObservation }
func(e *OperatorError)Unwrap()error{return e.cause}
// Client-side library failures retain their original object with the same
// fixed invalid_request presentation. A semantic refusal has no original cause.
func OperatorInvalidRequestCause(cause error)error{if cause==nil{return &OperatorPublicError{Code:"invalid_request"}};return &OperatorError{Stage:"invalid_request",cause:cause}}
// Keep storage originals reachable privately while Error and JSON stay closed.
func OperatorStorageCause(cause error)error{return &OperatorError{Stage:"storage",cause:cause}}
// A failed post-read context check keeps its original object under the fixed
// reconciliation stage without publishing the underlying transport detail.
func OperatorContextCause(cause error)error{return &OperatorError{Stage:"context",cause:cause}}

func (e *OperatorError) Error() string {
	switch e.Stage {
	case "http":
		method := "request"; if e.Method == "GET" || e.Method == "POST" { method = e.Method }
		route := "runtime API"; switch e.Route { case "services", "health", "lifecycle", "inbox", "setup", "runtime": route = e.Route }
		return fmt.Sprintf("%s %s: runtime returned %d %s; check connection permissions", method, route, e.Status, http.StatusText(e.Status))
	case "invalid_request": return OperatorMessage("invalid_request")
	case "binding_stale": return OperatorMessage("binding_stale")
	case "transport": return "runtime connection failed; check connection and retry reads"
	case "response": return "runtime response rejected; check supported API version"
	case "storage": return "operation recovery storage unavailable; retained operation was not replayed"
	case "configuration": return "connection configuration unavailable; check profile settings"
	case "context": return "operation actor or runtime context changed; reconciliation required"
	case "permission": return "Core action unavailable; check lifecycle permission and service prerequisites"
	default: return "operator stage failed; check connection and retained operation state"
	}
}

// SafeError also closes errors from injected clients, stores and terminal drivers.
func SafeError(err error) string {
	if err == nil { return "" }
	// A safe stage wrapper owns presentation even when its private cause happens
	// to contain another typed error. Direct public server errors remain useful.
	var classified *OperatorError
	if errors.As(err, &classified) { return classified.Error() }
	var public *OperatorPublicError
	if errors.As(err,&public){return OperatorMessage(public.Code)}
	var configuration *ConfigurationError
	if errors.As(err, &configuration) { return configuration.Error() }
	return (&OperatorError{Stage: "unknown"}).Error()
}

func ErrorTitle(err error) string {
	var stage *OperatorError
	if errors.As(err, &stage) { switch { case stage.Stage == "http" && stage.Status == 401: return "Runtime authentication denied"; case stage.Stage == "http" && stage.Status == 403: return "Runtime permission denied"; case stage.Stage == "permission": return "Core action unavailable"; case stage.Stage == "context": return "Operation reconciliation required"; case stage.Stage == "storage": return "Operation recovery storage unavailable" } }
	return "Runtime API unavailable"
}

const maxResponseBytes = 1 << 20

func decodeResponse(raw []byte, destination any) error {
 if len(raw)==0||len(raw)>maxResponseBytes||!utf8.Valid(raw){return &OperatorError{Stage:"response"}}
 d:=json.NewDecoder(bytes.NewReader(raw));d.UseNumber()
 admitted,err:=boundedValue(d,0);if !admitted{return &OperatorError{Stage:"response",cause:err}}
 if _,err:=d.Token();err!=io.EOF{return &OperatorError{Stage:"response",cause:err}}
 if err:=json.Unmarshal(raw,destination);err!=nil{return &OperatorError{Stage:"response",cause:err}}
 return nil
}

// Semantic and bound refusals have no manufactured original error. Only actual
// decoder failures carry the original object through recursive admission.
func boundedValue(d *json.Decoder,depth int)(bool,error){
 if depth>16{return false,nil}
 t,err:=d.Token();if err!=nil{return false,err}
 switch value:=t.(type){
 case string:return len(value)<=4096,nil
 case json.Delim:
  if value!='{'&&value!='['{return false,nil}
  keys:=map[string]bool{};count:=0
  for d.More(){
   count++;if count>1024{return false,nil}
   if value=='{'{
    keyToken,err:=d.Token();if err!=nil{return false,err};key,ok:=keyToken.(string)
    if !ok||len(key)>128||keys[key]{return false,nil};keys[key]=true
   }
   admitted,err:=boundedValue(d,depth+1);if !admitted{return false,err}
  }
  end,err:=d.Token();if err!=nil{return false,err}
  return (value=='{'&&end==json.Delim('}'))||(value=='['&&end==json.Delim(']')),nil
 default:return true,nil
 }
}

func ReasonLabel(reason string) string {
	switch reason {
	case "": return "available"
	case "forbidden", "scope_denied", "insufficient_scope", "insufficient_profile", "permission_denied": return "permission denied"
	case "durable_operation_unavailable": return "durable operation unavailable"
	case "service_already_running", "service_not_running", "service_already_installed", "service_not_startable", "service_start_preflight_blocked", "restart_preflight_blocked", "configuration_preflight_blocked": return "service prerequisite not met"
	default: return "Core action unavailable"
	}
}

func coreAction(action string) string { if action == "config" { return "service_configure" }; return "service_"+action }

func terminalStatus(status string) bool {
	switch status { case "succeeded", "failed", "cancelled", "skipped", "unknown_after_crash": return true }; return false
}

// MatchOperation admits Core's initial singleton and frozen preflight expansion.
func MatchOperation(operation Operation, preview LifecyclePreview) bool {
	if operation.Action != coreAction(preview.Action) || operation.Ownership != "own" { return false }
	allowed := map[string]bool{preview.ServiceID: true}
	for _, id := range preview.Targets { allowed[id] = true }
	for _, id := range operation.Targets { if !allowed[id] { return false } }
	return true
}
