package app

import (
 "context"
 "encoding/json"
 "net/http"
 "net/http/httptest"
 "testing"

 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// Source regressions are UNRUN. Synthetic contexts do not establish an issuer grant.
func TestOperatorGlobalQueueDraftKeepsOriginalRowServiceAndContext(t *testing.T){
 for _,rotation:=range []string{"unchanged","before","after"}{t.Run(rotation,func(t *testing.T){
  binding:=selectedOperatorTestContext("a");foreign:=selectedOperatorTestContext("b");reads,contexts,posts:=0,0,0;plan:="plan"
  row:=api.OperatorQueueRow{RowID:"original-row",ServiceID:"root",Kind:"manual",Status:"open",Severity:"info",CreatedAt:"2026-10-08T00:00:00.000Z",Summary:"original",PlanID:&plan}
  other:=row;other.RowID="other-row";other.ServiceID="other"
  preview:=api.OperatorPreview{PlanID:"plan",ServiceID:"root",ActionID:"queue_plan_execute",DefinitionRevision:"definition",ExpectedRevision:"revision",Generation:"generation",ModelEpoch:"epoch",Targets:[]string{},Effects:[]string{},Availability:api.OperatorAvailability{Supported:true,Available:true},ExpiresAt:"2099-10-08T00:00:00.000Z"}
  if !api.OperatorConfirmationResponseValid(preview,""){t.Fatal("invalid source preview fixture")}
  server:=httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter,r *http.Request){
   switch r.URL.Path{
   case "/api/operator/v2/queue":
    if r.Method!="GET"||r.URL.Query().Get("serviceId")!=""{t.Error("global read was retargeted");w.WriteHeader(400);return};reads++
    page:=api.OperatorPage[api.OperatorQueueRow]{SnapshotID:"snapshot",SourceRevision:"revision",Rows:[]api.OperatorQueueRow{row,other},ExpiresAt:"2099-10-08T00:00:00.000Z"}
    json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorPage[api.OperatorQueueRow]]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:page})
   case "/api/operator/v2/reconciliation-context":
    contexts++;current:=binding;if rotation=="before"||rotation=="after"&&contexts==2{current=foreign}
    json.NewEncoder(w).Encode(struct{ContractVersion string `json:"contractVersion"`;Context api.OperatorContext `json:"context"`}{"service-lasso-durable-reconciliation-context.v1",current})
   case "/api/operator/v2/queue/original-row/preview":
    if r.Method!="POST"{t.Error("unexpected preview method");w.WriteHeader(400);return};var body struct{ExpectedRevision string `json:"expectedRevision"`};if json.NewDecoder(r.Body).Decode(&body)!=nil||body.ExpectedRevision!="revision"{t.Error("original revision lost");w.WriteHeader(400);return};posts++
    json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorPreview]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:preview})
   default:t.Error("unrelated row or mutation requested");w.WriteHeader(400)
   }
  }));defer server.Close()
  client,err:=api.NewClient(server.URL,server.Client(),"source-token");if err!=nil{t.Fatal(err)};_,discovery,_:=terminalRefreshFixture(t);discovery.Capabilities[0].ID="read_queue"
  model:=operatorModel{ctx:context.Background(),client:client,epoch:1,journey:"queue",binding:binding,discovery:&discovery}
  command:=model.load(nil);if command==nil{t.Fatal("missing global queue read")};loaded:=command().(operatorLoaded);if loaded.Err!=nil{t.Fatal(loaded.Err)};updated,_:=model.Update(loaded);model=updated.(operatorModel)
  model.beginEdit();if !model.draftSelectionCurrent()||model.draft.ReadService!=""||model.draft.Service!="root"||model.draft.QueueRow==nil||model.draft.QueueRow.RowID!="original-row"{t.Fatal("global scope replaced original selected row binding")}
  command=model.previewDraft();if command==nil{t.Fatal("useful global queue preview blocked")};reply:=command().(operatorPreviewed);updated,_=model.Update(reply);model=updated.(operatorModel)
  if reads!=1||model.draft.Service!="root"||model.draft.Expected!="revision"||model.draft.Context!=binding||model.draft.Sent{t.Fatal("original draft changed or mutation replayed")}
  if rotation=="unchanged"{if reply.Err!=nil||model.draft.Preview==nil||posts!=1||contexts!=2{t.Fatal("unchanged useful preview rejected")}}else{want:=1;if rotation=="before"{want=0};if reply.Err==nil||model.draft.Preview!=nil||posts!=want{t.Fatal("foreign context supplied preview authority")}}
 })}
}

