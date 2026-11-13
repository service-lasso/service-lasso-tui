package app

import("context";"encoding/json";"errors";"io";"net/http";"reflect";"strings";"testing";"github.com/service-lasso/service-lasso-tui/internal/api")

// Source only, UNRUN; a retained failed observation is not authoritative state.
func TestOwnReadAndRestoreKeepActualReadbackOnPostContextFailure(t *testing.T){
 operation,_,_:=terminalRefreshFixture(t);binding:=selectedOperatorTestContext("a");cause:=errors.New("private-post-read-context-original")
 for _,mode:=range []string{"own-read","restore"}{contexts,reads:=0,0
  transport:=terminalRefreshTransport(func(request *http.Request)(*http.Response,error){
   if request.Method!="GET"{t.Fatal("readback failure dispatched mutation")}
   var value any
   switch request.URL.Path{case "/api/operator/v2/reconciliation-context":contexts++;if contexts>1{return nil,cause};value=struct{ContractVersion string `json:"contractVersion"`;Context api.OperatorContext `json:"context"`}{"service-lasso-durable-reconciliation-context.v1",binding}
   case "/api/operator/v2/operations/exact":reads++;value=api.OperatorEnvelope[api.OperatorReadback]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:api.OperatorReadback{Operation:operation}}
   default:t.Fatal("readback guessed route")}
   raw,err:=json.Marshal(value);if err!=nil{t.Fatal(err)};return &http.Response{StatusCode:200,Header:http.Header{},Body:io.NopCloser(strings.NewReader(string(raw)))},nil
  })
  client,err:=api.NewClient("http://127.0.0.1:1",&http.Client{Transport:transport},"source-token");if err!=nil{t.Fatal(err)}
  if mode=="own-read"{readback,failure:=readOperatorOwn(context.Background(),client,binding,operation)
   if !errors.Is(failure,cause)||!reflect.DeepEqual(readback.Operation,operation){t.Fatal("actual successful readback or post-context original lost")}
   original:=operation;original.Status="running";original.Phase="executing";original.Outcome=nil;held:=&api.OperatorReadback{Operation:original}
   state:=operatorModel{epoch:1,binding:binding,operation:held,pollInFlight:true,pollRequest:1}
   updated,_:=state.Update(operatorPoll{Epoch:1,Sequence:1,ID:operation.OperationID,Binding:binding,Readback:readback,Err:failure});current:=updated.(operatorModel)
   if current.operation!=held||current.operation.Operation.Outcome!=nil||len(current.receivedFailures)!=1||!errors.Is(current.receivedFailures[0].Err,cause){t.Fatal("failed observation became settled authority or lost original")}
  }else{raw,err:=savedV3Bytes(api.OperatorSavedV3{Version:3,OperationID:operation.OperationID,ServiceID:operation.ServiceID,ActionID:operation.ActionID,Context:binding});if err!=nil{t.Fatal(err)}
   held:=&privateSavedCauseHeld{raw:raw};restored,failure:=restoreOperator(context.Background(),client,"source",fileOperationStore{path:"source-only-held",held:held})
   if !errors.Is(failure,cause)||restored.Readback==nil||!reflect.DeepEqual(restored.Readback.Operation,operation)||restored.Migration!=nil||string(restored.Raw)!=string(raw)||held.publishes!=0||held.removes!=0{t.Fatal("restoration lost actual failed observation/record or published migration")}
   if failure.Error()!=(&api.OperatorError{Stage:"context"}).Error()||api.SafeError(failure)!=failure.Error(){t.Fatal("post-context original changed fixed reconciliation presentation")}
  }
  if contexts!=2||reads!=1{t.Fatal("exact before/read/after path not reached")}
 }
}
