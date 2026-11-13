package app

import (
 "encoding/json"
 "testing"
 "strings"
 tea "github.com/charmbracelet/bubbletea"

 "github.com/service-lasso/service-lasso-tui/internal/api"
)

func TestOperatorOverlayDraftKeepsNestedReplacementRequired(t *testing.T){
 closed:=false;minimum,maximum:=0,4096
 leaf:=api.OperatorPayloadSchema{Types:[]string{"string"},Constraints:api.OperatorPayloadConstraints{MinLength:&minimum,MaxLength:&maximum},Properties:[]api.OperatorPayloadProperty{},Required:[]string{}}
 nested:=api.OperatorPayloadSchema{Types:[]string{"object"},Constraints:api.OperatorPayloadConstraints{AdditionalProperties:&closed},Properties:[]api.OperatorPayloadProperty{{Name:"required",Schema:leaf}},Required:[]string{"required"}}
 root:=api.OperatorPayloadSchema{Types:[]string{"object"},Constraints:api.OperatorPayloadConstraints{AdditionalProperties:&closed,Enum:[]json.RawMessage{json.RawMessage(`{"nested":{"required":"original"}}`)}},Properties:[]api.OperatorPayloadProperty{{Name:"nested",Schema:nested}},Required:[]string{"nested"}}
 if !root.ValidRoot(){t.Fatal("original closed schema is invalid")}
 node:=newOperatorPayloadOverlay(&root);if node==nil{t.Fatal("useful optional root overlay could not initialize")}
 raw,ok:=node.value();if !ok||string(raw)!="{}"{t.Fatal("root overlay forced original required property or enum")}
 if root.Constraints.Enum==nil||len(root.Required)!=1{t.Fatal("editor projection mutated original schema")}
 child:=node.Properties["nested"];if child==nil||child.Required{t.Fatal("root property is not optional")}
 count:=node.extent();replacement,ok:=operatorPayloadNodeWithin(child.Schema,true,&count);if !ok{t.Fatal("nested subtree cannot initialize")};node.Properties["nested"]=replacement
 replacement.Properties["required"].Present=false;if _,ok=node.value();ok{t.Fatal("nested complete-replacement requirement lost")}
 replacement.Properties["required"].Present=true;replacement.Properties["required"].Text="new";if _,ok=node.value();!ok{t.Fatal("complete supplied subtree was blocked by root structural enum")}
}
func TestOperatorReferencePickerRetainsExactTokenAndRejectsLabelAuthority(t *testing.T){
 token:="opaque / token";ready:=api.OperatorAvailability{Supported:true,Available:true};reason:="source_unavailable"
 m:=operatorModel{draft:operatorDraft{Action:"declared_action_run",PayloadPolicy:api.OperatorPayloadPolicy{References:true},PayloadReferences:[]api.OperatorPayloadReference{{Label:"Unavailable",Availability:api.OperatorAvailability{Supported:true,ReasonCode:&reason}},{PayloadRef:&token,Label:"Stored payload",Availability:ready}}}}
 if !m.pickPayloadReference(false)||m.draft.PayloadRef==nil||*m.draft.PayloadRef!=token{t.Fatal("picker did not preserve exact issued token")}
 m.draft.Text="a typed basename";m.insertEditorText("arbitrary");if !m.saveEditorField()||*m.draft.PayloadRef!=token{t.Fatal("display label editing changed token authority")}
 foreign:="Stored payload";m.draft.PayloadRef=&foreign;if m.saveEditorField(){t.Fatal("label became reference authority")}
}

