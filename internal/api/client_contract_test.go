package api

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
)

const wireID = "mcp-operation-12345678-1234-1234-1234-123456789abc"
const wireConfirmation = "mcp-confirmation-12345678-1234-1234-1234-123456789abc"

func wireOperation(action, status string, targets []string, supported bool) map[string]any {
	var outcome any; progress := 50
	if terminalStatus(status) { outcome, progress = status, 100 }
	return map[string]any{"operationId": wireID, "action": action, "status": status, "phase": "executing", "progress": progress, "outcome": outcome, "cancellationSupported": supported, "ownership": "own", "targetIds": targets}
}

func wireClient(t *testing.T, reply string, status int) *Client {
	t.Helper()
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { w.WriteHeader(status); _, _ = w.Write([]byte(reply)) }))
	t.Cleanup(server.Close)
	client, err := NewClient(server.URL, server.Client(), "inert-token"); if err != nil { t.Fatal(err) }; return client
}

func wireJSON(value any) string { raw, _ := json.Marshal(value); return string(raw) }

func wireAvailability(extensions bool) map[string]any {
	var actions []any
	for _, name := range []string{"install", "config", "start", "stop", "restart", "reload"} {
		var reason any; available := name != "reload"; if !available { reason = "durable_operation_unavailable" }
		actions = append(actions, map[string]any{"action": name, "available": available, "reason": reason, "permission": actionPermission(name), "requiresConfirmation": true})
	}
	if extensions { for _, name := range []string{"update_check", "update_download"} { actions = append(actions, map[string]any{"action": name, "available": true, "reason": nil, "permission": actionPermission(name), "requiresConfirmation": name != "update_check"}) } }
	return map[string]any{"contractVersion": "service-lasso-durable-lifecycle-operation.v1", "serviceId": "echo", "actions": actions}
}

func TestAuthenticPinnedAndCurrentAvailabilityProjection(t *testing.T) {
	for _, extensions := range []bool{false, true} { t.Run(fmt.Sprint(extensions), func(t *testing.T) {
		client := wireClient(t, wireJSON(wireAvailability(extensions)), 200)
		actions, err := client.LifecycleAvailability(context.Background(), "echo")
		if err != nil || len(actions) != 6 { t.Fatalf("projection=%#v err=%v", actions, err) }
		for _, action := range actions { if strings.HasPrefix(action.Action, "update") { t.Fatal("unsupported extension advertised") }; if action.Action != "reload" && !action.Available { t.Fatal("supported action blocked") } }
	}) }
}

func TestAvailabilityAuthorityFailures(t *testing.T) {
	for _, fault := range []string{"version", "service", "duplicate", "missing", "unknown", "permission", "bool", "reason", "conflict", "unpaired"} { t.Run(fault, func(t *testing.T) {
		value := wireAvailability(false); actions := value["actions"].([]any)
		switch fault {
		case "version": value["contractVersion"] = "unknown.v2"
		case "service": value["serviceId"] = "other"
		case "duplicate": actions = append(actions, actions[0])
		case "missing": actions = actions[1:]
		case "unknown": actions[0].(map[string]any)["action"] = "delete"
		case "permission": actions[0].(map[string]any)["permission"] = "service-lasso:lifecycle:write"
		case "bool": delete(actions[0].(map[string]any), "available")
		case "reason": delete(actions[0].(map[string]any), "reason")
		case "conflict": actions[0].(map[string]any)["reason"] = "forbidden"
		case "unpaired": actions = append(actions, map[string]any{"action":"update_check", "available":true, "reason":nil, "permission":"service-lasso:update:write", "requiresConfirmation":false})
		}; value["actions"] = actions
		if _, err := wireClient(t, wireJSON(value), 200).LifecycleAvailability(context.Background(), "echo"); err == nil { t.Fatal("authority admitted") }
	}) }
}

func TestAllFiveSubmittedTuplesAndCoreTargetExpansion(t *testing.T) {
	for _, action := range []string{"install", "config", "start", "stop", "restart"} { for _, targets := range [][]string{{"echo"}, {"dependency", "echo"}} { t.Run(action+strings.Join(targets,"-"), func(t *testing.T) {
		preview := LifecyclePreview{Action: action, ServiceID: "echo", ConfirmationID: wireConfirmation, ConfirmationPhrase: "confirm " + action, Targets: []string{"dependency", "echo"}, Effects: []string{"Apply action."}}
		value := map[string]any{"contractVersion":"service-lasso-mcp-operation-accepted.v1", "accepted":true, "operation":wireOperation(coreAction(action), "running", targets, false)}
		operation, err := wireClient(t, wireJSON(value), 202).SubmitLifecycle(context.Background(), preview, "tui-0123456789abcdef0123456789abcdef")
		if err != nil || operation.Action != coreAction(action) || !MatchOperation(operation, preview) { t.Fatalf("authentic tuple denied: %#v %v", operation, err) }
	}) } }
}

