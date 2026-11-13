package app

import (
 "context"
 "encoding/json"
 "errors"
 "net/http"
 "net/http/httptest"
 "testing"

 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// Actual HTTP preflight source cases, UNRUN; no native issuer qualification.
func TestOperatorDirectPreflightKeepsAuthenticatedAvailabilityReason(t *testing.T){
 for _,reason:=range []string{"unsupported_capability","permission_denied","source_unavailable","storage_unavailable","dependency_blocked","missing"}{t.Run(reason,func(t *testing.T){
  binding:=selectedOperatorTestContext("a");_,discovery,_:=terminalRefreshFixture(t)
  discovery.MutationSemantics="all_writer_atomic_revision"
  capability:=discovery.Capabilities[0];capability.ID="inbox_mark_read"
  code:=reason;if reason=="missing"{code="unsupported_capability";discovery.Capabilities=[]api.OperatorCapability{}}else{capability.Availability=api.OperatorAvailability{Supported:reason!="unsupported_capability",ReasonCode:&code};discovery.Capabilities=[]api.OperatorCapability{capability}}
  if !discovery.Valid(){t.Fatal("invalid source discovery fixture")}
  contexts,discoveries,unexpected:=0,0,0
  server:=httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter,r *http.Request){
   if r.Method!="GET"||r.Header.Get("x-service-lasso-admin-token")!="source-token"{unexpected++;w.WriteHeader(400);return}
   switch r.URL.Path{
   case "/api/operator/v2/reconciliation-context":contexts++;json.NewEncoder(w).Encode(struct{ContractVersion string `json:"contractVersion"`;Context api.OperatorContext `json:"context"`}{"service-lasso-durable-reconciliation-context.v1",binding})
   case "/api/operator/v2/discovery":discoveries++;json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorDiscovery]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:discovery})
   default:unexpected++;w.WriteHeader(400)
   }
  }));defer server.Close()
  client,err:=api.NewClient(server.URL,server.Client(),"source-token");if err!=nil{t.Fatal(err)}
  state:=operatorModel{client:client,ctx:context.Background(),epoch:1,journey:"inbox",binding:binding}
  state.acceptPageSelection(state.ticket())
  original:=&api.OperatorInboxRow{RowID:"original-row",ServiceID:"root"}
  state.draft=operatorDraft{ReadConnection:1,ReadJourney:"inbox",ReadFilter:"all",Service:"root",Action:"inbox_mark_read",Expected:"original-revision",Context:binding,Review:true,InboxRow:original}
  command:=state.submitDirect();if command==nil{t.Fatal("preflight not reached")};reply:=command().(operatorDirectResult)
  var public *api.OperatorPublicError
  if !errors.As(reply.Err,&public)||public.Code!=code||reply.Dispatched||contexts!=1||discoveries!=1||unexpected!=0{t.Fatal("availability reason changed or denied action reached row/mutation route")}
  updated,follow:=state.Update(reply);state=updated.(operatorModel)
  if follow!=nil||state.draft.Sent||state.draft.InboxRow!=original||state.draft.Expected!="original-revision"||state.draft.Context!=binding||state.errorText!=api.OperatorMessage(code){t.Fatal("denied preflight lost original draft or safe refusal")}
 })}
}
