package app

import (
 "context"
 "encoding/json"
 "net/http"
 "net/http/httptest"
 "strings"
 "testing"

 tea "github.com/charmbracelet/bubbletea"
 "github.com/charmbracelet/x/ansi"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// Actual HTTP/load/reducer/key/view sources, UNRUN; no native/issuer receipt.
func TestOperatorDeclaredReadInspectionNeverCreatesPayloadDraft(t *testing.T){
 for _,variant:=range []string{"ready","denied","no-schema","unavailable-schema"}{t.Run(variant,func(t *testing.T){
  denied:=variant=="denied"||variant=="unavailable-schema"
  ready:=api.OperatorAvailability{Supported:true,Available:true};availability:=ready;revision:="revision";var expected *string=&revision;reason:="permission_denied";if denied{availability=api.OperatorAvailability{Supported:true,ReasonCode:&reason};expected=nil}
  token:="opaque-payload-token";table:=api.OperatorPayloadTable{Root:0,Nodes:[]api.OperatorPayloadNode{{ID:0,Types:[]string{"object"},Constraints:api.OperatorPayloadConstraintMap{"additionalProperties":json.RawMessage(`false`)},Properties:[]api.OperatorPayloadEdge{{Name:"message",Node:1}},Required:[]string{"message"}},{ID:1,Types:[]string{"string"},Constraints:api.OperatorPayloadConstraintMap{"enum":json.RawMessage(`["withheld-enum-value"]`),"maxLength":json.RawMessage(`4096`)},Properties:[]api.OperatorPayloadEdge{},Required:[]string{}}}}
  action:=api.OperatorAction{ActionID:"declared",Description:"Declared action",DefinitionRevision:"definition",PayloadPolicy:api.OperatorPayloadPolicy{Inline:true,References:true,AllowMixed:true,Required:true},InputFields:&table,PayloadReferences:[]api.OperatorPayloadReference{{PayloadRef:&token,Label:"Stored payload",Availability:ready}},RequiredProfile:"operator",RequiredScopes:[]string{"service-lasso:read"},DeclaredPermission:"declared-action",Availability:availability,ExpectedRevision:expected}
  if variant=="no-schema"||variant=="unavailable-schema"{action.InputFields=nil}
  form:=api.OperatorActionForm{ServiceID:"root",DefinitionRevision:"definition",Availability:ready,Actions:[]api.OperatorAction{action}};if !form.Valid(){t.Fatal("invalid actual declared form fixture")};original,_:=json.Marshal(form);reads:=0
  server:=httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter,r *http.Request){if r.Method!="GET"||r.URL.Path!="/api/operator/v2/services/root/action-form"||r.Header.Get("x-service-lasso-admin-token")!="source-token"{t.Error("inspection sent mutation or changed source");w.WriteHeader(400);return};reads++;json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorActionForm]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:form})}));defer server.Close()
  client,err:=api.NewClient(server.URL,server.Client(),"source-token");if err!=nil{t.Fatal(err)};_,discovery,_:=terminalRefreshFixture(t)
  model:=operatorModel{ctx:context.Background(),client:client,epoch:1,service:"root",journey:"declared_actions",focus:1,width:1000,height:30,binding:selectedOperatorTestContext("a"),discovery:&discovery}
  command:=model.load(nil);reply:=command().(operatorLoaded);if reply.Err!=nil{t.Fatal(reply.Err)};updated,_:=model.Update(reply);model=updated.(operatorModel)
  updated,command=model.Update(tea.KeyMsg{Type:tea.KeyPgDown,Alt:true});model=updated.(operatorModel)
  rows:=strings.Join(model.declaredInspectionRows(),"\n");if command!=nil||!model.declaredInspectionCurrent()||model.draft.Dirty||model.draft.Payload!=nil||model.draft.PayloadRef!=nil||model.rowCount()!=1||model.row!=0{t.Fatal("readonly inspection changed selectable action/draft authority")}
  expectedRows:=[]string{"Stored payload: available","Schedule/concurrency/failure metadata: unavailable in this contract"};if action.InputFields!=nil{expectedRows=append(expectedRows,"Node 0: object","Property message -> node 1","Required properties: message","Node 1: string","Constraint enum: 1 declared choices (values withheld)")}else{status:="not declared";if denied{status="unavailable"};expectedRows=append(expectedRows,"Payload schema: "+status)};for _,text:=range expectedRows{if !strings.Contains(rows,text){t.Fatal("exact supported declaration was not inspectable")}}
  if strings.Contains(rows,token)||strings.Contains(rows,"withheld-enum-value"){t.Fatal("private reference token or enum bytes displayed")}
  for _,key:=range []tea.KeyMsg{{Type:tea.KeyEnter},{Type:tea.KeyRunes,Runes:[]rune{'e'}},{Type:tea.KeyRunes,Runes:[]rune{'i'},Alt:true}}{updated,command=model.Update(key);model=updated.(operatorModel);if command!=nil||model.draft.Dirty||model.draft.Sent||model.draft.Payload!=nil{t.Fatal("inspection allocated editable draft or mutation command")}}
  updated,_=model.Update(tea.WindowSizeMsg{Width:12,Height:1});model=updated.(operatorModel)
  for i:=0;i<len(model.declaredInspectionRows());i++{view:=model.View();if len(strings.Split(view,"\n"))>1||ansi.StringWidth(view)>12||strings.ContainsRune(view,'\x1b')||strings.Contains(view,token){t.Fatal("narrow inspection escaped viewport/privacy bounds")};updated,command=model.Update(tea.KeyMsg{Type:tea.KeyDown});model=updated.(operatorModel);if command!=nil||model.row!=0||model.draft.Dirty{t.Fatal("inspection scroll changed action-row authority")}}
  updated,_=model.Update(tea.WindowSizeMsg{Width:0,Height:0});model=updated.(operatorModel);if model.View()!=""{t.Fatal("zero viewport widened inspection")}
  updated,_=model.Update(tea.KeyMsg{Type:tea.KeyEsc});model=updated.(operatorModel);if model.declaredReadReview||model.draft.Dirty||model.row!=0{t.Fatal("return changed original action/draft")}
  retained,_:=json.Marshal(model.actions);if string(retained)!=string(original)||reads!=1{t.Fatal("readonly inspection changed original table/reference bytes or reread source")}
 })}
}