func TestOperationAndEnvelopeAuthorityFailures(t *testing.T) {
	for _, fault := range []string{"id", "action", "target", "ownership", "status", "outcome", "missingoutcome", "progress", "missingprogress", "missingcancel", "duplicateTargets", "version", "missingVersion", "trailing", "duplicateJSON"} { t.Run(fault, func(t *testing.T) {
		op := wireOperation("service_start", "running", []string{"echo"}, false)
		envelope := map[string]any{"contractVersion":"service-lasso-mcp-operation-accepted.v1", "accepted":true, "operation":op}
		switch fault {
		case "id": op["operationId"] = "mcp-operation-random"
		case "action": op["action"] = "service_stop"
		case "target": op["targetIds"] = []string{"other"}
		case "ownership": op["ownership"] = "other"
		case "status": op["status"] = "accepted"
		case "outcome": op["outcome"] = "succeeded"
		case "missingoutcome": delete(op,"outcome")
		case "progress": op["progress"] = 101
		case "missingprogress": delete(op,"progress")
		case "missingcancel": delete(op,"cancellationSupported")
		case "duplicateTargets": op["targetIds"] = []string{"echo","echo"}
		case "version": envelope["contractVersion"] = "unknown.v2"
		case "missingVersion": delete(envelope,"contractVersion")
		}
		raw := wireJSON(envelope); if fault == "trailing" { raw += "{}" }; if fault == "duplicateJSON" { raw = strings.Replace(raw, `"ownership":"own"`, `"ownership":"own","ownership":"own"`, 1) }
		preview := LifecyclePreview{Action:"start", ServiceID:"echo", ConfirmationID:wireConfirmation, ConfirmationPhrase:"confirm start", Targets:[]string{"echo"}, Effects:[]string{"Start service."}}
		if _, err := wireClient(t, raw, 202).SubmitLifecycle(context.Background(), preview, "tui-0123456789abcdef0123456789abcdef"); err == nil { t.Fatal("invalid submission admitted") }
	}) }
}

func TestAuthenticTopLevelCancellationShapes(t *testing.T) {
	for _, test := range []struct{ result, status, action string; supported, terminal bool }{
		{"requested", "cancelling", "update_check", true, false},
		{"requested", "cancelled", "update_download", true, true},
		{"unsupported", "running", "service_start", false, false},
		{"too_late", "succeeded", "service_start", false, true},
	} { t.Run(test.result+test.status,func(t *testing.T) {
		value := map[string]any{"contractVersion":"service-lasso-mcp-operation.v1", "operation":wireOperation(test.action,test.status,[]string{"echo"},test.supported), "cancellation":map[string]any{"result":test.result,"terminal":test.terminal}}
		operation, err := wireClient(t, wireJSON(value), 200).CancelOperation(context.Background(),wireID)
		if err != nil || operation.CancellationResult != test.result || operation.CancellationTerminal != test.terminal { t.Fatalf("wire=%#v err=%v",operation,err) }
	}) }
}

func TestCancellationRejectsNestedFakeMismatchAndMissingAuthority(t *testing.T) {
	for _, fault := range []string{"nested", "id", "terminal", "result", "missing", "duplicate", "trailing", "ownership"} { t.Run(fault,func(t *testing.T) {
		op := wireOperation("service_start","running",[]string{"echo"},false)
		cancel := map[string]any{"result":"unsupported","terminal":false}
		value := map[string]any{"contractVersion":"service-lasso-mcp-operation.v1","operation":op,"cancellation":cancel}
		switch fault { case "nested": delete(value,"operation"); cancel["operation"]=op; case "id": op["operationId"]="mcp-operation-aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"; case "terminal": cancel["terminal"]=true; case "result": cancel["result"]="invented"; case "missing": delete(cancel,"terminal"); case "ownership":op["ownership"]="other" }
		raw:=wireJSON(value); if fault=="duplicate" { raw=strings.Replace(raw,`"terminal":false`,`"terminal":false,"terminal":false`,1) }; if fault=="trailing" {raw+="null"}
		if _,err:=wireClient(t,raw,200).CancelOperation(context.Background(),wireID);err==nil {t.Fatal("invalid cancellation admitted")}
	}) }
}

type sentinelTransport struct{}
func (sentinelTransport) RoundTrip(*http.Request)(*http.Response,error){return nil,errors.New("SECRET https://private/token /private/path \x1b[2J")}
func TestTransportAndDecodeStagesNeverLeakSentinels(t *testing.T){
	client,_:=NewClient("http://127.0.0.1:17883",&http.Client{Transport:sentinelTransport{}},"SECRET")
	_,err:=client.Services(context.Background()); if err==nil || strings.Contains(err.Error(),"SECRET") || strings.Contains(err.Error(),"private") || strings.Contains(err.Error(),"\x1b") {t.Fatalf("transport leak: %v",err)}
	for _,raw:=range []string{`{"services":"SECRET /private/path"}`,strings.Repeat(" ",maxResponseBytes+1),`{"services":[],"services":[]}`,`{"services":[]}[]`,`{"services":[],"nested":`+strings.Repeat("[",18)+`0`+strings.Repeat("]",18)+`}`,`{"services":[],"text":"`+strings.Repeat("x",4097)+`"}`,`{"services":[],"list":[`+strings.Repeat("0,",1024)+`0]}`} {
		_,err=wireClient(t,raw,200).Services(context.Background()); if err==nil || strings.Contains(err.Error(),"SECRET") || strings.Contains(err.Error(),"private") {t.Fatalf("response admitted or leak: %v",err)}
	}
}

func TestHistoryBindsServiceAndCollectionQuota(t *testing.T){
	for _,raw:=range []string{`{"serviceId":"other","history":{"transitions":[]}}`,`{"serviceId":"echo","history":{"transitions":[`+strings.Repeat("{},",256)+`{}]}}`} {if _,err:=wireClient(t,raw,200).HealthHistory(context.Background(),"echo");err==nil {t.Fatal("wrong or unbounded history admitted")}}
}
