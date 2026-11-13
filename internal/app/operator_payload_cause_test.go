package app

import (
 "encoding/json"
 "errors"
 "strconv"
 "strings"
 "testing"

 tea "github.com/charmbracelet/bubbletea"

 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// Source cases only: no parser, compiler, or test execution is admitted.
func TestPayloadFailureCapturesMutableValuesAndOriginalOwner(t *testing.T){
 child:=&operatorPayloadNodeDraft{Present:true,Variant:"number",Text:"private-invalid-number",Schema:&api.OperatorPayloadSchema{}}
 root:=&operatorPayloadNodeDraft{Present:true,Variant:"array",Items:[]*operatorPayloadNodeDraft{child,child},Properties:map[string]*operatorPayloadNodeDraft{"original":child},Schema:&api.OperatorPayloadSchema{}}
 raw,ok,cause:=root.valueCause();if cause==nil||ok||raw!=nil{t.Fatal("nested marshal failure must return zero value and original cause")}
 source:=&api.OperatorActionForm{ServiceID:"private-form-owner"}
 m:=operatorModel{actions:source};m.editorFailure(operatorDraft{Payload:root},api.OperatorInvalidRequestCause(cause))
 failure:=m.editorFailures[0];if failure.Draft.Payload!=root||!errors.Is(failure.Err,cause){t.Fatal("original owner and cause lost")}
 snapshot:=failure.PayloadValues;if snapshot==root||snapshot.Items[0]==child||snapshot.Items[0]!=snapshot.Items[1]||snapshot.Properties["original"]!=snapshot.Items[0]{t.Fatal("captured graph relationships lost")}
 child.Text="changed";root.Items=nil;delete(root.Properties,"original");m.actions=&api.OperatorActionForm{}
 if failure.ActionSource!=source{t.Fatal("replacing current form lost original source owner")}
 if snapshot.Items[0].Text!="private-invalid-number"||len(snapshot.Items)!=2||snapshot.Properties["original"]==nil||snapshot.Items[0].EvidenceOwner!=child{t.Fatal("later payload editing mutated captured values or lost removed node owner")}
 encoded,err:=json.Marshal(failure);if err!=nil||string(encoded)!="{}"||strings.Contains(failure.Err.Error(),"private-invalid-number"){t.Fatal("private failure values entered public projection")}
}

func TestPayloadSemanticRefusalHasNoInventedCause(t *testing.T){
 node:=&operatorPayloadNodeDraft{Present:true,Variant:"boolean",Text:"neither",Schema:&api.OperatorPayloadSchema{}}
 raw,ok,cause:=node.valueCause();if raw!=nil||ok||cause!=nil{t.Fatal("semantic refusal must remain cause-free")}
}

func TestPayloadValidationFailureRetainsCauseAfterSuccessfulMarshal(t *testing.T){
 node:=&operatorPayloadNodeDraft{Present:true,Variant:"number",Text:"1e999",Schema:&api.OperatorPayloadSchema{}}
 raw,ok,cause:=node.valueCause();var number *strconv.NumError
 if raw!=nil||ok||!errors.As(cause,&number){t.Fatal("actual post-marshal numeric validation failure lost")}
}

func TestStorageCauseKeepsOriginalBehindClosedError(t *testing.T){
 original:=errors.New("private-native-owner-and-path")
 wrapped:=storageErrorCause(original)
 if !errors.Is(wrapped,original)||strings.Contains(wrapped.Error(),original.Error())||strings.Contains(api.SafeError(wrapped),original.Error()){t.Fatal("storage original must be private and reachable")}
 raw,err:=json.Marshal(wrapped);if err!=nil||strings.Contains(string(raw),original.Error()){t.Fatal("storage JSON exposed original")}
}

func TestEnumGestureCorruptionRetainsOriginalAndRejectedChoice(t *testing.T){
 closed:=false
 schema:=&api.OperatorPayloadSchema{Types:[]string{"object"},Properties:[]api.OperatorPayloadProperty{{Name:"flag",Schema:api.OperatorPayloadSchema{Types:[]string{"boolean"},Properties:[]api.OperatorPayloadProperty{},Required:[]string{}}}},Required:[]string{"flag"},Constraints:api.OperatorPayloadConstraints{AdditionalProperties:&closed,Enum:[]json.RawMessage{json.RawMessage(`{"flag":true}`),json.RawMessage(`{"flag":false}`)}}}
 if !schema.ValidRoot(){t.Fatal("fixture must begin as a valid complete source schema")};node:=newOperatorPayloadNode(schema,true);if node==nil{t.Fatal("valid enum draft unavailable")}
 // Fault injection after admission is not an alternate accepted source format.
 schema.Constraints.Enum[1]=json.RawMessage(`{"flag":@}`)
 m:=operatorModel{draft:operatorDraft{Payload:node,InlinePayload:true}}
 if !m.payloadKey(tea.KeyMsg{Type:tea.KeyRunes,Runes:[]rune{'n'},Alt:true}){t.Fatal("actual enum gesture did not reach choice parser")}
 var syntax *json.SyntaxError;if len(m.editorFailures)!=1||!errors.As(m.editorFailures[0].Err,&syntax){t.Fatal("actual rejected choice decoder cause lost")}
 failure:=m.editorFailures[0]
 if node.EnumCursor!=0||node.EnumIndex!=0||failure.Draft.Payload!=node||failure.Candidate==nil||failure.CandidatePayloadValues.EnumCursor!=1{t.Fatal("original choice or rejected candidate lost")}
 if m.draft.Sent||m.loading{t.Fatal("local choice failure marked dispatch")}
}
