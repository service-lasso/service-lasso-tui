package app

import (
 "context"
 "encoding/json"
 "errors"
 "net/http"
 "net/http/httptest"
 "strings"
 "testing"

 tea "github.com/charmbracelet/bubbletea"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// Actual HTTP decoder/command/reducer sources, UNRUN. This fixture supplies no
// installed Core, trusted issuer, backend atomic admission or native evidence.
func terminalRefreshFixture(t *testing.T)(api.OperatorOperation,api.OperatorDiscovery,api.OperatorDetail){
 t.Helper();outcome:="succeeded"
 operation:=api.OperatorOperation{OperationID:"exact",ServiceID:"root",ActionID:"service_start",Targets:[]string{"root"},Status:"succeeded",Phase:"completed",Outcome:&outcome,CorrelationID:"correlation",CreatedAt:"2026-10-08T00:00:00.000Z",UpdatedAt:"2026-10-08T00:00:01.000Z",ExpiresAt:"2099-10-08T00:00:00.000Z"}
 discovery:=api.OperatorDiscovery{MetadataRevision:"metadata",ModelEpoch:"epoch",SupportedJourneys:[]string{},MutationSemantics:"unavailable",Limits:api.OperatorLimits{RequestBytes:65536,ResponseBytes:262144,ErrorBytes:8192,Depth:16,TotalValues:8192,PropertyNameBytes:128,PublicStringBytes:4096,IDBytes:128,SummaryBytes:512,DisplayNameBytes:256,DescriptionBytes:1024,PageRows:100,ContextBytes:4096},Capabilities:[]api.OperatorCapability{{ID:"read_detail",Availability:api.OperatorAvailability{Supported:true,Available:true},RequiredProfile:"observer",RequiredScopes:[]string{"service-lasso:read"},ConfirmationPolicy:"none",BindingRequirements:[]string{}}}}
 detail:=api.OperatorDetail{Summary:api.OperatorService{ServiceID:"root",DisplayName:"Original",Description:"",DefinitionRevision:"definition",Generation:"generation",ModelEpoch:"epoch",Installed:"yes",Configured:"yes",Running:"yes",HealthStatus:"unknown",UpdateStatus:"not_checked",CapabilityIDs:[]string{}},Provider:api.OperatorProvider{Role:"service",Availability:api.OperatorAvailability{Supported:true,Available:true}},Dependencies:[]api.OperatorEdge{},Endpoints:[]api.OperatorEndpoint{},VariableMetadata:[]api.OperatorVariable{},SetupMetadata:[]api.OperatorSetupStep{},ActionMetadata:[]api.OperatorAction{}}
 if !operation.Valid()||!discovery.Valid()||!detail.Valid(){t.Fatal("terminal fixture violates closed public codecs")};return operation,discovery,detail
}
func terminalRefreshCommandMessage(t *testing.T,command tea.Cmd)operatorTerminalRefreshed{
 t.Helper();if command==nil{t.Fatal("missing original service read")};message:=command()
 if own,ok:=message.(operatorTerminalRefreshed);ok{return own}
 if batch,ok:=message.(tea.BatchMsg);ok{for _,child:=range batch{message:=child();if own,ok:=message.(operatorTerminalRefreshed);ok{return own}}}
 t.Fatal("command did not contain original service read");return operatorTerminalRefreshed{}
}
func TestOperatorTerminalRefreshOriginalRootAndOnceAttempt(t *testing.T){
 operation,discovery,detail:=terminalRefreshFixture(t);binding:=selectedOperatorTestContext("a");foreign:=selectedOperatorTestContext("b")
 for _,test:=range []struct{name string;before,after api.OperatorContext;reads int;success bool}{{"current",binding,binding,1,true},{"before changed",foreign,foreign,0,false},{"after changed",binding,foreign,1,false}}{t.Run(test.name,func(t *testing.T){
  contexts,reads:=0,0;retryCurrent:=false
  server:=httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter,r *http.Request){
   if r.Method!="GET"||r.URL.RawQuery!=""||r.Header.Get("x-service-lasso-admin-token")!="source-token"{t.Error("service refresh replayed a mutation or changed authorization");w.WriteHeader(400);return}
   switch r.URL.Path{case "/api/operator/v2/reconciliation-context":current:=test.before;if contexts>0{current=test.after};if retryCurrent{current=binding};contexts++;json.NewEncoder(w).Encode(struct{ContractVersion string `json:"contractVersion"`;Context api.OperatorContext `json:"context"`}{"service-lasso-durable-reconciliation-context.v1",current})
   case "/api/operator/v2/services/root":reads++;json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorDetail]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:detail})
   default:t.Error("refresh selected a current or target service instead of original root");w.WriteHeader(404)}
  }));defer server.Close();client,err:=api.NewClient(server.URL,server.Client(),"source-token");if err!=nil{t.Fatal(err)}
  raw,err:=savedV3Bytes(api.OperatorSavedV3{Version:3,OperationID:operation.OperationID,ServiceID:operation.ServiceID,ActionID:operation.ActionID,Context:binding});if err!=nil{t.Fatal(err)}
  running:=operation;running.Status="running";running.Phase="executing";running.Outcome=nil
  model:=operatorModel{ctx:context.Background(),client:client,epoch:1,pollRequest:1,pollInFlight:true,service:"other",journey:"detail",binding:binding,discovery:&discovery,saved:raw,custodyWarning:"original storage failure",operation:&api.OperatorReadback{Operation:running}}
  updated,command:=model.Update(operatorPoll{Epoch:1,Sequence:1,ID:operation.OperationID,Binding:binding,Readback:api.OperatorReadback{Operation:operation}});model=updated.(operatorModel)
  message:=terminalRefreshCommandMessage(t,command);if (message.Err==nil)!=test.success||reads!=test.reads{t.Fatal("original context admission changed")}
  updated,_=model.Update(message);model=updated.(operatorModel)
  if string(model.saved)!=string(raw)||model.custodyWarning!="original storage failure"||!model.operation.Operation.SameRoot(operation)||model.detail!=nil||model.service!="other"{t.Fatal("original refresh changed custody, operation or browsing pane")}
  if test.success{if model.terminalDetail==nil||model.terminalDetail.Summary.Running!="yes"||model.terminalRefreshWarning!=""{t.Fatal("authoritative state did not publish separately")}}else if model.terminalDetail!=nil||model.terminalRefreshWarning==""{t.Fatal("denied read erased failure or published data")}
  model.pollInFlight=true;model.pollRequest++;updated,_=model.Update(operatorPoll{Epoch:1,Sequence:model.pollRequest,ID:operation.OperationID,Binding:binding,Readback:api.OperatorReadback{Operation:operation}});model=updated.(operatorModel)
  if model.terminalRefreshInFlight||model.refreshTerminalState(false)!=nil{t.Fatal("repeated terminal poll retried the automatic read")}
  // Only an explicit read retry may retry a failed attempt; useful success
  // remains once and timer polling never retries failure.
  if test.success{if model.refreshTerminalState(true)!=nil{t.Fatal("successful read repeated")}}else{retryCurrent=true;model.journey="inbox";updated,retry:=model.Update(tea.KeyMsg{Type:tea.KeyRunes,Runes:[]rune{'r'}});model=updated.(operatorModel);reply:=terminalRefreshCommandMessage(t,retry);updated,_=model.Update(reply);model=updated.(operatorModel);if model.terminalDetail==nil||model.terminalRefreshWarning!=""||len(model.terminalRefreshFailures)!=1||string(model.saved)!=string(raw)||model.custodyWarning!="original storage failure"{t.Fatal("explicit successful read retry lost original failure/custody or did not distinguish success")}}
 })}
}
func TestOperatorTerminalRefreshEntryPathsAndStorageFailure(t *testing.T){
 operation,discovery,detail:=terminalRefreshFixture(t);binding:=selectedOperatorTestContext("a")
 for _,entry:=range []string{"restored","immediate submission"}{t.Run(entry,func(t *testing.T){
  reads:=0;server:=httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter,r *http.Request){if r.Method!="GET"{t.Error("terminal entry replayed mutation");w.WriteHeader(400);return};switch r.URL.Path{case "/api/operator/v2/reconciliation-context":json.NewEncoder(w).Encode(struct{ContractVersion string `json:"contractVersion"`;Context api.OperatorContext `json:"context"`}{"service-lasso-durable-reconciliation-context.v1",binding});case "/api/operator/v2/services/root":reads++;json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorDetail]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:detail});default:t.Error("terminal entry used another root/route");w.WriteHeader(404)}}));defer server.Close();client,err:=api.NewClient(server.URL,server.Client(),"source-token");if err!=nil{t.Fatal(err)}
  model:=operatorModel{ctx:context.Background(),client:client,epoch:1,journey:"inbox",binding:binding,discovery:&discovery,custodyWarning:"retained original failure"}
  var updated tea.Model;var command tea.Cmd
  if entry=="restored"{raw,err:=savedV3Bytes(api.OperatorSavedV3{Version:3,OperationID:operation.OperationID,ServiceID:operation.ServiceID,ActionID:operation.ActionID,Context:binding});if err!=nil{t.Fatal(err)};record,err:=api.DecodeOperatorSavedV3(raw);if err!=nil{t.Fatal(err)};updated,command=model.Update(operatorBooted{Epoch:1,Discovery:discovery,Context:binding,Restore:operatorRestoration{Raw:raw,Record:record,Readback:&api.OperatorReadback{Operation:operation}}})
  }else{ticket:=model.ticket();model.saved=[]byte("retained old bytes");model.draft=operatorDraft{Sent:true,Context:binding,MutationTicket:&ticket,Preview:&api.OperatorPreview{ServiceID:operation.ServiceID,ActionID:operation.ActionID}};updated,command=model.Update(operatorSubmitted{Epoch:1,Ticket:ticket,Binding:binding,Expected:model.saved,Submission:api.OperatorSubmission{Kind:"completed",Operation:operation,Result:&api.OperatorResult{OperationID:operation.OperationID,ServiceID:operation.ServiceID,ActionID:operation.ActionID,Targets:[]string{"root"},Effects:[]string{},Summary:"Completed.",ResultingState:[]api.OperatorResultState{{ServiceID:"root",Installed:true,Configured:true,Running:true}}}}})}
  actual:=updated.(operatorModel);if !actual.terminalRefreshInFlight||actual.terminalRefreshID!=operation.OperationID||actual.operation==nil||actual.custodyWarning==""{t.Fatal("terminal entry or storage failure bypassed original service refresh")}
  if entry=="immediate submission"&&string(actual.saved)!="retained old bytes"{t.Fatal("failed persistence discarded original bytes")}
  message:=terminalRefreshCommandMessage(t,command);updated,_=actual.Update(message);actual=updated.(operatorModel);if reads!=1||actual.terminalDetail==nil||actual.custodyWarning==""{t.Fatal("terminal entry did not complete actual original service read while retaining storage failure")}
 })}
}
func TestOperatorTerminalRefreshStaleRepliesAndCurrentDetail(t *testing.T){
 operation,discovery,detail:=terminalRefreshFixture(t);binding:=selectedOperatorTestContext("a")
 base:=operatorModel{epoch:1,service:operation.ServiceID,journey:"detail",binding:binding,discovery:&discovery,terminalRefreshSequence:1,terminalRefreshInFlight:true,operation:&api.OperatorReadback{Operation:operation},saved:[]byte("original"),custodyWarning:"original custody"}
 message:=operatorTerminalRefreshed{Epoch:1,Sequence:1,Binding:binding,Original:operation,Ticket:base.ticket(),Detail:detail}
 current,_:=base.Update(message);if current.(operatorModel).detail==nil{t.Fatal("current original Detail selection did not admit fresh state")}
 for _,change:=range []string{"browse","profile","operation","clear"}{t.Run(change,func(t *testing.T){model:=base;switch change{case "browse":model.service="other";model.selection++;case "profile":model.epoch++;case "operation":copy:=operation;copy.OperationID="new";model.operation=&api.OperatorReadback{Operation:copy};case "clear":model.operation=nil;model.terminalRefreshSequence++;model.terminalRefreshInFlight=false};updated,_:=model.Update(message);actual:=updated.(operatorModel);if actual.detail!=nil||string(actual.saved)!="original"||actual.custodyWarning!="original custody"{t.Fatal("stale service read published into another pane or discarded custody")};if change!="browse"&&actual.terminalDetail!=nil{t.Fatal("retired operation/context read published original detail")}})}
}
type terminalRefreshTransport func(*http.Request)(*http.Response,error)
func(f terminalRefreshTransport)RoundTrip(r *http.Request)(*http.Response,error){return f(r)}
func TestOperatorTerminalRefreshRetainsOriginalContextError(t *testing.T){
 operation,discovery,_:=terminalRefreshFixture(t);cause:=errors.New("private original context transport cause")
 client,err:=api.NewClient("http://127.0.0.1:1",&http.Client{Transport:terminalRefreshTransport(func(*http.Request)(*http.Response,error){return nil,cause})},"source-token");if err!=nil{t.Fatal(err)}
 model:=operatorModel{ctx:context.Background(),client:client,epoch:1,binding:selectedOperatorTestContext("a"),discovery:&discovery,operation:&api.OperatorReadback{Operation:operation},width:120,height:20}
 message:=terminalRefreshCommandMessage(t,model.refreshTerminalState(false));if !errors.Is(message.Err,cause){t.Fatal("context and client wrappers discarded original error cause")}
 updated,_:=model.Update(message);actual:=updated.(operatorModel);if len(actual.terminalRefreshFailures)!=1||!errors.Is(actual.terminalRefreshFailures[0].Err,cause){t.Fatal("model discarded original failure observation")};encoded,marshalErr:=json.Marshal(actual.terminalRefreshFailures[0]);if marshalErr!=nil||strings.Contains(string(encoded),cause.Error()){t.Fatal("private retained error entered JSON")};if actual.terminalRefreshWarning==""||strings.Contains(actual.View(),cause.Error()){t.Fatal("original cause was lost or disclosed in ordinary UI")}
}

