package app

import (
 "context"
 "encoding/json"
 "net/http"
 "net/http/httptest"
 "testing"

 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// Actual HTTP client/load/reducer source regression, UNRUN. Directly invoking
// a second read is a source race vector, not a compiled keyboard/native claim.
func TestOperatorConfigurationPreviewNeverSubstitutesReplacementFormBinding(t *testing.T){
 for _,replacement:=range []string{"unchanged","definition","model_epoch"}{t.Run(replacement,func(t *testing.T){
  binding:=selectedOperatorTestContext("a");ready:=api.OperatorAvailability{Supported:true,Available:true};reads,posts:=0,0
  original:=api.OperatorConfigForm{ServiceID:"root",DefinitionRevision:"definition",ModelEpoch:"epoch",Availability:ready,Fields:[]api.OperatorField{{Selector:"enabled",Type:"boolean",Value:json.RawMessage(`false`),Constraints:json.RawMessage(`{}`),Availability:ready}},Catalogs:[]api.OperatorCatalog{}}
  server:=httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter,r *http.Request){switch r.URL.Path{
  case "/api/operator/v2/services/root/config-form":
   reads++;form:=original;if reads==2{if replacement=="definition"{form.DefinitionRevision="replacement-definition"};if replacement=="model_epoch"{form.ModelEpoch="replacement-epoch"}}
   json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorConfigForm]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:form})
  case "/api/operator/v2/reconciliation-context":json.NewEncoder(w).Encode(struct{ContractVersion string `json:"contractVersion"`;Context api.OperatorContext `json:"context"`}{"service-lasso-durable-reconciliation-context.v1",binding})
  case "/api/operator/v2/services/root/config-preview":
   var body struct{ExpectedRevision string `json:"expectedRevision"`;Operations []api.OperatorConfigOperation `json:"operations"`};if r.Method!="POST"||json.NewDecoder(r.Body).Decode(&body)!=nil||body.ExpectedRevision!="definition"||len(body.Operations)!=1||body.Operations[0].Selector!="enabled"||string(body.Operations[0].Value)!="true"{t.Error("original config binding/input changed");w.WriteHeader(400);return};posts++
   preview:=api.OperatorPreview{PlanID:"plan",ServiceID:"root",ActionID:"config_patch",DefinitionRevision:"definition",ExpectedRevision:"definition",Generation:"generation",ModelEpoch:"epoch",Availability:ready,Targets:[]string{},Effects:[]string{},ExpiresAt:"2099-10-08T00:00:00.000Z"};json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorPreview]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:preview})
  default:t.Error("unexpected root or mutation");w.WriteHeader(400)
  }}));defer server.Close();client,err:=api.NewClient(server.URL,server.Client(),"source-token");if err!=nil{t.Fatal(err)};_,discovery,_:=terminalRefreshFixture(t)
  model:=operatorModel{ctx:context.Background(),client:client,epoch:1,service:"root",journey:"config",binding:binding,discovery:&discovery}
  command:=model.load(nil);if command==nil{t.Fatal("missing actual config read")};reply:=command().(operatorLoaded);if reply.Err!=nil{t.Fatal(reply.Err)};updated,_:=model.Update(reply);model=updated.(operatorModel);model.beginEdit();model.draft.Text="true"
  if !model.draft.Dirty||model.draft.ConfigModelEpoch!="epoch"||model.draft.Expected!="definition"{t.Fatal("original form binding not captured")}
  command=model.load(nil);reply=command().(operatorLoaded);if reply.Err!=nil{t.Fatal(reply.Err)};updated,_=model.Update(reply);model=updated.(operatorModel)
  command=model.previewDraft()
  if replacement=="unchanged"{if command==nil{t.Fatal("useful unchanged preview blocked")};previewed:=command().(operatorPreviewed);if previewed.Err!=nil||posts!=1{t.Fatal("original useful HTTP preview failed")}}else{if command!=nil||posts!=0||len(model.draft.ConfigOperations)!=0||string(model.draft.Values["enabled"])!="false"{t.Fatal("replacement binding transformed input or dispatched")}}
  if reads!=2||model.draft.Text!="true"||model.draft.Definition!="definition"||model.draft.Expected!="definition"||model.draft.ConfigModelEpoch!="epoch"||model.draft.Service!="root"||model.draft.Sent{t.Fatal("replacement read rewrote original draft")}
 })}
}
