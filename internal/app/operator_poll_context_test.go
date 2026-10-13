package app

import(
 "context"
 "encoding/json"
 "net/http"
 "net/http/httptest"
 "testing"

 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// Real HTTP client and reducer fixtures remain SOURCE-UNRUN; this server is
// not an installed Core, trusted issuer or native ownership witness.
func TestOperatorPollingBracketsOriginalContextAndCorrelation(t *testing.T){
 original:=api.OperatorOperation{OperationID:"exact",ServiceID:"root",ActionID:"service_start",Targets:[]string{"root","dependency"},Status:"running",Phase:"executing",CorrelationID:"original-correlation",CreatedAt:"2026-10-08T00:00:00.000Z",UpdatedAt:"2026-10-08T00:00:00.000Z",ExpiresAt:"2099-10-08T00:00:00.000Z"}
 expected:=selectedOperatorTestContext("a");foreign:=selectedOperatorTestContext("b")
 if !original.Valid()||!expected.Valid()||!foreign.Valid(){t.Fatal("poll fixture violates actual closed codecs")}
 cases:=[]struct{name string;before,after api.OperatorContext;correlation string;wantReads int;positive bool}{
  {"unchanged context and original correlation",expected,expected,original.CorrelationID,1,true},
  {"context rotated before own read",foreign,foreign,original.CorrelationID,0,false},
  {"context rotated after own read",expected,foreign,original.CorrelationID,1,false},
  {"same ID foreign correlation",expected,expected,"foreign-correlation",1,false},
 }
 for _,test:=range cases{t.Run(test.name,func(t *testing.T){
  contexts,reads:=0,0
  server:=httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter,r *http.Request){
   if r.Method!="GET"||r.URL.RawQuery!=""||r.Header.Get("x-service-lasso-admin-token")!="source-only-token"{t.Error("unexpected mutation, query or authentication edge");w.WriteHeader(400);return}
   switch r.URL.Path{
   case "/api/operator/v2/reconciliation-context":
    binding:=test.before;if contexts>0{binding=test.after};contexts++
    json.NewEncoder(w).Encode(struct{ContractVersion string `json:"contractVersion"`;Context api.OperatorContext `json:"context"`}{"service-lasso-durable-reconciliation-context.v1",binding})
   case "/api/operator/v2/operations/exact":
    reads++;operation:=original;operation.CorrelationID=test.correlation;operation.Targets=[]string{}
    json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorReadback]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:api.OperatorReadback{Operation:operation}})
   default:t.Error("poll inferred another operation or called another route");w.WriteHeader(404)
   }
  }));defer server.Close()
  client,err:=api.NewClient(server.URL,server.Client(),"source-only-token");if err!=nil{t.Fatal(err)}
  raw,errSaved:=savedV3Bytes(api.OperatorSavedV3{Version:3,OperationID:original.OperationID,ServiceID:original.ServiceID,ActionID:original.ActionID,Context:expected});if errSaved!=nil{t.Fatal(errSaved)}
  model:=operatorModel{ctx:context.Background(),client:client,epoch:1,pollTimer:1,pollScheduled:true,binding:expected,saved:raw,operation:&api.OperatorReadback{Operation:original}}
  started,command:=model.Update(operatorTick{Epoch:1,Sequence:1});model=started.(operatorModel);if command==nil||!model.pollInFlight||model.pollScheduled{t.Fatal("original tick did not own one exact-ID read")}
  message:=command().(operatorPoll)
  if reads!=test.wantReads||(message.Err==nil)!=test.positive{t.Fatal("original context admission or own-ID read count changed")}
  updated,_:=model.Update(message);actual:=updated.(operatorModel)
  if string(actual.saved)!=string(raw){t.Fatal("poll replaced original retained record")}
  if test.positive{if len(actual.operation.Operation.Targets)!=0||actual.operation.Operation.CorrelationID!=original.CorrelationID{t.Fatal("unchanged own read failed to replace authoritative empty targets")}}else if actual.operation.Operation.CorrelationID!=original.CorrelationID||len(actual.operation.Operation.Targets)!=len(original.Targets)||actual.warning==""{t.Fatal("denied context/correlation read replaced original authority")}
 })}
}