func TestOperatorAltMKeepsSetupManualAndDeclaredOverlayJourneys(t *testing.T){
 key:=tea.KeyMsg{Type:tea.KeyRunes,Runes:[]rune{'m'},Alt:true}
 setup:=operatorModel{width:120,height:30,draft:operatorDraft{Dirty:true,Action:"setup_step_run"}}
 updated,command:=setup.Update(key);setup=updated.(operatorModel)
 if command!=nil||!setup.draft.IncludeManual||setup.draft.Payload!=nil||setup.errorText!=""{t.Fatal("setup manual key entered declared payload overlay")}
 if !strings.Contains(setup.View(),"force=false include manual=true")||!strings.Contains(setup.View(),"Alt+m toggle manual steps"){t.Fatal("setup current manual option or existing key was not disclosed")}
 forceKey:=tea.KeyMsg{Type:tea.KeyCtrlF}
 updated,_=setup.Update(forceKey);setup=updated.(operatorModel);if !setup.draft.Force||!strings.Contains(setup.View(),"force=true include manual=true"){t.Fatal("setup force option was not toggled and disclosed")}
 updated,_=setup.Update(key);if updated.(operatorModel).draft.IncludeManual{t.Fatal("setup manual key did not toggle back")}
 token:="issued-reference";ready:=api.OperatorAvailability{Supported:true,Available:true}
 table:=api.OperatorPayloadTable{Root:0,Nodes:[]api.OperatorPayloadNode{{ID:0,Types:[]string{"object"},Constraints:api.OperatorPayloadConstraintMap{"additionalProperties":json.RawMessage("false")},Properties:[]api.OperatorPayloadEdge{{Name:"value",Node:1}},Required:[]string{"value"}},{ID:1,Types:[]string{"string"},Constraints:api.OperatorPayloadConstraintMap{"minLength":json.RawMessage("0"),"maxLength":json.RawMessage("4096")},Properties:[]api.OperatorPayloadEdge{},Required:[]string{}}}}
 if _,ok:=table.Tree();!ok{t.Fatal("overlay key fixture violates actual closed table")}
 declared:=operatorModel{actions:&api.OperatorActionForm{Actions:[]api.OperatorAction{{ActionID:"declared",InputFields:&table}}},draft:operatorDraft{Dirty:true,Action:"declared_action_run",DeclaredID:"declared",ReferenceMode:true,PayloadRef:&token,PayloadPolicy:api.OperatorPayloadPolicy{Inline:true,References:true,AllowMixed:true},PayloadReferences:[]api.OperatorPayloadReference{{PayloadRef:&token,Label:"Stored",Availability:ready}}}}
 updated,command=declared.Update(key);declared=updated.(operatorModel)
 if command!=nil||declared.draft.Payload==nil||!declared.draft.InlinePayload||declared.draft.ReferenceMode||declared.draft.IncludeManual||declared.draft.PayloadRef==nil||*declared.draft.PayloadRef!=token{t.Fatal("declared overlay key lost genuine reference or toggled setup policy")}
}
func TestOperatorTypedFieldShiftTabRetainsEditsAndSkipsReadonly(t *testing.T){
 ready:=api.OperatorAvailability{Supported:true,Available:true}
 fields:=[]api.OperatorField{
  {Selector:"first",Type:"string",Value:json.RawMessage(`"first original"`),Constraints:json.RawMessage(`{"maxBytes":4096}`),Availability:ready},
  {Selector:"readonly",Type:"string",Value:json.RawMessage(`"retained readonly"`),Constraints:json.RawMessage(`{"maxBytes":4096}`),Readonly:true,Availability:ready},
  {Selector:"last",Type:"string",Value:json.RawMessage(`"last original"`),Constraints:json.RawMessage(`{"maxBytes":4096}`),Availability:ready},
 }
 model:=operatorModel{draft:operatorDraft{Dirty:true,Action:"setup_step_run",Fields:fields,Values:map[string][]byte{},Selector:"first",Text:"first edited",Cursor:12}}
 updated,command:=model.Update(tea.KeyMsg{Type:tea.KeyShiftTab});model=updated.(operatorModel)
 if command!=nil||model.draft.FieldIndex!=2||model.draft.Selector!="last"||model.draft.Text!="last original"||string(model.draft.Values["first"])!=`"first edited"`{t.Fatal("backward field navigation lost literal edit or wrap")}
 model.setEditorText("last edited")
 updated,command=model.Update(tea.KeyMsg{Type:tea.KeyShiftTab});model=updated.(operatorModel)
 if command!=nil||model.draft.FieldIndex!=0||model.draft.Text!="first edited"||string(model.draft.Values["last"])!=`"last edited"`{t.Fatal("backward field navigation selected readonly field or lost retained edit")}
 updated,command=model.Update(tea.KeyMsg{Type:tea.KeyTab});model=updated.(operatorModel)
 if command!=nil||model.draft.FieldIndex!=2||model.draft.Text!="last edited"||string(fields[1].Value)!=`"retained readonly"`{t.Fatal("forward field navigation changed readonly data or retained edit")}
}