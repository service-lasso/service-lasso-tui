package app

import(
 "context"
 "encoding/json"
 "net/http"
 "net/http/httptest"
 "testing"

 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// Client/reducer cases do not prove atomic Core authorization between the
// final context check and POST, an installed issuer, or native ownership.
func TestOperatorCancellationRetainsOriginalContextAndOwnOperation(t *testing.T){
 original:=api.OperatorOperation{OperationID:"exact",ServiceID:"root",ActionID:"update_check",Targets:[]string{"root"},Status:"running",Phase:"checking_update",CorrelationID:"original-correlation",CancellationSupported:true,CreatedAt:"2026-10-08T00:00:00.000Z",UpdatedAt:"2026-10-08T00:00:00.000Z",ExpiresAt:"2099-10-08T00:00:00.000Z"}
 expected:=selectedOperatorTestContext("a");foreign:=selectedOperatorTestContext("b")
 if !original.Valid(){t.Fatal("cancel fixture violates actual operation codec")}
 cases:=[]struct{name string;rotateAt int;correlation string;supported bool;posts int;responseInvalid bool}{
  {"current supported cancellation",0,original.CorrelationID,true,1,false},
  {"context changed before own read",1,original.CorrelationID,true,0,false},
  {"context changed after own read",2,original.CorrelationID,true,0,false},
  {"context changed immediately before POST",3,original.CorrelationID,true,0,false},
  {"same ID changed correlation",0,"foreign-correlation",true,0,false},
  {"current cancellation unavailable",0,original.CorrelationID,false,0,false},
  {"cancel response invalid after POST",0,original.CorrelationID,true,1,true},
 }
 for _,test:=range cases{t.Run(test.name,func(t *testing.T){
  contexts,posts:=0,0
  server:=httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter,r *http.Request){
   if r.URL.RawQuery!=""||r.Header.Get("x-service-lasso-admin-token")!="source-only-token"{t.Error("unexpected query or auth edge");w.WriteHeader(400);return}
   switch r.URL.Path{
   case "/api/operator/v2/reconciliation-context":
    if r.Method!="GET"{t.Error("context mutation")};contexts++;binding:=expected;if test.rotateAt>0&&contexts>=test.rotateAt{binding=foreign}
    json.NewEncoder(w).Encode(struct{ContractVersion string `json:"contractVersion"`;Context api.OperatorContext `json:"context"`}{"service-lasso-durable-reconciliation-context.v1",binding})
   case "/api/operator/v2/operations/exact":
    if r.Method!="GET"{t.Error("own read mutation")};operation:=original;operation.CorrelationID=test.correlation;operation.CancellationSupported=test.supported
    json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorReadback]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:api.OperatorReadback{Operation:operation}})
   case "/api/operator/v2/operations/exact/cancel":
    if r.Method!="POST"{t.Error("cancel method")};posts++;var body map[string]any;if json.NewDecoder(r.Body).Decode(&body)!=nil||len(body)!=0{t.Error("cancel body introduced authority")}
    if test.responseInvalid{json.NewEncoder(w).Encode(map[string]any{"invalid":true});return}
    json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorCancel]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:api.OperatorCancel{OperationID:original.OperationID,ActionID:original.ActionID,Status:"requested"}})
   default:t.Error("cancel inferred replacement ID or route");w.WriteHeader(404)
   }
  }));defer server.Close()
  client,err:=api.NewClient(server.URL,server.Client(),"source-only-token");if err!=nil{t.Fatal(err)}
  raw,err:=savedV3Bytes(api.OperatorSavedV3{Version:3,OperationID:original.OperationID,ServiceID:original.ServiceID,ActionID:original.ActionID,Context:expected});if err!=nil{t.Fatal(err)}
  model:=operatorModel{ctx:context.Background(),client:client,epoch:1,binding:expected,saved:raw,operation:&api.OperatorReadback{Operation:original}}
  command:=model.cancelOperation();if command==nil{t.Fatal("supported original cancel has no command")};if model.cancelOperation()!=nil{t.Fatal("concurrent keyboard cancellation created a second command")};message:=command().(operatorCancelled)
  if posts!=test.posts||(message.Err==nil)!=(test.posts==1&&!test.responseInvalid){t.Fatal("cancel context/correlation admission changed")}
  updated,_:=model.Update(message);actual:=updated.(operatorModel)
  if string(actual.saved)!=string(raw)||!actual.operation.Operation.SameRoot(original){t.Fatal("cancel replaced original retained operation")}
  if test.posts==1&&actual.warning==""{t.Fatal("genuine request omitted authoritative readback explanation")}
  if actual.cancelInFlight||actual.cancelAttempted!=(test.posts==1){t.Fatal("cancellation request ownership did not distinguish prePOST denial from possible dispatch")}
  if test.posts==1&&actual.cancelOperation()!=nil{t.Fatal("possibly dispatched cancellation was replayed")}
 })}
}