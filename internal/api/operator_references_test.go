package api

import (
 "encoding/json"
 "strconv"
 "testing"
)

func operatorReferenceFormFixture() OperatorActionForm {
 revision,token:="revision","opaque / reference";ready:=OperatorAvailability{Supported:true,Available:true}
 table:=&OperatorPayloadTable{Root:0,Nodes:[]OperatorPayloadNode{
  {ID:0,Types:[]string{"object"},Constraints:OperatorPayloadConstraintMap{"additionalProperties":json.RawMessage("false")},Properties:[]OperatorPayloadEdge{{Name:"nested",Node:1},{Name:"title",Node:3}},Required:[]string{"nested","title"}},
  {ID:1,Types:[]string{"object"},Constraints:OperatorPayloadConstraintMap{"additionalProperties":json.RawMessage("false")},Properties:[]OperatorPayloadEdge{{Name:"required",Node:2}},Required:[]string{"required"}},
  {ID:2,Types:[]string{"string"},Constraints:OperatorPayloadConstraintMap{},Properties:[]OperatorPayloadEdge{},Required:[]string{}},
  {ID:3,Types:[]string{"string"},Constraints:OperatorPayloadConstraintMap{},Properties:[]OperatorPayloadEdge{},Required:[]string{}},
 }}
 action:=OperatorAction{ActionID:"run",Description:"Run",DefinitionRevision:revision,ExpectedRevision:&revision,PayloadPolicy:OperatorPayloadPolicy{Required:true,Inline:true,References:true,AllowMixed:true},InputFields:table,PayloadReferences:[]OperatorPayloadReference{{PayloadRef:&token,Label:"Stored payload",Availability:ready}},RequiredProfile:"operator",RequiredScopes:[]string{},DeclaredPermission:"run",Availability:ready}
 return OperatorActionForm{ServiceID:"service",DefinitionRevision:revision,Actions:[]OperatorAction{action},Availability:ready}
}
func TestOperatorReferenceMembershipAndRootOverlay(t *testing.T) {
 form:=operatorReferenceFormFixture();if !form.Valid(){t.Fatal("complete roster fixture rejected")}
 token:=*form.Actions[0].PayloadReferences[0].PayloadRef
 input:=OperatorDeclaredInput{DeclaredActionID:"run",PayloadRef:&token,Payload:json.RawMessage(`{"title":"override"}`)}
 if !OperatorDeclaredInputValid(form,input){t.Fatal("partial root overlay narrowed to full required payload")}
 input.Payload=json.RawMessage(`{"nested":{}}`);if OperatorDeclaredInputValid(form,input){t.Fatal("incomplete supplied nested replacement admitted")}
 input.Payload=json.RawMessage(`{"nested":{"required":"replacement"}}`);if !OperatorDeclaredInputValid(form,input){t.Fatal("complete supplied subtree rejected")}
 foreign:="another opaque reference";input.PayloadRef=&foreign;if OperatorDeclaredInputValid(form,input){t.Fatal("opaque syntax became authority")}
 input.PayloadRef=nil;input.Payload=json.RawMessage(`{"title":"override"}`);if OperatorDeclaredInputValid(form,input){t.Fatal("ordinary inline root lost required properties")}
 schema,ok:=form.Actions[0].InputFields.Tree();if !ok{t.Fatal("original schema rejected")};selectors:=operatorPayloadRequestSelectors(schema,json.RawMessage(`{"title":"override"}`),true);found:=false
 for _,selector:=range selectors{if selector=="/input/payload/title"{found=true};if selector=="/input/payload/nested"||selector=="/input/payload/nested/required"{t.Fatal("reference-private omitted fields became diagnostics authority")}}
 if !found{t.Fatal("actual supplied root overlay lost logical diagnostics")}
}
func TestOperatorReferenceCompleteFormClosedRoster(t *testing.T) {
 form:=operatorReferenceFormFixture();raw,_:=json.Marshal(form);var decoded OperatorActionForm
 if decodeOperator(raw,&decoded,OperatorResponseBytes)!=nil||!decoded.Valid(){t.Fatal("complete action form codec rejected roster")}
 form.Actions[0].PayloadReferences=nil;raw,_=json.Marshal(form);if decodeOperator(raw,&decoded,OperatorResponseBytes)==nil&&decoded.Valid(){t.Fatal("null mandatory roster admitted")}
 form=operatorReferenceFormFixture();second:=form.Actions[0];second.ActionID="other";form.Actions=append(form.Actions,second);if form.Valid(){t.Fatal("same token admitted across actions")}
 form=operatorReferenceFormFixture();form.Actions[0].InputFields=nil;if !form.Valid(){t.Fatal("ready reference-only declaration rejected")}
 form.Actions[0].PayloadReferences=[]OperatorPayloadReference{};if form.Valid(){t.Fatal("required action admitted without any genuine input channel")}
 form=operatorReferenceFormFixture();form.Actions[0].DefinitionRevision="another definition";if form.Valid(){t.Fatal("selected form/row definition relation lost")}
}
func TestOperatorReferenceGlobalRosterBoundaryAndClosedRows(t *testing.T){
 form:=operatorReferenceFormFixture();ready:=form.Availability;form.Actions[0].PayloadReferences=[]OperatorPayloadReference{}
 for i:=0;i<128;i++{token:="issued "+strconv.Itoa(i);form.Actions[0].PayloadReferences=append(form.Actions[0].PayloadReferences,OperatorPayloadReference{PayloadRef:&token,Label:"Stored payload",Availability:ready})}
 raw,_:=json.Marshal(form);var decoded OperatorActionForm;if decodeOperator(raw,&decoded,OperatorResponseBytes)!=nil||!decoded.Valid(){t.Fatal("complete 128-choice roster rejected")}
 second:=form.Actions[0];second.ActionID="other";token:="additional issued token";second.PayloadReferences=[]OperatorPayloadReference{{PayloadRef:&token,Label:"Other payload",Availability:ready}};form.Actions=append(form.Actions,second);if form.Valid(){t.Fatal("across-form 129 choice boundary escaped per-action limit")}
 form=operatorReferenceFormFixture();raw,_=json.Marshal(form);var body map[string]any;if json.Unmarshal(raw,&body)!=nil{t.Fatal("fixture marshal failed")};action:=body["actions"].([]any)[0].(map[string]any)
 delete(action,"payloadReferences");raw,_=json.Marshal(body);if decodeOperator(raw,&decoded,OperatorResponseBytes)==nil{t.Fatal("absent mandatory roster admitted")}
 action["payloadReferences"]=[]any{map[string]any{"payloadRef":"issued","label":"Stored payload","availability":map[string]any{"supported":true,"available":true,"reasonCode":nil},"privatePath":"hidden"}};raw,_=json.Marshal(body);if decodeOperator(raw,&decoded,OperatorResponseBytes)==nil{t.Fatal("unknown reference field admitted")}
}
func TestOperatorDisclosedUnreadyActionDoesNotGrantReferenceUse(t *testing.T){
 form:=operatorReferenceFormFixture();reason:="dependency_blocked";form.Actions[0].Availability=OperatorAvailability{Supported:true,ReasonCode:&reason};form.Actions[0].ExpectedRevision=nil
 if !form.Valid(){t.Fatal("operational denial invented a disclosure denial")}
 token:=*form.Actions[0].PayloadReferences[0].PayloadRef
 if OperatorDeclaredInputValid(form,OperatorDeclaredInput{DeclaredActionID:"run",PayloadRef:&token}){t.Fatal("disclosed metadata granted unavailable action mutation")}
}
