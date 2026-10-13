package app

import (
 "context"
 "encoding/json"
 "net/http"
 "net/http/httptest"
 "strings"
 "testing"

 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// Actual HTTP/reducer source case, UNRUN. No durable diagnostic/native proof.
func TestOperatorFailedCurrentMetaRefreshKeepsOriginalButDisplaysUnknown(t *testing.T){
 favorite:=true;revision:="revision";meta:=api.OperatorMeta{ServiceID:"root",SourceRevision:&revision,Favorite:&favorite,Availability:api.OperatorAvailability{Supported:true,Available:true}};reads:=0
 server:=httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter,r *http.Request){
  if metadataBindingFixture(t,w,r){return}
  if r.Method!="GET"||r.URL.Path!="/api/operator/v2/services/root/meta"{t.Error("unexpected mutation or root");w.WriteHeader(400);return};reads++
  if reads==2{w.WriteHeader(503);w.Write([]byte(`{"contractVersion":"service-lasso-operator.v2","requestId":"request","error":{"code":"source_unavailable","messageKey":"operator.error.source_unavailable","retryability":"read_retry","fieldErrors":[]}}`));return}
  json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorMeta]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:meta})
 }));defer server.Close();client,err:=api.NewClient(server.URL,server.Client(),"source-token");if err!=nil{t.Fatal(err)};_,discovery,_:=terminalRefreshFixture(t);discovery.Capabilities[0].ID="service_meta_read"
 model:=operatorModel{ctx:context.Background(),client:client,epoch:1,service:"root",journey:"metadata",binding:selectedOperatorTestContext("a"),discovery:&discovery,width:100,height:18,focus:1}
 command:=model.load(nil);if command==nil{t.Fatal("missing metadata read")};reply:=command().(operatorLoaded);if reply.Err!=nil{t.Fatal(reply.Err)};updated,_:=model.Update(reply);model=updated.(operatorModel)
 retained:=model.meta;if retained==nil||!strings.Contains(model.View(),"favorite=true"){t.Fatal("useful original metadata absent")};model.beginEdit();if !model.draft.Dirty{t.Fatal("useful original metadata edit blocked")};originalText:=model.draft.Text;model.draft.Review=true
 command=model.load(nil);reply=command().(operatorLoaded);public,ok:=reply.Err.(*api.OperatorPublicError);if !ok||public.Code!="source_unavailable"{t.Fatal("actual unavailable refresh did not retain its public classification")};oldFailure:=reply;updated,_=model.Update(reply);model=updated.(operatorModel)
 view:=model.View();if model.meta!=retained||model.meta.Favorite==nil||!*model.meta.Favorite||model.meta.SourceRevision==nil||*model.meta.SourceRevision!=revision{t.Fatal("failed refresh erased original preference evidence")}
 if !model.formReadFailed()||!strings.Contains(view,"favorite=unknown")||strings.Contains(view,"favorite=true")||strings.Contains(view,"layout: automatic"){t.Fatal("failed refresh donated ready values")}
 if model.previewDraft()!=nil||model.submitDirect()!=nil||model.draft.Sent||model.draft.Text!=originalText||model.draft.Expected!=revision||!model.draft.Dirty{t.Fatal("failed read dispatched or erased original draft")}
 model.draft=operatorDraft{};if model.beginEdit()!=nil||model.draft.Dirty{t.Fatal("failed read admitted a new draft")}
 command=model.load(nil);reply=command().(operatorLoaded);if reply.Err!=nil{t.Fatal(reply.Err)};updated,_=model.Update(reply);model=updated.(operatorModel);model.beginEdit()
 if reads!=3||model.formReadFailed()||!model.draft.Dirty||model.draft.Expected!=revision||!strings.Contains(model.View(),"favorite=true"){t.Fatal("fresh successful same-selection read failed to restore useful values and editing")}
 updated,_=model.Update(oldFailure);model=updated.(operatorModel);if model.formReadFailed()||model.errorText!=""||model.draft.Expected!=revision{t.Fatal("obsolete failed reply denied a fresh admitted form")}
}
