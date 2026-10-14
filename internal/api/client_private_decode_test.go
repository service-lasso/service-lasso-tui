package api

import (
	"context"
	"encoding/json"
	"errors"
	"io"
	"net/http"
	"net/url"
	"strings"
	"testing"
)

// Source-authored cases remain UNRUN until entire-source/command admission.
func TestInvalidURLPreservesOriginalPrivately(t *testing.T){
	_,failure:=parseBaseURL("http://127.0.0.1/%private-original-url")
	var original *url.Error
	if !errors.As(failure,&original){t.Fatal("actual URL parser error lost")}
	encoded,err:=json.Marshal(failure);if err!=nil{t.Fatal(err)}
	for _,projection:=range []string{failure.Error(),SafeError(failure),string(encoded)}{if strings.Contains(projection,"private-original-url"){t.Fatal("URL original leaked")}}
	_,semantic:=parseBaseURL("relative")
	if semantic==nil||errors.Unwrap(semantic)!=nil{t.Fatal("semantic URL refusal invented a cause")}
}

func TestOperationDecoderRetainsTypedProgressAndOutcomeFailures(t *testing.T){
	for _,raw:=range []string{
		`{"operationId":"mcp-operation-exact","action":"service_start","status":"running","phase":"running","progress":"private-progress","outcome":null,"cancellationSupported":false,"ownership":"own","targetIds":["root"]}`,
		`{"operationId":"mcp-operation-exact","action":"service_start","status":"running","phase":"running","progress":0,"outcome":7,"cancellationSupported":false,"ownership":"own","targetIds":["root"]}`,
	}{
		value,failure:=parseOperation(json.RawMessage(raw))
		var original *json.UnmarshalTypeError
		if value.ID!=""||!errors.As(failure,&original){t.Fatal("typed operation decode failure lost or admitted operation")}
		if strings.Contains(SafeError(failure),"private-progress"){t.Fatal("typed decode original leaked")}
	}
}

func TestAvailabilityReasonRetainsActualTypeError(t *testing.T){
	entries:=[]map[string]any{}
	for _,action:=range []string{"install","config","start","stop","restart","reload"}{
		permission:="service-lasso:lifecycle:write";if action=="install"||action=="config"{permission="service-lasso:config:write"}
		var reason any=nil;if action=="install"{reason=7}
		entries=append(entries,map[string]any{"action":action,"available":false,"requiresConfirmation":true,"reason":reason,"permission":permission})
	}
	raw,err:=json.Marshal(map[string]any{"contractVersion":"service-lasso-durable-lifecycle-operation.v1","serviceId":"root","actions":entries})
	if err!=nil{t.Fatal(err)}
	client,err:=NewClient("http://127.0.0.1:1",&http.Client{Transport:retainedErrorTransport(func(*http.Request)(*http.Response,error){return &http.Response{StatusCode:200,Header:http.Header{},Body:io.NopCloser(strings.NewReader(string(raw)))},nil})},"")
	if err!=nil{t.Fatal(err)}
	actions,failure:=client.LifecycleAvailability(context.Background(),"root")
	var original *json.UnmarshalTypeError
	if actions!=nil||!errors.As(failure,&original){t.Fatal("reason decode original lost or actions admitted")}
}

func TestCompatibilityEffectsPreserveCoreUnicodeAndUTF16Boundary(t *testing.T){
	for _,effect:=range []string{"配置服务",strings.Repeat("é",300),strings.Repeat("😀",150),strings.Repeat("a",300)}{
		if !safeTargetEffects([]string{"root"},[]string{effect}){t.Fatal("useful Core Unicode effect rejected")}
	}
	for _,effect:=range []string{strings.Repeat("😀",151),strings.Repeat("a",301),"line\ncontrol","escape\x1bcontrol",string([]byte{0xff})}{
		if safeTargetEffects([]string{"root"},[]string{effect}){t.Fatal("invalid or oversized effect admitted")}
	}
	if safeTargetEffects([]string{"root","root"},[]string{"useful"}){t.Fatal("duplicate target admitted")}
}
