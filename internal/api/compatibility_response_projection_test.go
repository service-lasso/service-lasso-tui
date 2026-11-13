package api

import (
 "bytes"
 "context"
 "encoding/json"
 "errors"
 "net/http"
 "net/url"
 "strings"
 "testing"
)

// Actual compatibility client source cases, UNRUN. Controlled HTTP replies
// are not genuine Core authority, complete capture or native qualification.
func compatibilityProjectionCall(client *Client,route string)error{
 ctx:=context.Background()
 switch route{
 case "services":_,err:=client.Services(ctx);return err
 case "inbox":_,err:=client.Inbox(ctx,"");return err
 case "history":_,err:=client.HealthHistory(ctx,"echo");return err
 case "availability":_,err:=client.LifecycleAvailability(ctx,"echo");return err
 case "preview":_,err:=client.PreviewLifecycle(ctx,"echo","start");return err
 case "submit":_,err:=client.SubmitLifecycle(ctx,LifecyclePreview{Action:"start",ServiceID:"echo",ConfirmationID:wireConfirmation,ConfirmationPhrase:"confirm start",Targets:[]string{"echo"},Effects:[]string{"Start service."}},"tui-0123456789abcdef0123456789abcdef");return err
 case "operation":_,err:=client.Operation(ctx,wireID);return err
 case "cancel":_,err:=client.CancelOperation(ctx,wireID);return err
 };panic("unknown source fixture route")
}
func TestCompatibilitySemanticProjectionKeepsOriginalResponseInEveryCaller(t *testing.T){
 for _,route:=range []string{"services","inbox","history","availability","preview","submit","operation","cancel"}{t.Run(route,func(t *testing.T){
  raw:=[]byte(`{"private-response-marker":"decoded but rejected projection"}`);body:=&observedFailureBody{reader:bytes.NewReader(raw)}
  status:=200;if route=="submit"{status=202};returned:=&http.Response{StatusCode:status,Header:http.Header{},Body:body};calls:=0
  client,err:=NewClient("http://127.0.0.1:1",&http.Client{Transport:retainedErrorTransport(func(*http.Request)(*http.Response,error){calls++;return returned,nil})},"");if err!=nil{t.Fatal(err)}
  err=compatibilityProjectionCall(client,route);assertPrivateResponse(t,err,body,raw,maxResponseBytes)
  if privateResponseObservation(err).Response!=returned||calls!=1||errors.Unwrap(err)!=nil{t.Fatal("semantic projection lost actual response, replayed request or invented cause")}
 })}
}
func TestCompatibilityProjectionCallersKeepResponseWhenDoAlsoReturnsError(t *testing.T){
 for _,route:=range []string{"services","inbox","history","availability","preview","submit","operation","cancel"}{t.Run(route,func(t *testing.T){
  raw:=[]byte("private-response-marker redirect body");body:=&observedFailureBody{reader:bytes.NewReader(raw)};original:=errors.New("private-response-marker redirect refusal");calls,redirects:=0,0
  returned:=&http.Response{StatusCode:302,Header:http.Header{"Location":[]string{"http://127.0.0.1:1/redirect-target"}},Body:body,ContentLength:int64(len(raw))}
  client,err:=NewClient("http://127.0.0.1:1",&http.Client{Transport:retainedErrorTransport(func(*http.Request)(*http.Response,error){calls++;return returned,nil})},"");if err!=nil{t.Fatal(err)}
  // Test-only rejection exercises http.Client's real simultaneous return;
  // production still selects ErrUseLastResponse and never follows redirects.
  client.http.CheckRedirect=func(*http.Request,[]*http.Request)error{redirects++;return original}
  err=compatibilityProjectionCall(client,route);observation:=privateResponseObservation(err);var transport *url.Error
  if !errors.As(err,&transport)||transport.Err!=original||errors.Unwrap(err)!=transport||!errors.Is(err,original)||observation==nil||observation.Response!=returned||observation.Body!=body{t.Fatal("caller-local nil observation erased actual transport response or original error")}
  if observation.ReadAttempted||observation.CloseAttempted||observation.Raw!=nil||observation.Limit!=0||calls!=1||redirects!=1||body.closed!=1||body.reads!=body.readsAtClose{t.Fatal("caller fabricated observation or reread/reclosed client-closed response")}
  encoded,marshalErr:=json.Marshal(err);if marshalErr!=nil||strings.Contains(string(encoded),"private-response-marker")||strings.Contains(string(encoded),"Response")||strings.Contains(SafeError(err),"private-response-marker")||strings.Contains(err.Error(),"private-response-marker"){t.Fatal("private response/error entered ordinary output")}
 })}
}
func TestCompatibilityNestedProjectionKeepsActualDecoderCauseAndResponse(t *testing.T){
 for _,route:=range []string{"availability","submit","operation","cancel"}{t.Run(route,func(t *testing.T){
  var value any
  if route=="availability"{current:=wireAvailability(false);current["actions"].([]any)[0].(map[string]any)["reason"]=map[string]any{"private-response-marker":true};value=current}else{
   operation:=wireOperation("service_start","running",[]string{"echo"},false);operation["outcome"]=map[string]any{"private-response-marker":true}
   envelope:=map[string]any{"contractVersion":"service-lasso-mcp-operation.v1","operation":operation}
   if route=="submit"{envelope["contractVersion"]="service-lasso-mcp-operation-accepted.v1";envelope["accepted"]=true}
   if route=="cancel"{envelope["cancellation"]=map[string]any{"result":"unsupported","terminal":false}}
   value=envelope
  }
  raw,marshalErr:=json.Marshal(value);if marshalErr!=nil{t.Fatal(marshalErr)};body:=&observedFailureBody{reader:bytes.NewReader(raw)};status:=200;if route=="submit"{status=202}
  returned:=&http.Response{StatusCode:status,Header:http.Header{},Body:body}
  client,err:=NewClient("http://127.0.0.1:1",&http.Client{Transport:retainedErrorTransport(func(*http.Request)(*http.Response,error){return returned,nil})},"");if err!=nil{t.Fatal(err)}
  err=compatibilityProjectionCall(client,route);assertPrivateResponse(t,err,body,raw,maxResponseBytes)
  var original *json.UnmarshalTypeError;if !errors.As(err,&original)||!errors.Is(err,original)||errors.Unwrap(err)!=original||privateResponseObservation(err).Response!=returned{t.Fatal("nested original library error or response replaced")}
 })}
}
func TestCompatibilityUsefulSuccessfulProjectionsRemainUnchanged(t *testing.T){
 for _,route:=range []string{"services","operation"}{t.Run(route,func(t *testing.T){
  value:=any(map[string]any{"services":[]any{map[string]any{"id":"echo"}}});if route=="operation"{value=map[string]any{"contractVersion":"service-lasso-mcp-operation.v1","operation":wireOperation("service_start","running",[]string{"echo"},false)}}
  raw,marshalErr:=json.Marshal(value);if marshalErr!=nil{t.Fatal(marshalErr)};body:=&observedFailureBody{reader:bytes.NewReader(raw)}
  client,err:=NewClient("http://127.0.0.1:1",&http.Client{Transport:retainedErrorTransport(func(*http.Request)(*http.Response,error){return &http.Response{StatusCode:200,Header:http.Header{},Body:body},nil})},"");if err!=nil{t.Fatal(err)}
  if route=="services"{rows,failure:=client.Services(context.Background());if failure!=nil||len(rows)!=1||rows[0].ID!="echo"{t.Fatal("useful services projection changed")}}else{operation,failure:=client.Operation(context.Background(),wireID);if failure!=nil||operation.ID!=wireID||operation.Action!="service_start"||operation.Status!="running"{t.Fatal("useful operation projection changed")}}
  if body.closed!=1{t.Fatal("successful response close count changed")}
 })}
}
