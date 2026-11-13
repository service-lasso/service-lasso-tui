package app

import (
 "encoding/json"
 "strings"
 "testing"

 tea "github.com/charmbracelet/bubbletea"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

func TestOperatorPayloadLiteralEditorRenderRequestPreservation(t *testing.T){
 minimum,maximum:=0,4096
 scalar:=api.OperatorPayloadSchema{Types:[]string{"string"},Constraints:api.OperatorPayloadConstraints{MinLength:&minimum,MaxLength:&maximum},Properties:[]api.OperatorPayloadProperty{},Required:[]string{}};closed:=false
 schema:=&api.OperatorPayloadSchema{Types:[]string{"object"},Constraints:api.OperatorPayloadConstraints{AdditionalProperties:&closed},Properties:[]api.OperatorPayloadProperty{{Name:"text",Schema:scalar}},Required:[]string{"text"}}
 node:=newOperatorPayloadNode(schema,true);if node==nil{t.Fatal("object draft missing")}
 model:=operatorModel{draft:operatorDraft{Payload:node,PayloadIndex:1,InlinePayload:true,Values:map[string][]byte{}}}
 original:="wide界\r\n\t\x00\x07\x1b\u0085\u2028\u2029👩‍💻"
 model.showPayloadFocus();model.insertEditorText(original);if !model.saveEditorField(){t.Fatal("typed draft rejected original scalar")}
 rendered:=operatorText(model.draft.Text)
 for _,control:=range []string{"\r","\n","\t","\x00","\x07","\x1b","\u0085","\u2028","\u2029"}{if strings.Contains(rendered,control){t.Fatal("terminal boundary retained literal control")}}
 if model.draft.Text!=original||node.Properties["text"].Text!=original{t.Fatal("rendering copied escaped display back into draft")}
 payload,ok:=node.value();if !ok{t.Fatal("literal payload rejected")};input:=api.OperatorDeclaredInput{DeclaredActionID:"action",Payload:payload}
 inputWire,err:=json.Marshal(input);if err!=nil{t.Fatal(err)};request:=api.OperatorPreviewRequest{ServiceID:"root",ActionID:"declared_action_run",DefinitionRevision:"definition",ExpectedRevision:"action-revision",Input:inputWire}
 wire,err:=json.Marshal(request);if err!=nil{t.Fatal(err)};var members map[string]json.RawMessage;if json.Unmarshal(wire,&members)!=nil||len(members)!=4||members["serviceId"]!=nil{t.Fatal("service-scoped preview body is not exact four keys")};var decoded api.OperatorDeclaredInput;if json.Unmarshal(members["input"],&decoded)!=nil{t.Fatal("request input")};var actual map[string]string
 if json.Unmarshal(decoded.Payload,&actual)!=nil||actual["text"]!=original{t.Fatal("request changed literal payload")}
 if model.draft.Text!=original{t.Fatal("serialization changed editor state")}
}
func TestOperatorTypedStringNewlineTabKeysPreserveLiteralData(t *testing.T){
 minimum,maximum:=0,4096;closed:=false
 scalar:=api.OperatorPayloadSchema{Types:[]string{"string"},Constraints:api.OperatorPayloadConstraints{MinLength:&minimum,MaxLength:&maximum},Properties:[]api.OperatorPayloadProperty{},Required:[]string{}}
 schema:=&api.OperatorPayloadSchema{Types:[]string{"object"},Constraints:api.OperatorPayloadConstraints{AdditionalProperties:&closed},Properties:[]api.OperatorPayloadProperty{{Name:"text",Schema:scalar}},Required:[]string{"text"}}
 node:=newOperatorPayloadNode(schema,true);m:=operatorModel{draft:operatorDraft{Action:"declared_action_run",Dirty:true,Payload:node,PayloadIndex:1,InlinePayload:true}}
 m.showPayloadFocus();m.insertEditorText("line");m.editKey(tea.KeyMsg{Type:tea.KeyCtrlN});m.editKey(tea.KeyMsg{Type:tea.KeyCtrlT});m.insertEditorText("next");m.saveEditorField()
 raw,ok:=node.value();if !ok{t.Fatal("typed literal keys produced invalid payload")};var value map[string]string;if json.Unmarshal(raw,&value)!=nil||value["text"]!="line\n\tnext"{t.Fatal("literal newline/tab changed on submission serialization")}
 if strings.ContainsAny(operatorText(m.draft.Text),"\n\t"){t.Fatal("literal keys bypassed safe terminal boundary")}
 m.draft.ReferenceMode=true;before:=m.draft.Text;m.editKey(tea.KeyMsg{Type:tea.KeyCtrlN});if m.draft.Text!=before{t.Fatal("literal key widened reference picker")}
 m.draft.ReferenceMode=false;phrase:="confirm service-start 0123456789abcdef0123456789abcdef";confirmationID:="confirmation";expiry:="2026-10-08T00:00:00.000Z"
 m.draft.Preview=&api.OperatorPreview{PlanID:"plan",ServiceID:"service",ActionID:"service_start",DefinitionRevision:"definition",ExpectedRevision:"family",Generation:"generation",ModelEpoch:"epoch",Targets:[]string{"service"},Effects:[]string{"start_runtime"},Availability:api.OperatorAvailability{Supported:true,Available:true},Confirmation:api.OperatorConfirmation{Required:true,ID:&confirmationID,PhraseKey:&phrase,ExpiresAt:&expiry},ExpiresAt:expiry}
 if !api.OperatorConfirmationResponseValid(*m.draft.Preview,phrase){t.Fatal("genuine confirmation fixture rejected")};m.setEditorText(phrase);m.editKey(tea.KeyMsg{Type:tea.KeyCtrlT});if m.draft.Text!=phrase{t.Fatal("literal key widened confirmation response")}
}

func payloadTestArray(item *api.OperatorPayloadSchema,minimum int)*api.OperatorPayloadSchema{maximum:=128;return &api.OperatorPayloadSchema{Types:[]string{"array"},Constraints:api.OperatorPayloadConstraints{MinItems:&minimum,MaxItems:&maximum},Items:item,Properties:[]api.OperatorPayloadProperty{},Required:[]string{}}}
func TestOperatorPayloadAbsentNestedArraysStayLazy(t *testing.T){
 scalar:=&api.OperatorPayloadSchema{Types:[]string{"boolean"},Properties:[]api.OperatorPayloadProperty{},Required:[]string{}}
 huge:=payloadTestArray(payloadTestArray(scalar,128),128);closed:=false
 schema:=&api.OperatorPayloadSchema{Types:[]string{"object"},Constraints:api.OperatorPayloadConstraints{AdditionalProperties:&closed},Properties:[]api.OperatorPayloadProperty{{Name:"optional",Schema:*huge}},Required:[]string{}}
 node:=newOperatorPayloadNode(schema,true);if node==nil||node.extent()!=1||len(node.Properties["optional"].Items)!=0{t.Fatal("optional minimum eagerly expanded")}
 model:=operatorModel{draft:operatorDraft{Payload:node,PayloadIndex:1}}
 model.payloadKey(tea.KeyMsg{Type:tea.KeyRunes,Runes:[]rune{'u'},Alt:true})
 if node.Properties["optional"].Present||node.extent()!=1||model.errorText==""{t.Fatal("overbudget optional expansion failed to retain absent source choice")}
}
func TestOperatorPayloadUnionCanReachVariantAfterInfeasibleBranch(t *testing.T){
 scalar:=&api.OperatorPayloadSchema{Types:[]string{"boolean"},Properties:[]api.OperatorPayloadProperty{},Required:[]string{}}
 huge:=payloadTestArray(payloadTestArray(scalar,128),128);closed:=false;zero,maxString,maxArray:=0,4096,128
 schema:=&api.OperatorPayloadSchema{Types:[]string{"string","object","array","null"},Constraints:api.OperatorPayloadConstraints{AdditionalProperties:&closed,MinLength:&zero,MaxLength:&maxString,MinItems:&zero,MaxItems:&maxArray},Properties:[]api.OperatorPayloadProperty{{Name:"required",Schema:*huge}},Required:[]string{"required"},Items:scalar}
 node:=newOperatorPayloadNode(schema,true);if node==nil||node.Variant!="string"{t.Fatal("useful union initializer rejected")};model:=operatorModel{draft:operatorDraft{Payload:node}}
 key:=tea.KeyMsg{Type:tea.KeyRunes,Runes:[]rune{'v'},Alt:true};model.payloadKey(key)
 if node.Variant!="string"||node.VariantCursor!=1{t.Fatal("infeasible object replaced original value or lost attempted type")}
 model.payloadKey(key);if node.Variant!="array"||len(node.Items)!=0{t.Fatal("infeasible intermediate variant trapped later array")}
 model.payloadKey(key);if node.Variant!="null"{t.Fatal("null alternative lost")}
}
func TestOperatorPayloadEnumMinimumAndAttemptedChoice(t *testing.T){
 closed:=false;zero,maximum:=0,128;values:=make([]bool,128);large,_:=json.Marshal(map[string]any{"a":values})
 boolean:=&api.OperatorPayloadSchema{Types:[]string{"boolean"},Properties:[]api.OperatorPayloadProperty{},Required:[]string{}}
 array:=payloadTestArray(boolean,0)
 item:=&api.OperatorPayloadSchema{Types:[]string{"object","null"},Constraints:api.OperatorPayloadConstraints{AdditionalProperties:&closed,Enum:[]json.RawMessage{json.RawMessage(`{}`),large,json.RawMessage(`null`)}},Properties:[]api.OperatorPayloadProperty{{Name:"a",Schema:*array}},Required:[]string{}}
 root:=payloadTestArray(item,128);node:=newOperatorPayloadNode(root,true);if node==nil||node.extent()!=129{t.Fatal("smaller legal structural choice did not initialize finite array")}
 for i:=0;i<62;i++{node.Items[i].EnumIndex=1;node.Items[i].EnumCursor=1}
 if node.extent()>8192{t.Fatal("test original payload exceeded admitted limit")}
 model:=operatorModel{draft:operatorDraft{Payload:node,PayloadIndex:63}};key:=tea.KeyMsg{Type:tea.KeyRunes,Runes:[]rune{'n'},Alt:true};model.payloadKey(key)
 if node.Items[62].EnumIndex!=0||node.Items[62].EnumCursor!=1{t.Fatal("infeasible enum replaced prior value or lost attempted choice")}
 model.payloadKey(key);if node.Items[62].EnumIndex!=2{t.Fatal("infeasible enum trapped later null choice")}
 // Schema minimum uses the actual best original choice, regardless of order.
 item.Constraints.Enum=[]json.RawMessage{large,json.RawMessage(`{}`)};root.Constraints.MinItems=&maximum;root.Constraints.MaxItems=&maximum;array.Constraints.MinItems=&zero
 node=newOperatorPayloadNode(root,true);if node==nil||node.Items[0].EnumIndex!=1{t.Fatal("large first choice made useful required array unavailable")}
}