func TestOperatorDeclaredInspectionRejectsForeignAndFailedFormSelections(t *testing.T){
 for _,change:=range []string{"context","service","failed","loading"}{t.Run(change,func(t *testing.T){
  model:=operatorModel{epoch:1,service:"root",journey:"declared_actions",focus:1,width:80,height:10,binding:selectedOperatorTestContext("a"),actions:&api.OperatorActionForm{Availability:api.OperatorAvailability{Supported:true,Available:true},Actions:[]api.OperatorAction{{ActionID:"original",Description:"original metadata"}}}}
  // Explicit synthetic source-selection negative; actual HTTP positive above.
  model.acceptPageSelection(model.ticket());model.moveDeclaredInspection(true);if !model.declaredInspectionCurrent(){t.Fatal("original selection cannot inspect")}
  switch change{case "context":model.binding=selectedOperatorTestContext("b");case "service":model.service="other";case "failed":model.markFormReadFailed();case "loading":model.loading=true}
  if model.declaredInspectionCurrent()||len(model.declaredInspectionRows())!=0||change!="loading"&&strings.Contains(model.View(),"original metadata"){t.Fatal("foreign/failed/pending selection exposed old inspection")}
 })}
}

func TestOperatorDeclaredInspectionKeepsRetainedUnknownRequestReadOnly(t *testing.T){
 model:=operatorModel{epoch:1,service:"root",journey:"declared_actions",focus:1,width:80,height:10,binding:selectedOperatorTestContext("a"),actions:&api.OperatorActionForm{Availability:api.OperatorAvailability{Supported:true,Available:true},Actions:[]api.OperatorAction{{ActionID:"original",Description:"current metadata"}}},draft:operatorDraft{Sent:true,Dirty:true,Action:"declared_action_run",Text:"retained-private-literal",Input:[]byte("retained-private-input")}}
 // Synthetic retained-request custody vector; genuine HTTP inspection above.
 model.acceptPageSelection(model.ticket());original,_:=privateOperatorDraftSnapshot(model.draft)
 updated,command:=model.Update(tea.KeyMsg{Type:tea.KeyPgDown,Alt:true});model=updated.(operatorModel);if command!=nil||!model.declaredInspectionCurrent(){t.Fatal("retained unknown request blocked safe readonly inspection")}
 updated,command=model.Update(tea.KeyMsg{Type:tea.KeyEnter});model=updated.(operatorModel);retained,_:=privateOperatorDraftSnapshot(model.draft)
 if command!=nil||string(retained)!=string(original)||strings.Contains(model.View(),"retained-private-"){t.Fatal("readonly inspection dispatched, changed or disclosed retained request input")}
}

func TestOperatorDeclaredInspectionDoesNotProjectUnsupportedDefaultMetadata(t *testing.T){
 table:=api.OperatorPayloadTable{Root:0,Nodes:[]api.OperatorPayloadNode{{ID:0,Types:[]string{"object"},Constraints:api.OperatorPayloadConstraintMap{"default":json.RawMessage(`{"private":"withheld-default"}`)},Properties:[]api.OperatorPayloadEdge{},Required:[]string{}}}}
 model:=operatorModel{epoch:1,service:"root",journey:"declared_actions",focus:1,width:1000,binding:selectedOperatorTestContext("a"),actions:&api.OperatorActionForm{Availability:api.OperatorAvailability{Supported:true,Available:true},Actions:[]api.OperatorAction{{ActionID:"original",InputFields:&table}}}}
 // Synthetic invalid table: the actual HTTP codec rejects it before loading.
 model.acceptPageSelection(model.ticket());rows:=strings.Join(model.declaredInspectionRows(),"\n")
 if !strings.Contains(rows,"Payload schema: unsupported")||strings.Contains(rows,"withheld-default")||strings.Contains(rows,"Node 0:"){t.Fatal("unsupported table donated partial/default metadata")}
}