func TestOperatorGlobalInboxDraftAndForeignLocalGuards(t *testing.T){
 for _,change:=range []string{"unchanged","context","service","journey","filter","connection","selection"}{t.Run(change,func(t *testing.T){
  _,discovery,_:=terminalRefreshFixture(t);binding:=selectedOperatorTestContext("a")
  model:=operatorModel{epoch:1,journey:"inbox",binding:binding,discovery:&discovery,inbox:operatorPageState[api.OperatorInboxRow]{Revision:"revision",Expiry:"2099-10-08T00:00:00.000Z",Rows:[]api.OperatorInboxRow{{RowID:"original-row",ServiceID:"root",Kind:"service",Source:"runtime",Severity:"info",Status:"unread",Visibility:"visible",CreatedAt:"2026-10-08T00:00:00.000Z",Summary:"original",CapabilityIDs:[]string{"inbox_mark_read"}},{RowID:"other-row",ServiceID:"other"}}}}
  // Explicit synthetic admitted-read fixture; the queue case above uses real HTTP.
  model.acceptPageSelection(model.ticket());otherTicket:=model.ticket();otherTicket.Journey="queue";otherTicket.Filter="all";model.acceptPageSelection(otherTicket);model.beginEdit();if !model.draftSelectionCurrent()||model.draft.Service!="root"||model.draft.ReadService!=""||model.draft.InboxRow.RowID!="original-row"{t.Fatal("global inbox lost original selected row")}
  if change=="unchanged"{if model.previewDraft()!=nil||!model.draft.Review||model.draft.Sent{t.Fatal("useful direct review blocked")};return}
  model.draft.Text="retained literal";model.draft.Review=true;original:=model.draft.InboxRow
  switch change{case "context":model.binding=selectedOperatorTestContext("b");case "service":model.service="other";case "journey":model.journey="queue";case "filter":model.filters=map[string]string{"inbox":"unread"};case "connection":model.epoch++;case "selection":model.selection++}
  // A separately admitted current page is insufficient to requalify this draft.
  model.acceptPageSelection(model.ticket())
  if model.previewDraft()!=nil||model.submitDirect()!=nil{t.Fatal("foreign local selection dispatched")}
  model.draft.Preview=&api.OperatorPreview{ServiceID:"root"};if model.submitPreview()!=nil{t.Fatal("foreign preview dispatched")}
  if model.draft.Text!="retained literal"||model.draft.InboxRow!=original||model.draft.Service!="root"||model.draft.Expected!="revision"||model.draft.Context!=binding||model.draft.Sent||model.draft.Idempotency!=""{t.Fatal("local denial transformed original draft")}
 })}
}

func TestOperatorGlobalInboxDirectReadKeepsOriginalRow(t *testing.T){
 binding:=selectedOperatorTestContext("a");_,discovery,_:=terminalRefreshFixture(t);discovery.MutationSemantics="all_writer_atomic_revision";discovery.Capabilities[0].ID="inbox_mark_read";discovery.Capabilities=append(discovery.Capabilities,api.OperatorCapability{ID:"read_inbox",Availability:api.OperatorAvailability{Supported:true,Available:true},RequiredProfile:"observer",RequiredScopes:[]string{"service-lasso:read"},ConfirmationPolicy:"none",BindingRequirements:[]string{}})
 row:=api.OperatorInboxRow{RowID:"original-row",ServiceID:"root",Kind:"service",Source:"runtime",Severity:"info",Status:"unread",Visibility:"visible",CreatedAt:"2026-10-08T00:00:00.000Z",Summary:"original",CapabilityIDs:[]string{"inbox_mark_read"}};other:=row;other.RowID="other-row";other.ServiceID="other";global,own,patches,contexts:=0,0,0,0
 server:=httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter,r *http.Request){
  switch r.URL.Path{
  case "/api/operator/v2/inbox":
   rows:=[]api.OperatorInboxRow{row};if r.URL.Query().Get("serviceId")==""{global++;rows=append(rows,other)}else if r.URL.Query().Get("serviceId")=="root"{own++}else{t.Error("fresh read substituted another service");w.WriteHeader(400);return}
   page:=api.OperatorPage[api.OperatorInboxRow]{SnapshotID:"snapshot",SourceRevision:"revision",Rows:rows,ExpiresAt:"2099-10-08T00:00:00.000Z"};json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorPage[api.OperatorInboxRow]]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:page})
  case "/api/operator/v2/reconciliation-context":contexts++;json.NewEncoder(w).Encode(struct{ContractVersion string `json:"contractVersion"`;Context api.OperatorContext `json:"context"`}{"service-lasso-durable-reconciliation-context.v1",binding})
  case "/api/operator/v2/discovery":json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorDiscovery]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:discovery})
  case "/api/operator/v2/inbox/original-row":
   var body struct{ExpectedRevision string `json:"expectedRevision"`;Op string `json:"op"`};if r.Method!="PATCH"||json.NewDecoder(r.Body).Decode(&body)!=nil||body.ExpectedRevision!="revision"||body.Op!="mark_read"{t.Error("lost original direct request");w.WriteHeader(400);return};patches++
   json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorInboxMutation]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:api.OperatorInboxMutation{RowID:row.RowID,SourceRevision:"revision-after",Status:"read",Visibility:"visible"}})
  default:t.Error("unrelated row requested");w.WriteHeader(400)
  }
 }));defer server.Close();client,err:=api.NewClient(server.URL,server.Client(),"source-token");if err!=nil{t.Fatal(err)}
 model:=operatorModel{ctx:context.Background(),client:client,epoch:1,journey:"inbox",binding:binding,discovery:&discovery};command:=model.load(nil);if command==nil{t.Fatal("missing global inbox read")};loaded:=command().(operatorLoaded);if loaded.Err!=nil{t.Fatal(loaded.Err)};updated,_:=model.Update(loaded);model=updated.(operatorModel);model.beginEdit();model.draft.Review=true
 command=model.submitDirect();if command==nil{t.Fatal("useful global direct action blocked")};reply:=command().(operatorDirectResult)
 if reply.Err!=nil||!reply.Dispatched||global!=1||own!=1||patches!=1||contexts!=2||model.draft.ReadService!=""||model.draft.Service!="root"||model.draft.InboxRow.RowID!=row.RowID{t.Fatal("global draft lost original row during fresh direct reads")}
}

