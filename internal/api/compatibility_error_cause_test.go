package api

import (
 "context"
 "encoding/json"
 "errors"
 "io"
 "net/http"
 "os"
 "path/filepath"
 "strings"
 "strconv"
 "testing"
)

// These actual client boundaries preserve error identity privately. They do not
// establish durable original diagnostics, response-close or native qualification.
func TestCompatibilityReadPreservesPrivateOriginalCause(t *testing.T){
 cause:=errors.New("private original https://secret.example/?token=raw")
 cases:=[]struct{name string;transport retainedErrorTransport}{
  {"transport",func(*http.Request)(*http.Response,error){return nil,cause}},
  {"body-read",func(*http.Request)(*http.Response,error){return &http.Response{StatusCode:200,Body:&retainedCloseBody{reader:retainedErrorReader{cause:cause}}},nil}},
 }
 for _,entry:=range cases{t.Run(entry.name,func(t *testing.T){
  client,err:=NewClient("http://127.0.0.1:17883",&http.Client{Transport:entry.transport},"private-token");if err!=nil{t.Fatal(err)}
  _,err=client.Health(context.Background());if !errors.Is(err,cause){t.Fatal("original error identity lost")}
  raw,marshalErr:=json.Marshal(err);if marshalErr!=nil{t.Fatal(marshalErr)}
  for _,value:=range []string{err.Error(),SafeError(err),string(raw)}{if strings.Contains(value,"secret.example")||strings.Contains(value,"token=raw"){t.Fatal("private cause escaped presentation")}}
 })}
}
func TestConnectionOpenPreservesPrivatePathError(t *testing.T){
 path:=filepath.Join(t.TempDir(),"private-token-profile.json")
 _,_,err:=ResolveOperatorConnections(ConnectionOptions{ConfigPath:path,Env:func(string)string{return ""}})
 if !errors.Is(err,os.ErrNotExist){t.Fatal("original filesystem error identity lost")}
 var original *os.PathError;if !errors.As(err,&original)||original.Path!=path{t.Fatal("original path error lost")}
 raw,marshalErr:=json.Marshal(err);if marshalErr!=nil{t.Fatal(marshalErr)}
 for _,value:=range []string{err.Error(),SafeError(err),string(raw)}{if strings.Contains(value,path)||strings.Contains(value,"private-token"){t.Fatal("private filesystem cause escaped presentation")}}
}
func TestCompatibilityReadAndCloseErrorsRemainOriginal(t *testing.T){
 readCause:=errors.New("private read https://secret.example/read")
 closeCause:=errors.New("private close https://secret.example/close")
 for _,readFailure:=range []bool{false,true}{
  body:=&retainedCloseBody{reader:strings.NewReader(`{"status":"ok","api":{"status":"ok","version":"v1"}}`),cause:closeCause};if readFailure{body.reader=retainedErrorReader{cause:readCause}}
  client,err:=NewClient("http://127.0.0.1:17883",&http.Client{Transport:retainedErrorTransport(func(*http.Request)(*http.Response,error){return &http.Response{StatusCode:200,Body:body},nil})},"");if err!=nil{t.Fatal(err)}
  health,err:=client.Health(context.Background())
  if body.closed!=1||!errors.Is(err,closeCause)||readFailure&&!errors.Is(err,readCause)||health!=(Health{}){t.Fatal("failed close published health or lost an original read/close error")}
  raw,marshalErr:=json.Marshal(err);if marshalErr!=nil{t.Fatal(marshalErr)}
  for _,value:=range []string{err.Error(),SafeError(err),string(raw)}{if strings.Contains(value,"secret.example"){t.Fatal("private read/close cause escaped presentation")}}
 }
}
func TestBoundedDecoderPreservesActualLibraryErrorsPrivately(t *testing.T){
 var destination struct{Value int `json:"value"`}
 cases:=[]struct{name,raw string;check func(error)bool}{
  {"syntax",`{"value":!}`,func(err error)bool{var original *json.SyntaxError;return errors.As(err,&original)}},
  {"type",`{"value":"private-secret"}`,func(err error)bool{var original *json.UnmarshalTypeError;return errors.As(err,&original)}},
  {"unexpected EOF",`{"value":"private-secret`,func(err error)bool{return errors.Is(err,io.ErrUnexpectedEOF)}},
  {"actual EOF",`{"value":`,func(err error)bool{return errors.Is(err,io.EOF)}},
 }
 for _,entry:=range cases{t.Run(entry.name,func(t *testing.T){
  err:=decodeResponse([]byte(entry.raw),&destination);if err==nil||!entry.check(err){t.Fatal("original decoder error type or identity lost")}
  raw,marshalErr:=json.Marshal(err);if marshalErr!=nil{t.Fatal(marshalErr)}
  for _,value:=range []string{err.Error(),SafeError(err),string(raw)}{if strings.Contains(value,"private-secret")||strings.Contains(value,"invalid character")||strings.Contains(value,"unexpected EOF"){t.Fatal("private parser error escaped safe presentation")}}
 })}
 for _,raw:=range []string{`{"value":1,"value":2}`,`{} {}`,`{"value":"`+strings.Repeat("x",4097)+`"}`} {
  err:=decodeResponse([]byte(raw),&destination);var safe *OperatorError
  if !errors.As(err,&safe)||safe.Unwrap()!=nil{t.Fatal("semantic refusal fabricated an original parser error")}
 }
}
func TestSelectedDecoderRetainsActualTokenAndNumericErrors(t *testing.T){
 _,err:=operatorJSONCandidate([]byte(`{"value":!}`),OperatorResponseBytes)
 var syntax *json.SyntaxError;if !errors.As(err,&syntax){t.Fatal("selected token error object lost")};_,fieldErr:=operatorFieldJSON("file_entry",[]byte(`{"value":!}`));if !errors.As(fieldErr,&syntax){t.Fatal("file entry discarded selected token cause")}
 _,numericErr:=operatorJSONCandidate([]byte(`1e10000`),OperatorResponseBytes)
 var number *strconv.NumError;if !errors.As(numericErr,&number)||!errors.Is(numericErr,strconv.ErrRange){t.Fatal("selected numeric parser cause lost")}
 for _,failure:=range []error{err,numericErr}{raw,marshalErr:=json.Marshal(failure);if marshalErr!=nil{t.Fatal(marshalErr)};for _,value:=range []string{failure.Error(),SafeError(failure),string(raw)}{if strings.Contains(value,"invalid character")||strings.Contains(value,"1e10000"){t.Fatal("selected private parser cause escaped presentation")}}}
 for _,raw:=range []string{`{"value":1,"value":2}`,`{} {}`,`9007199254740992`}{_,err:=operatorJSONCandidate([]byte(raw),OperatorResponseBytes);var safe *OperatorError;if !errors.As(err,&safe)||safe.Unwrap()!=nil{t.Fatal("selected semantic refusal fabricated a parser error")}}
}
func TestTypedErrorFallbackRetainsOriginalDecoderError(t *testing.T){
 err:=operatorResponseError([]byte(`{"error":!}`),400,"GET","/services",nil)
 var syntax *json.SyntaxError;var safe *OperatorError
 if !errors.As(err,&syntax)||!errors.As(err,&safe)||safe.Stage!="http"||safe.Status!=400{t.Fatal("typed error fallback discarded original cause or HTTP projection")}
 raw,marshalErr:=json.Marshal(err);if marshalErr!=nil{t.Fatal(marshalErr)}
 for _,value:=range []string{err.Error(),SafeError(err),string(raw)}{if strings.Contains(value,"invalid character"){t.Fatal("fallback exposed private decoder cause")}}
 semantic:=operatorResponseError([]byte(`{}`),400,"GET","/services",nil);if !errors.As(semantic,&safe)||safe.Unwrap()!=nil{t.Fatal("typed error semantic refusal fabricated decoder cause")}
}
func TestSelectedSurrogateScannerRetainsOriginalHexError(t *testing.T){
 _,err:=operatorJSONCandidate([]byte(`{"value":"\uzzzz"}`),OperatorResponseBytes)
 var original *strconv.NumError;if !errors.As(err,&original){t.Fatal("actual surrogate hex error lost")}
 raw,marshalErr:=json.Marshal(err);if marshalErr!=nil{t.Fatal(marshalErr)}
 for _,value:=range []string{err.Error(),SafeError(err),string(raw)}{if strings.Contains(value,"zzzz"){t.Fatal("private surrogate error exposed")}}
 _,semantic:=operatorJSONCandidate([]byte(`{"value":"\udc00"}`),OperatorResponseBytes);var safe *OperatorError;if !errors.As(semantic,&safe)||safe.Unwrap()!=nil{t.Fatal("isolated surrogate fabricated original parser error")}
}
func TestSelectedHTTPReadsRetainOriginalDecoderError(t *testing.T){
 for _,route:=range []string{"context","detail"}{t.Run(route,func(t *testing.T){
  body:=&retainedCloseBody{reader:strings.NewReader(`{"private-value":!}`)}
  client,err:=NewOperatorClientWithAuth("http://127.0.0.1:17883",&http.Client{Transport:retainedErrorTransport(func(*http.Request)(*http.Response,error){return &http.Response{StatusCode:200,Body:body},nil})},"",AuthModeLocalAdmin);if err!=nil{t.Fatal(err)}
  if route=="context"{current,failure:=client.OperatorContext(context.Background());err=failure;if current!=(OperatorContext{}){t.Fatal("failed parser published context")}}else{current,failure:=client.OperatorDetail(context.Background(),"test");err=failure;if current.Summary.ServiceID!=""{t.Fatal("failed parser published detail")}}
  var syntax *json.SyntaxError;if !errors.As(err,&syntax)||body.closed!=1{t.Fatal("actual HTTP parser cause lost or body not closed once")}
  raw,marshalErr:=json.Marshal(err);if marshalErr!=nil{t.Fatal(marshalErr)};for _,value:=range []string{err.Error(),SafeError(err),string(raw)}{if strings.Contains(value,"invalid character")||strings.Contains(value,"private-value"){t.Fatal("HTTP parser cause exposed")}}
 })}
}
type originalMarshalFailure struct{cause error}
func(value originalMarshalFailure)MarshalJSON()([]byte,error){return nil,value.cause}
func TestRequestEncodingRetainsOriginalErrorWithoutDispatch(t *testing.T){
 cause:=errors.New("private request encoding https://secret.example/token")
 dispatched:=0;client,err:=NewOperatorClientWithAuth("http://127.0.0.1:17883",&http.Client{Transport:retainedErrorTransport(func(*http.Request)(*http.Response,error){dispatched++;return nil,errors.New("unexpected dispatch")})},"",AuthModeLocalAdmin);if err!=nil{t.Fatal(err)}
 _,err=operatorRequest(context.Background(),client,"PATCH","/services/test/meta",originalMarshalFailure{cause},func(value OperatorMeta)bool{return value.Valid()},200)
 var original *json.MarshalerError;if !errors.Is(err,cause)||!errors.As(err,&original)||dispatched!=0||SafeError(err)!=OperatorMessage("invalid_request"){t.Fatal("request encoding cause lost, presentation changed or request dispatched")}
 raw,marshalErr:=json.Marshal(err);if marshalErr!=nil{t.Fatal(marshalErr)};for _,value:=range []string{err.Error(),SafeError(err),string(raw)}{if strings.Contains(value,"secret.example"){t.Fatal("private encoding error exposed")}}
}
func TestSeparateExponentParseRetainsActualRangeError(t *testing.T){
 exponent:=strings.Repeat("9",400)
 admitted,err:=operatorSafeMagnitude("0e"+exponent)
 var original *strconv.NumError;if admitted||!errors.As(err,&original)||!errors.Is(err,strconv.ErrRange)||original.Num!=exponent{t.Fatal("separate actual exponent error lost or admitted")}
 _,failure:=operatorJSONCandidate([]byte("0e"+exponent),OperatorResponseBytes)
 if !errors.As(failure,&original)||!errors.Is(failure,strconv.ErrRange){t.Fatal("numeric candidate discarded actual range error")}
 raw,marshalErr:=json.Marshal(failure);if marshalErr!=nil{t.Fatal(marshalErr)};for _,value:=range []string{failure.Error(),SafeError(failure),string(raw)}{if strings.Contains(value,exponent){t.Fatal("private exponent cause exposed")}}
 admitted,semantic:=operatorSafeMagnitude("9007199254740992");if admitted||semantic!=nil{t.Fatal("magnitude refusal fabricated original parser error")}
}