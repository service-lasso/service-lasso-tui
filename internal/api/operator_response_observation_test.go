package api

import (
 "bytes"
 "context"
 "encoding/json"
 "errors"
 "io"
 "net/http"
 "net/url"
 "strings"
 "testing"
)

// Actual client paths with controlled bodies; UNRUN source cases, never native
// original capture, complete stream EOF, finite lifetime or durable ACK proof.
type observedFailureBody struct{reader *bytes.Reader;readErr,closeErr error;closed,reads,readsAtClose int}
func(b *observedFailureBody)Read(p []byte)(int,error){b.reads++;if b.reader.Len()>0{return b.reader.Read(p)};if b.readErr!=nil{return 0,b.readErr};return 0,io.EOF}
func(b *observedFailureBody)Close()error{b.closed++;b.readsAtClose=b.reads;return b.closeErr}
func privateResponseObservation(err error)*operatorResponseObservation{
 var safe *OperatorError;if errors.As(err,&safe){return safe.response};var public *OperatorPublicError;if errors.As(err,&public){return public.response};return nil
}
func assertPrivateResponse(t *testing.T,err error,body *observedFailureBody,raw []byte,limit int){
 t.Helper();observation:=privateResponseObservation(err)
 if err==nil||observation==nil||observation.Response==nil||observation.Response.Body!=body||observation.Body!=body||!bytes.Equal(observation.Raw,raw)||observation.Limit!=limit||!observation.ReadAttempted||!observation.CloseAttempted||observation.ReadError!=body.readErr||observation.CloseError!=body.closeErr||body.closed!=1{t.Fatal("original bounded response/body/read/close observation lost")}
 encoded,marshalErr:=json.Marshal(err)
 if marshalErr!=nil||strings.Contains(string(encoded),"private-response-marker")||strings.Contains(string(encoded),"Response")||strings.Contains(string(encoded),"Raw")||strings.Contains(err.Error(),"private-response-marker")||strings.Contains(SafeError(err),"private-response-marker"){t.Fatal("private response entered ordinary presentation")}
}
func TestResponseReaderKeepsOriginalResponseAndReadBufferIdentity(t *testing.T){
 body:=&observedFailureBody{reader:bytes.NewReader([]byte("private-response-marker"))}
 response:=&http.Response{StatusCode:503,Header:http.Header{"X-Private":[]string{"original"}},Body:body}
 raw,err,observation:=operatorReadResponse(response,OperatorErrorBytes)
 if err!=nil||observation.Response!=response||observation.Body!=body||len(raw)==0||len(observation.Raw)!=len(raw)||&raw[0]!=&observation.Raw[0]||body.closed!=1{t.Fatal("actual returned response or io.ReadAll backing bytes replaced")}
}
func TestActualClientRedirectErrorRetainsReturnedResponseWithoutSecondReadOrClose(t *testing.T){
 for _,route:=range []string{"detail","context","compatibility","reconciliation"}{t.Run(route,func(t *testing.T){
  raw:=[]byte("private-response-marker redirect body");body:=&observedFailureBody{reader:bytes.NewReader(raw)}
  original:=errors.New("private-response-marker redirect refusal");calls,redirects:=0,0
  returned:=&http.Response{StatusCode:302,Header:http.Header{"Location":[]string{"http://127.0.0.1:1/redirect-target"}},Body:body,ContentLength:int64(len(raw))}
  client,err:=NewClient("http://127.0.0.1:1",&http.Client{Transport:retainedErrorTransport(func(*http.Request)(*http.Response,error){calls++;return returned,nil})},"");if err!=nil{t.Fatal(err)}
  // Exercise real http.Client's supported response+error return. This test-only
  // callback does not change NewClient's production ErrUseLastResponse policy.
  client.http.CheckRedirect=func(*http.Request,[]*http.Request)error{redirects++;return original}
  switch route{case "detail":_,err=client.OperatorDetail(context.Background(),"root");case "context":_,err=client.OperatorContext(context.Background());case "compatibility":_,err=client.Health(context.Background());case "reconciliation":binding,admitted,failure:=client.ReconciliationContext();if binding!=""||admitted{t.Fatal("redirect error admitted binding")};err=failure}
  observation:=privateResponseObservation(err);var transport *url.Error;var failure *OperatorError
  if !errors.As(err,&transport)||!errors.As(err,&failure)||failure.Unwrap()!=transport||!errors.Is(err,original)||transport.Err!=original{t.Fatal("actual returned URL error or redirect cause replaced")}
  if observation==nil||observation.Response!=returned||observation.Body!=body||observation.Status!=302||observation.Raw!=nil||observation.Limit!=0||observation.ReadAttempted||observation.CloseAttempted||observation.ReadError!=nil||observation.CloseError!=nil{t.Fatal("returned response lost or caller observation fabricated")}
  if calls!=1||redirects!=1||body.closed!=1||body.reads!=body.readsAtClose{t.Fatal("caller followed redirect or reread/reclosed client-closed body")}
  encoded,marshalErr:=json.Marshal(err);if marshalErr!=nil||strings.Contains(string(encoded),"private-response-marker")||strings.Contains(string(encoded),"Response")||strings.Contains(err.Error(),"private-response-marker")||strings.Contains(SafeError(err),"private-response-marker"){t.Fatal("simultaneous response/error leaked into presentation")}
 })}
}
func TestFailedClientsRetainBoundedOriginalResponseAndSeparateReadCloseCauses(t *testing.T){
 raw:=[]byte(`{"private-response-marker":"original bytes"}`)
 readErr:=errors.New("private-response-marker read");closeErr:=errors.New("private-response-marker close")
 for _,route:=range []string{"detail","context","compatibility"}{for _,status:=range []int{200,503}{t.Run(route+http.StatusText(status),func(t *testing.T){
  body:=&observedFailureBody{reader:bytes.NewReader(raw),readErr:readErr,closeErr:closeErr}
  client,err:=NewClient("http://127.0.0.1:1",&http.Client{Transport:retainedErrorTransport(func(*http.Request)(*http.Response,error){return &http.Response{StatusCode:status,Header:http.Header{},Body:body},nil})},"");if err!=nil{t.Fatal(err)}
  limit:=OperatorResponseBytes;if status!=200{limit=OperatorErrorBytes}
  switch route{case "detail":_,err=client.OperatorDetail(context.Background(),"root");case "context":_,err=client.OperatorContext(context.Background());if status==200{limit=OperatorContextBytes};case "compatibility":_,err=client.Health(context.Background());limit=maxResponseBytes}
  assertPrivateResponse(t,err,body,raw,limit)
  if !errors.Is(err,readErr)||!errors.Is(err,closeErr){t.Fatal("actual read/close originals lost through response boundary")}
 })}}
}
func TestResponseObservationPreservesTypedAndDecoderRefusalWithoutInventingSemanticCause(t *testing.T){
 typed:=`{"contractVersion":"`+OperatorContract+`","requestId":"request","error":{"code":"permission_denied","messageKey":"operator.error.permission_denied","retryability":"never","fieldErrors":[]}}`
 for _,current:=range []struct{name,raw string;status int;typed,syntax bool}{
  {"typed",typed,403,true,false},
  {"decoder",`{"private-response-marker":@}`,200,false,true},
  {"semantic",`{"private-response-marker":"source","contractVersion":"wrong"}`,200,false,false},
 }{t.Run(current.name,func(t *testing.T){
  raw:=[]byte(current.raw);body:=&observedFailureBody{reader:bytes.NewReader(raw)}
  client,err:=NewClient("http://127.0.0.1:1",&http.Client{Transport:retainedErrorTransport(func(*http.Request)(*http.Response,error){return &http.Response{StatusCode:current.status,Header:http.Header{},Body:body},nil})},"");if err!=nil{t.Fatal(err)}
  _,err=client.OperatorDetail(context.Background(),"root");limit:=OperatorResponseBytes;if current.status!=200{limit=OperatorErrorBytes};assertPrivateResponse(t,err,body,raw,limit)
  if current.typed{public,ok:=err.(*OperatorPublicError);if !ok||public.Code!="permission_denied"{t.Fatal("original direct public error type changed")}}else{var syntax *json.SyntaxError;if errors.As(err,&syntax)!=current.syntax{t.Fatal("decoder identity or semantic cause distinction changed")};if !current.syntax{safe,ok:=err.(*OperatorError);if !ok||safe.Unwrap()!=nil{t.Fatal("semantic refusal invented a cause")}}}
 })}
}
func TestResponseObservationKeepsLimitPlusOnePartialBytesAndUsefulContextSuccess(t *testing.T){
 raw:=[]byte(strings.Repeat("x",OperatorContextBytes+20));body:=&observedFailureBody{reader:bytes.NewReader(raw)}
 client,err:=NewClient("http://127.0.0.1:1",&http.Client{Transport:retainedErrorTransport(func(*http.Request)(*http.Response,error){return &http.Response{StatusCode:200,Header:http.Header{},Body:body},nil})},"");if err!=nil{t.Fatal(err)}
 _,err=client.OperatorContext(context.Background());assertPrivateResponse(t,err,body,raw[:OperatorContextBytes+1],OperatorContextBytes)
 if body.reader.Len()!=19{t.Fatal("bounded observation unexpectedly consumed complete excess body")}
 binding:="slrc_"+strings.Repeat("a",64);expected:=OperatorContext{InstanceBinding:binding,WorkspaceBinding:binding,ActorBinding:binding,ClientBinding:binding}
 good,marshalErr:=json.Marshal(struct{ContractVersion string `json:"contractVersion"`;Context OperatorContext `json:"context"`}{"service-lasso-durable-reconciliation-context.v1",expected});if marshalErr!=nil{t.Fatal(marshalErr)}
 body=&observedFailureBody{reader:bytes.NewReader(good)}
 actual,err:=client.OperatorContext(context.Background());if err!=nil||actual!=expected||body.closed!=1{t.Fatal("useful successful context read changed")}
}
func TestCompatibilityContextRetainsActualResponseGraphAndUsefulAbsence(t *testing.T){
 for _,status:=range []int{200,503}{t.Run(http.StatusText(status),func(t *testing.T){
  raw:=[]byte("private-response-marker context bytes");readErr:=errors.New("private-response-marker context read");closeErr:=errors.New("private-response-marker context close")
  body:=&observedFailureBody{reader:bytes.NewReader(raw),readErr:readErr,closeErr:closeErr}
  returned:=&http.Response{StatusCode:status,Header:http.Header{},Body:body}
  client,err:=NewClient("http://127.0.0.1:1",&http.Client{Transport:retainedErrorTransport(func(*http.Request)(*http.Response,error){return returned,nil})},"");if err!=nil{t.Fatal(err)}
  binding,admitted,err:=client.ReconciliationContext();assertPrivateResponse(t,err,body,raw,4096)
  if binding!=""||admitted||privateResponseObservation(err).Response!=returned||!errors.Is(err,readErr)||!errors.Is(err,closeErr){t.Fatal("context refusal erased returned originals or admitted binding")}
  if status==503{var failure *OperatorError;if !errors.As(err,&failure)||failure.Stage!="http"||failure.Status!=status{t.Fatal("HTTP refusal lost original safe status")}}
 })}
 for _,raw:=range []string{`{"contractVersion":@}`,`{"unknown":"private-response-marker"}`}{
  body:=&observedFailureBody{reader:bytes.NewReader([]byte(raw))}
  client,err:=NewClient("http://127.0.0.1:1",&http.Client{Transport:retainedErrorTransport(func(*http.Request)(*http.Response,error){return &http.Response{StatusCode:200,Header:http.Header{},Body:body},nil})},"");if err!=nil{t.Fatal(err)}
  _,_,err=client.ReconciliationContext();assertPrivateResponse(t,err,body,[]byte(raw),4096)
  var syntax *json.SyntaxError;if errors.As(err,&syntax)!=strings.Contains(raw,"@") {t.Fatal("context decoder identity or semantic distinction lost")};if !strings.Contains(raw,"@")&&errors.Unwrap(err)!=nil{t.Fatal("context semantic refusal invented a cause")}
 }
 for _,closeFailure:=range []bool{false,true}{
  body:=&observedFailureBody{reader:bytes.NewReader([]byte("private-response-marker absent endpoint"))};if closeFailure{body.closeErr=errors.New("private-response-marker absent close")}
  client,err:=NewClient("http://127.0.0.1:1",&http.Client{Transport:retainedErrorTransport(func(*http.Request)(*http.Response,error){return &http.Response{StatusCode:404,Header:http.Header{},Body:body},nil})},"");if err!=nil{t.Fatal(err)}
  binding,admitted,err:=client.ReconciliationContext();if binding!=""||admitted||body.reads!=0||body.closed!=1||(err!=nil)!=closeFailure{t.Fatal("useful endpoint absence changed or response reread")}
  if closeFailure{observation:=privateResponseObservation(err);if observation==nil||observation.Body!=body||observation.Raw!=nil||observation.Limit!=0||observation.ReadAttempted||!observation.CloseAttempted||observation.ReadError!=nil||observation.CloseError!=body.closeErr||!errors.Is(err,body.closeErr){t.Fatal("actual absent-endpoint close original lost or read fabricated")}}
 }
 opaque:="slrc_"+strings.Repeat("a",64);expected:=OperatorContext{InstanceBinding:opaque,WorkspaceBinding:opaque,ActorBinding:opaque,ClientBinding:opaque}
 good,marshalErr:=json.Marshal(struct{ContractVersion string `json:"contractVersion"`;Context OperatorContext `json:"context"`}{"service-lasso-durable-reconciliation-context.v1",expected});if marshalErr!=nil{t.Fatal(marshalErr)}
 body:=&observedFailureBody{reader:bytes.NewReader(good)}
 client,err:=NewClient("http://127.0.0.1:1",&http.Client{Transport:retainedErrorTransport(func(*http.Request)(*http.Response,error){return &http.Response{StatusCode:200,Header:http.Header{},Body:body},nil})},"");if err!=nil{t.Fatal(err)}
 binding,admitted,err:=client.ReconciliationContext();if err!=nil||!admitted||binding!=expected.LegacyDigest()||body.closed!=1{t.Fatal("useful successful original compatibility binding changed")}
}
