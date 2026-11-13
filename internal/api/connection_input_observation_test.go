package api

import (
 "encoding/json"
 "errors"
 "io"
 "net/url"
 "strings"
 "testing"
)

// Controlled actual resolver cases, ALLUNRUN. No durable/native capture proof.
type connectionObservedFile struct{
 reader *strings.Reader
 readError,closeError error
 reads,closes int
 firstBuffer []byte
}
func(f *connectionObservedFile)Read(p []byte)(int,error){
 f.reads++;if f.firstBuffer==nil{f.firstBuffer=p}
 n,err:=f.reader.Read(p);if err==io.EOF&&f.readError!=nil{return n,f.readError};return n,err
}
func(f *connectionObservedFile)Close()error{f.closes++;return f.closeError}
func connectionFailureInput(err error)*connectionInputObservation{
 switch failure:=err.(type){case *OperatorError:return failure.input;case *ConfigurationError:return failure.input};return nil
}
func assertConnectionInputPrivate(t *testing.T,failure error,observation *connectionInputObservation){
 t.Helper();if failure==nil||observation==nil{t.Fatal("actual configuration failure/input absent")}
 encoded,err:=json.Marshal(failure);if err!=nil{t.Fatal(err)}
 for _,projection:=range []string{failure.Error(),SafeError(failure),string(encoded)}{
  for _,private:=range []string{"private-file-marker","private-config-path","private-token-value","OpenError","ReadError","CloseError","Raw","File","Path"}{if strings.Contains(projection,private){t.Fatal("private input entered ordinary projection")}}
 }
}
func TestConnectionResolverRetainsActualInputThroughDownstreamRefusals(t *testing.T){
 original:=`{"defaultProfile":"private-file-marker","profiles":{"private-file-marker":{"url":"http://127.0.0.1:17883","tokenEnv":"SOURCE_TOKEN"}}}`
 for _,mode:=range []string{"decode","empty-profiles","loaded-profile","selected-profile","client-url","credential"}{t.Run(mode,func(t *testing.T){
  raw:=original;options:=ConnectionOptions{ConfigPath:"private-config-path",Env:func(name string)string{if name=="SOURCE_TOKEN"&&mode!="credential"{return "private-token-value"};return ""}}
  switch mode{
  case "decode":raw=`{"private-file-marker":@}`
  case "empty-profiles":raw=`{"defaultProfile":"private-file-marker","profiles":{}}`
  case "loaded-profile":raw=`{"defaultProfile":"private-file-marker","profiles":{"private-file-marker":{"url":"https://core.example.invalid","tokenEnv":"SOURCE_TOKEN","authMode":"oauth-bearer","scopes":[]}}}`
  case "selected-profile":options.Profile="missing"
  case "client-url":options.APIURL="http://127.0.0.1/%private-file-marker"
  }
  file:=&connectionObservedFile{reader:strings.NewReader(raw)};opens:=0
  manager,client,failure:=resolveConnectionsWithOpener(options,true,func(path string)(io.ReadCloser,error){opens++;if path!=options.ConfigPath{t.Fatal("attempted path changed")};return file,nil})
  observation:=connectionFailureInput(failure);assertConnectionInputPrivate(t,failure,observation)
  if manager!=nil||client!=nil||opens!=1||file.closes!=1||observation.File!=file||observation.Path!=options.ConfigPath||string(observation.Raw)!=raw||observation.Limit!=maxResponseBytes||!observation.OpenAttempted||!observation.ReadAttempted||!observation.CloseAttempted||observation.OpenError!=nil||observation.ReadError!=nil||observation.CloseError!=nil{t.Fatal("actual file/read/close context lost or replayed")}
  if len(raw)>0 && &observation.Raw[0]!=&file.firstBuffer[0]{t.Fatal("original read buffer replaced by a sole copy")}
  switch mode{
  case "decode":var syntax *json.SyntaxError;if !errors.As(failure,&syntax){t.Fatal("actual decoder original lost")}
  case "loaded-profile":configured,ok:=failure.(*ConfigurationError);if !ok||configured.Kind!=ConfigurationErrorOperatorReaderAdmission||configured.cause!=nil{t.Fatal("direct loaded-profile type or semantic cause changed")}
  case "client-url":configured,ok:=failure.(*ConfigurationError);var originalURL *url.Error;if !ok||configured.Kind!=ConfigurationErrorInvalidURL||!errors.As(failure,&originalURL)||configured.cause!=originalURL{t.Fatal("direct client type or URL parser identity lost")}
  default:if _,ok:=failure.(*OperatorError);!ok||errors.Unwrap(failure)!=nil{t.Fatal("semantic configuration refusal invented cause or changed type")}
  }
 })}
}
func TestConnectionResolverRetainsReadCloseAndOpenOriginals(t *testing.T){
 readOriginal:=errors.New("private-file-marker-read");closeOriginal:=errors.New("private-file-marker-close");openOriginal:=errors.New("private-file-marker-open")
 for _,mode:=range []string{"open","read-close","close"}{t.Run(mode,func(t *testing.T){
  file:=&connectionObservedFile{reader:strings.NewReader("private-file-marker-partial"),closeError:closeOriginal};if mode=="read-close"{file.readError=readOriginal}
  options:=ConnectionOptions{ConfigPath:"private-config-path",Env:func(string)string{return ""}}
  opens:=0;_,_,failure:=resolveConnectionsWithOpener(options,true,func(string)(io.ReadCloser,error){opens++;if mode=="open"{return nil,openOriginal};return file,nil})
  observation:=connectionFailureInput(failure);assertConnectionInputPrivate(t,failure,observation)
  if opens!=1||observation.Path!=options.ConfigPath||!observation.OpenAttempted{t.Fatal("attempted open lost or replayed")}
  if mode=="open"{if observation.File!=nil||observation.Raw!=nil||observation.ReadAttempted||observation.CloseAttempted||file.reads!=0||file.closes!=0||observation.OpenError!=openOriginal||!errors.Is(failure,openOriginal){t.Fatal("open failure fabricated file work or lost original")};return}
  if observation.File!=file||string(observation.Raw)!="private-file-marker-partial"||observation.CloseError!=closeOriginal||file.closes!=1||!errors.Is(failure,closeOriginal)||!observation.ReadAttempted||!observation.CloseAttempted{t.Fatal("read/close original or partial bytes lost")}
  if mode=="read-close"&&(observation.ReadError!=readOriginal||!errors.Is(failure,readOriginal)){t.Fatal("actual read error lost")}
 })}
}
func TestConnectionResolverUsefulSuccessAndExistingPartialLimit(t *testing.T){
 raw:=`{"defaultProfile":"source","profiles":{"source":{"url":"http://127.0.0.1:17883","tokenEnv":"SOURCE_TOKEN"}}}`
 file:=&connectionObservedFile{reader:strings.NewReader(raw)};lookups:=0
 options:=ConnectionOptions{ConfigPath:"private-config-path",Env:func(name string)string{if name=="SOURCE_TOKEN"{lookups++;return "private-token-value"};return ""}}
 manager,client,err:=resolveConnectionsWithOpener(options,true,func(string)(io.ReadCloser,error){return file,nil})
 if err!=nil||manager==nil||client==nil||manager.Current()!="source"||!manager.operatorV2||client.operatorToken!="private-token-value"||lookups!=1||file.closes!=1{t.Fatal("useful resolution changed")}
 // No HTTP or Switch is executed; successful manager provenance remains a limit.
 oversized:=strings.Repeat("x",maxResponseBytes+20);file=&connectionObservedFile{reader:strings.NewReader(oversized)}
 _,_,err=resolveConnectionsWithOpener(options,true,func(string)(io.ReadCloser,error){return file,nil})
 observation:=connectionFailureInput(err)
 if err==nil||observation==nil||len(observation.Raw)!=maxResponseBytes+1||file.reader.Len()!=19||file.closes!=1{t.Fatal("existing bounded partial read was widened or hidden")}
}