func TestOperatorUpdateDraftLifecycleReadRetainsOriginalBinding(t *testing.T){
 for _,rotation:=range []string{"unchanged","before","after","local"}{t.Run(rotation,func(t *testing.T){
  binding:=selectedOperatorTestContext("a");foreign:=selectedOperatorTestContext("b");definition,generation,epoch,revision:="definition","generation","epoch","original-revision";reason:="unsupported_capability";ready:=api.OperatorAvailability{Supported:true,Available:true}
  form:=api.OperatorLifecycleAvailability{ServiceID:"root",DefinitionRevision:&definition,Generation:&generation,ModelEpoch:&epoch,Availability:ready,Capabilities:[]api.OperatorCapability{},MutationBindings:[]api.OperatorMutationBinding{}}
  for _,id:=range []string{"service_install","service_configure","service_start","service_stop","service_restart","update_check","update_download","service_reload"}{availability:=ready;var expected *string=&revision;if id=="service_reload"{availability=api.OperatorAvailability{ReasonCode:&reason};expected=nil};form.Capabilities=append(form.Capabilities,api.OperatorCapability{ID:id,Availability:availability,RequiredProfile:"operator",RequiredScopes:[]string{"service-lasso:read"},ConfirmationPolicy:"none",BindingRequirements:[]string{"definition_revision","runtime_generation","model_epoch"}});form.MutationBindings=append(form.MutationBindings,api.OperatorMutationBinding{ActionID:id,Availability:availability,ExpectedRevision:expected})}
  if !form.Valid(){t.Fatal("invalid actual lifecycle source fixture")};contexts,reads:=0,0
  server:=httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter,r *http.Request){switch r.URL.Path{
  case "/api/operator/v2/reconciliation-context":contexts++;current:=binding;if rotation=="before"||rotation=="after"&&contexts==2{current=foreign};json.NewEncoder(w).Encode(struct{ContractVersion string `json:"contractVersion"`;Context api.OperatorContext `json:"context"`}{"service-lasso-durable-reconciliation-context.v1",current})
  case "/api/operator/v2/services/root/lifecycle-availability":if r.Method!="GET"{t.Error("lifecycle read became mutation");w.WriteHeader(400);return};reads++;json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorLifecycleAvailability]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:form})
  default:t.Error("original target replaced");w.WriteHeader(400)
  }}));defer server.Close();client,err:=api.NewClient(server.URL,server.Client(),"source-token");if err!=nil{t.Fatal(err)};_,discovery,_:=terminalRefreshFixture(t)
  model:=operatorModel{ctx:context.Background(),client:client,epoch:1,service:"root",journey:"updates",binding:binding,discovery:&discovery,updates:&api.OperatorUpdateForm{ServiceID:"root",DefinitionRevision:definition}}
  // Explicit admitted update-form fixture; lifecycle binding itself uses HTTP.
  model.acceptPageSelection(model.ticket());command:=model.beginEdit();if command==nil{t.Fatal("useful original update read missing")};reply:=command().(operatorDraftBinding)
  // Synthetic changed retained-draft identity isolates the final draft guard;
  // the actual HTTP reply and its captured ticket remain untouched.
  if rotation=="local"{model.draft.ReadSelection++}
  updated,_:=model.Update(reply);model=updated.(operatorModel)
  if model.draft.Service!="root"||model.draft.Context!=binding||model.draft.Action!="update_check"||model.draft.Sent{t.Fatal("lifecycle reply replaced original draft")}
  if rotation=="unchanged"{if reply.Err!=nil||model.draft.Expected!=revision||reads!=1||contexts!=2{t.Fatal("useful unchanged lifecycle binding rejected")}}else{want:=1;if rotation=="before"{want=0};if model.draft.Expected!=""||reads!=want||rotation!="local"&&reply.Err==nil{t.Fatal("foreign lifecycle binding acquired revision")}}
 })}
}