// Injected held writer tests actual reducer Clear admission only. It grants no
// native writer exclusion, durability or successful operating-system closure.
type terminalRefreshHeldStore struct{raw []byte;removed bool}
func(s *terminalRefreshHeldStore)read()([]byte,error){return s.raw,nil}
func(s *terminalRefreshHeldStore)publish(raw []byte)error{s.raw=raw;return nil}
func(s *terminalRefreshHeldStore)remove()error{s.raw=nil;s.removed=true;return nil}
func(s *terminalRefreshHeldStore)close()error{return nil}
func(s *terminalRefreshHeldStore)retainOwnership(){}
func TestOperatorDeliberateClearRetiresRefreshPublicationWithoutDiscardingFailure(t *testing.T){
 operation,discovery,detail:=terminalRefreshFixture(t);binding:=selectedOperatorTestContext("a")
 raw,err:=savedV3Bytes(api.OperatorSavedV3{Version:3,OperationID:operation.OperationID,ServiceID:operation.ServiceID,ActionID:operation.ActionID,Context:binding});if err!=nil{t.Fatal(err)}
 held:=&terminalRefreshHeldStore{raw:raw};cause:=errors.New("private earlier refresh failure")
 model:=operatorModel{epoch:1,journey:"detail",service:operation.ServiceID,binding:binding,discovery:&discovery,saved:raw,store:fileOperationStore{path:"source-only-held-record",held:held},operation:&api.OperatorReadback{Operation:operation},terminalRefreshSequence:1,terminalRefreshInFlight:true,terminalRefreshFailures:[]operatorTerminalRefreshFailure{{Epoch:1,Sequence:0,Binding:binding,Original:operation,Err:cause}}}
 message:=operatorTerminalRefreshed{Epoch:1,Sequence:1,Binding:binding,Original:operation,Ticket:model.ticket(),Detail:detail}
 updated,_:=model.Update(tea.KeyMsg{Type:tea.KeyRunes,Runes:[]rune{'x'}});model=updated.(operatorModel)
 if !held.removed||model.operation!=nil||model.saved!=nil||model.terminalRefreshInFlight{t.Fatal("deliberate exact-own clear did not retire publication")}
 updated,_=model.Update(message);actual:=updated.(operatorModel)
 if actual.detail!=nil||actual.terminalDetail!=nil||len(actual.terminalRefreshFailures)!=1||!errors.Is(actual.terminalRefreshFailures[0].Err,cause){t.Fatal("retired natural read published stale detail or discarded prior failure")}
}
func TestOperatorPrivateErrorCauseCannotBecomePublicFieldDiagnostics(t *testing.T){
 private:=&api.OperatorPublicError{Code:"validation_failed",FieldErrors:[]api.OperatorFieldError{{Selector:"private transport value",Code:"validation_failed"}}}
 client,err:=api.NewClient("http://127.0.0.1:1",&http.Client{Transport:terminalRefreshTransport(func(*http.Request)(*http.Response,error){return nil,private})},"source-token");if err!=nil{t.Fatal(err)}
 _,failure:=client.OperatorDetail(context.Background(),"root");if !errors.Is(failure,private){t.Fatal("actual transport did not retain private cause")}
 model:=operatorModel{epoch:1,loading:true,width:120,height:20};updated,_:=model.Update(operatorPreviewed{Ticket:model.ticket(),Err:failure});actual:=updated.(operatorModel)
 if len(actual.fieldErrors)!=0||strings.Contains(actual.View(),private.FieldErrors[0].Selector){t.Fatal("private nested error was granted public field diagnostic authority")}
 public:=&api.OperatorPublicError{Code:"validation_failed",FieldErrors:[]api.OperatorFieldError{{Selector:"/input/payload/text",Code:"validation_failed"}}}
 updated,_=model.Update(operatorPreviewed{Ticket:model.ticket(),Err:public});if len(updated.(operatorModel).fieldErrors)!=1{t.Fatal("genuine direct public diagnostics became unavailable")}
}
func TestOperatorRestorationAfterContextFailurePreservesCauseAndOriginalBytes(t *testing.T){
 operation,_,_:=terminalRefreshFixture(t);binding:=selectedOperatorTestContext("a");cause:=errors.New("private post-read context failure")
 raw,err:=savedV3Bytes(api.OperatorSavedV3{Version:3,OperationID:operation.OperationID,ServiceID:operation.ServiceID,ActionID:operation.ActionID,Context:binding});if err!=nil{t.Fatal(err)}
 contexts,reads:=0,0
 server:=httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter,r *http.Request){if r.Method!="GET"{t.Error("restoration replayed mutation");w.WriteHeader(400);return};switch r.URL.Path{case "/api/operator/v2/reconciliation-context":json.NewEncoder(w).Encode(struct{ContractVersion string `json:"contractVersion"`;Context api.OperatorContext `json:"context"`}{"service-lasso-durable-reconciliation-context.v1",binding});case "/api/operator/v2/operations/exact":reads++;json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorReadback]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:api.OperatorReadback{Operation:operation}});default:t.Error("restoration guessed root or operation");w.WriteHeader(404)}}));defer server.Close()
 originalTransport:=server.Client().Transport
 client,err:=api.NewClient(server.URL,&http.Client{Transport:terminalRefreshTransport(func(r *http.Request)(*http.Response,error){if r.URL.Path=="/api/operator/v2/reconciliation-context"{contexts++;if contexts==2{return nil,cause}};return originalTransport.RoundTrip(r)})},"source-token");if err!=nil{t.Fatal(err)}
 result,failure:=restoreOperator(context.Background(),client,"source-connection",fileOperationStore{path:"source-only-held-record",held:&terminalRefreshHeldStore{raw:raw}})
 if !errors.Is(failure,cause)||string(result.Raw)!=string(raw)||result.Readback==nil||result.Readback.Operation.OperationID!=operation.OperationID||result.Record!=(api.OperatorSavedV3{})||result.Migration!=nil||reads!=1||contexts!=2||strings.Contains(api.SafeError(failure),cause.Error()){t.Fatal("post-read context failure lost actual private observation/cause/record, admitted migration or replayed operation")}
}
