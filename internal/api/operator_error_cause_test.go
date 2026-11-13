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

type retainedErrorTransport func(*http.Request)(*http.Response,error)
func(f retainedErrorTransport)RoundTrip(r *http.Request)(*http.Response,error){return f(r)}
type retainedErrorReader struct{cause error}
func(r retainedErrorReader)Read([]byte)(int,error){return 0,r.cause}
type retainedCloseBody struct{reader io.Reader;cause error;closed int}
func(r *retainedCloseBody)Read(p []byte)(int,error){return r.reader.Read(p)}
func(r *retainedCloseBody)Close()error{r.closed++;return r.cause}

// Actual context and generic Detail client paths retain original failures;
// source-only fixtures do not prove native stream/close/capture qualification.
func TestOperatorClientPreservesPrivateTransportAndReadCause(t *testing.T){
 cause:=errors.New("private cause https://secret.example/token?key=raw")
 for _,transportFailure:=range []bool{true,false}{for _,route:=range []string{"context","detail"}{t.Run(route+map[bool]string{true:" transport",false:" body"}[transportFailure],func(t *testing.T){
  client,err:=NewClient("http://127.0.0.1:1",&http.Client{Transport:retainedErrorTransport(func(*http.Request)(*http.Response,error){if transportFailure{return nil,cause};return &http.Response{StatusCode:200,Header:http.Header{},Body:io.NopCloser(retainedErrorReader{cause})},nil})},"source-token");if err!=nil{t.Fatal(err)}
  if route=="context"{_,err=client.OperatorContext(context.Background())}else{_,err=client.OperatorDetail(context.Background(),"root")}
  if !errors.Is(err,cause){t.Fatal("actual client discarded original cause")};var safe *OperatorError;if !errors.As(err,&safe){t.Fatal("missing safe stage")}
  encoded,marshalErr:=json.Marshal(safe);if marshalErr!=nil||strings.Contains(string(encoded),cause.Error())||strings.Contains(err.Error(),cause.Error())||strings.Contains(SafeError(err),cause.Error()){t.Fatal("private cause entered ordinary output")}
 })}}
}
func TestOperatorClientRetainsReadAndCloseCauses(t *testing.T){
 readCause:=errors.New("private read https://secret.example/read")
 closeCause:=errors.New("private close https://secret.example/close")
 for _,readFailure:=range []bool{true,false}{for _,route:=range []string{"context","detail"}{t.Run(route+map[bool]string{true:" read and close",false:" close"}[readFailure],func(t *testing.T){
  var reader io.Reader=strings.NewReader("{}");if readFailure{reader=retainedErrorReader{readCause}}
  body:=&retainedCloseBody{reader:reader,cause:closeCause}
  client,err:=NewClient("http://127.0.0.1:1",&http.Client{Transport:retainedErrorTransport(func(*http.Request)(*http.Response,error){return &http.Response{StatusCode:200,Header:http.Header{},Body:body},nil})},"source-token");if err!=nil{t.Fatal(err)}
  if route=="context"{_,err=client.OperatorContext(context.Background())}else{_,err=client.OperatorDetail(context.Background(),"root")}
  if body.closed!=1||!errors.Is(err,closeCause)||readFailure&&!errors.Is(err,readCause){t.Fatal("actual response discarded a read/close cause or did not close exactly once")}
  var safe *OperatorError;if !errors.As(err,&safe){t.Fatal("missing safe projection")};encoded,marshalErr:=json.Marshal(safe)
  if marshalErr!=nil||strings.Contains(string(encoded),"secret.example")||strings.Contains(err.Error(),"secret.example")||strings.Contains(SafeError(err),"secret.example"){t.Fatal("private read/close cause entered ordinary output")}
 })}}
}
func TestOperatorContextPublishesOnlyAfterSuccessfulBodyClose(t *testing.T){
 binding:=OperatorContext{InstanceBinding:"slrc_"+strings.Repeat("1",64),WorkspaceBinding:"slrc_"+strings.Repeat("2",64),ActorBinding:"slrc_"+strings.Repeat("3",64),ClientBinding:"slrc_"+strings.Repeat("4",64)}
 raw,marshalErr:=json.Marshal(struct{ContractVersion string `json:"contractVersion"`;Context OperatorContext `json:"context"`}{"service-lasso-durable-reconciliation-context.v1",binding});if marshalErr!=nil||!binding.Valid(){t.Fatal("invalid closed context fixture")}
 cause:=errors.New("private close https://secret.example/close")
 for _,closeFailure:=range []bool{true,false}{
  body:=&retainedCloseBody{reader:strings.NewReader(string(raw))};if closeFailure{body.cause=cause}
  client,err:=NewClient("http://127.0.0.1:1",&http.Client{Transport:retainedErrorTransport(func(*http.Request)(*http.Response,error){return &http.Response{StatusCode:200,Header:http.Header{},Body:body},nil})},"source-token");if err!=nil{t.Fatal(err)}
  current,err:=client.OperatorContext(context.Background())
  if body.closed!=1{t.Fatal("body close must be observed exactly once")}
  if closeFailure{if !errors.Is(err,cause)||current!=(OperatorContext{}){t.Fatal("close failure published valid-looking context")}}else if err!=nil||current!=binding{t.Fatal("successful read and close rejected genuine context")}
 }
}
func TestOperatorSafeWrapperOwnsPresentationAndDirectPublicErrorsRemainUseful(t *testing.T){
 public:=&OperatorPublicError{Code:"permission_denied"};wrapper:=&OperatorError{Stage:"transport",cause:public}
 if !errors.Is(wrapper,public)||SafeError(wrapper)!=wrapper.Error()||SafeError(public)!=OperatorMessage(public.Code){t.Fatal("wrapper or genuine direct public error presentation changed")}
 joined:=errors.Join(&OperatorError{Stage:"context"},wrapper)
 if SafeError(joined)!=(&OperatorError{Stage:"context"}).Error(){t.Fatal("private nested type displaced original safe context stage")}
}
func TestOperatorSubmissionFailureRetainsCauseWithoutSerializingIt(t *testing.T){
 cause:=&url.Error{Op:"POST",URL:"https://secret.example/token?key=raw",Err:errors.New("private cause")}
 failure:=&OperatorSubmissionFailure{Dispatched:true,Cause:cause}
 encoded,err:=json.Marshal(failure)
 if !errors.Is(failure,cause)||err!=nil||strings.Contains(string(encoded),"secret.example")||strings.Contains(failure.Error(),"secret.example")||strings.Contains(SafeError(failure),"secret.example"){t.Fatal("dispatched failure lost private cause or serialized it")}
}