func TestOperatorReducerRejectsForeignPollBinding(t *testing.T){
 expected:=selectedOperatorTestContext("a");foreign:=selectedOperatorTestContext("b")
 original:=api.OperatorOperation{OperationID:"exact",ServiceID:"root",ActionID:"service_start",Targets:[]string{"root"},Status:"running",Phase:"executing",CorrelationID:"original-correlation",CreatedAt:"2026-10-08T00:00:00.000Z",UpdatedAt:"2026-10-08T00:00:00.000Z",ExpiresAt:"2099-10-08T00:00:00.000Z"}
 if !original.Valid()||!expected.Valid()||!foreign.Valid(){t.Fatal("foreign-message fixture violates actual codecs")}
 raw,err:=savedV3Bytes(api.OperatorSavedV3{Version:3,OperationID:original.OperationID,ServiceID:original.ServiceID,ActionID:original.ActionID,Context:expected});if err!=nil{t.Fatal(err)}
 readback:=original;readback.Targets=[]string{}
 model:=operatorModel{epoch:1,pollRequest:1,pollInFlight:true,binding:expected,saved:raw,operation:&api.OperatorReadback{Operation:original}}
 updated,_:=model.Update(operatorPoll{Epoch:1,Sequence:1,ID:original.OperationID,Readback:api.OperatorReadback{Operation:readback},Binding:foreign});actual:=updated.(operatorModel)
 if string(actual.saved)!=string(raw)||len(actual.operation.Operation.Targets)!=1||actual.warning==""{t.Fatal("foreign context message replaced original saved/readback authority")}
}
func TestOperatorPollingOwnsOneTimerAndReadAndRejectsOldResults(t *testing.T){
 expected:=selectedOperatorTestContext("a")
 original:=api.OperatorOperation{OperationID:"exact",ServiceID:"root",ActionID:"service_start",Targets:[]string{"root"},Status:"running",Phase:"executing",CorrelationID:"correlation",CreatedAt:"2026-10-08T00:00:00.000Z",UpdatedAt:"2026-10-08T00:00:02.000Z",ExpiresAt:"2099-10-08T00:00:00.000Z"}
 if !original.Valid(){t.Fatal("poll scheduling fixture violates actual codec")}
 model:=operatorModel{epoch:1,binding:expected,operation:&api.OperatorReadback{Operation:original}}
 if model.nextPollTick()==nil||!model.pollScheduled||model.nextPollTick()!=nil{t.Fatal("one owner allocated duplicate timers")}
 tick:=operatorTick{Epoch:model.epoch,Sequence:model.pollTimer}
 started,command:=model.Update(tick);model=started.(operatorModel)
 if command==nil||!model.pollInFlight||model.pollScheduled||model.nextPollTick()!=nil{t.Fatal("timer and exact-ID read overlap")}
 duplicated,extra:=model.Update(tick);model=duplicated.(operatorModel);if extra!=nil||!model.pollInFlight{t.Fatal("duplicate original tick started another read")}
 old:=original;old.UpdatedAt="2026-10-08T00:00:01.000Z";old.Targets=[]string{}
 ignored,extra:=model.Update(operatorPoll{Epoch:1,Sequence:model.pollRequest-1,ID:original.OperationID,Readback:api.OperatorReadback{Operation:old},Binding:expected});model=ignored.(operatorModel)
 if extra!=nil||!model.pollInFlight||model.operation.Operation.UpdatedAt!=original.UpdatedAt{t.Fatal("stale read generation retired active read or replaced authority")}
 denied,extra:=model.Update(operatorPoll{Epoch:1,Sequence:model.pollRequest,ID:original.OperationID,Readback:api.OperatorReadback{Operation:old},Binding:expected});model=denied.(operatorModel)
 if extra==nil||model.pollInFlight||!model.pollScheduled||model.operation.Operation.UpdatedAt!=original.UpdatedAt||len(model.operation.Operation.Targets)!=1||model.warning==""{t.Fatal("older server readback regressed original state or lost next timer")}
 // A connection replacement explicitly retires local scheduling authority;
 // old timer bytes cannot enter the new connection's reads.
 model.epoch++;model.pollScheduled=false;model.pollInFlight=false
 previousTimer:=model.pollTimer;if model.nextPollTick()==nil{t.Fatal("new epoch has no timer")}
 ignored,extra=model.Update(operatorTick{Epoch:1,Sequence:previousTimer});model=ignored.(operatorModel)
 if extra!=nil||!model.pollScheduled||model.pollInFlight{t.Fatal("old connection timer entered new context")}
}
func TestOperatorPollingPreservesKnownTerminalAndAllowsUnknownReconciliation(t *testing.T){
 binding:=selectedOperatorTestContext("a");succeeded:="succeeded";unknown:="unknown_after_crash"
 terminal:=api.OperatorOperation{OperationID:"exact",ServiceID:"root",ActionID:"service_start",Targets:[]string{"root"},Status:"succeeded",Phase:"completed",Outcome:&succeeded,CorrelationID:"correlation",CreatedAt:"2026-10-08T00:00:00.000Z",UpdatedAt:"2026-10-08T00:00:01.000Z",ExpiresAt:"2099-10-08T00:00:00.000Z"}
 result:=api.OperatorResult{OperationID:terminal.OperationID,ServiceID:terminal.ServiceID,ActionID:terminal.ActionID,Targets:[]string{"root"},Effects:[]string{},Summary:"Completed.",ResultingState:[]api.OperatorResultState{{ServiceID:"root",Installed:true,Configured:true,Running:true}}}
 running:=terminal;running.Status="running";running.Phase="executing";running.Outcome=nil;running.Targets=[]string{}
 if !terminal.Valid()||!running.Valid(){t.Fatal("terminal regression fixture violates actual operation codec")}
 for _,time:=range []string{terminal.UpdatedAt,"2026-10-08T00:00:02.000Z"}{
  running.UpdatedAt=time
  model:=operatorModel{epoch:1,pollRequest:1,pollInFlight:true,binding:binding,operation:&api.OperatorReadback{Operation:terminal,Result:&result}}
  updated,_:=model.Update(operatorPoll{Epoch:1,Sequence:1,ID:terminal.OperationID,Binding:binding,Readback:api.OperatorReadback{Operation:running}});actual:=updated.(operatorModel)
  if !actual.operation.Operation.CanClear()||actual.operation.Result!=&result||len(actual.operation.Operation.Targets)!=1||actual.warning==""{t.Fatal("equal/newer timestamp nonterminal read erased committed original terminal/result")}
 }
 uncertain:=terminal;uncertain.Status=unknown;uncertain.Phase=unknown;uncertain.Outcome=&unknown
 reconciled:=terminal;reconciled.Phase="guarded_result_reconciled";reconciled.Targets=[]string{}
 if !uncertain.Valid()||uncertain.CanClear()||!reconciled.Valid(){t.Fatal("unknown reconciliation fixture violates actual operation codec")}
 model:=operatorModel{epoch:1,pollRequest:1,pollInFlight:true,binding:binding,operation:&api.OperatorReadback{Operation:uncertain}}
 updated,_:=model.Update(operatorPoll{Epoch:1,Sequence:1,ID:terminal.OperationID,Binding:binding,Readback:api.OperatorReadback{Operation:reconciled,Result:&result}});actual:=updated.(operatorModel)
 if !actual.operation.Operation.CanClear()||actual.operation.Operation.Phase!="guarded_result_reconciled"||len(actual.operation.Operation.Targets)!=0||actual.operation.Result!=&result{t.Fatal("unknown original could not accept genuine same-root guarded reconciliation/emptytargets")}
}