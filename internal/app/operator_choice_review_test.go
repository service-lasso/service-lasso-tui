package app

import (
 "encoding/json"
 "strings"
 "testing"

 tea "github.com/charmbracelet/bubbletea"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

func TestOperatorStructuralChoiceAllRowsReachableWithoutValueChange(t *testing.T){
 closed:=false;minimum,maximum:=0,128
 scalar:=api.OperatorPayloadSchema{Types:[]string{"string"},Constraints:api.OperatorPayloadConstraints{MinLength:&minimum,MaxLength:&maximum},Properties:[]api.OperatorPayloadProperty{},Required:[]string{}}
 original:=json.RawMessage(`{"a":"first","b":"second","c":"third","d":"fourth","e":"last-visible-value"}`)
 root:=&api.OperatorPayloadSchema{Types:[]string{"object"},Constraints:api.OperatorPayloadConstraints{AdditionalProperties:&closed,Enum:[]json.RawMessage{original}},Properties:[]api.OperatorPayloadProperty{},Required:[]string{}}
 for _,name:=range []string{"a","b","c","d","e"}{root.Properties=append(root.Properties,api.OperatorPayloadProperty{Name:name,Schema:scalar})}
 if !root.ValidRoot(){t.Fatal("complete public structural choice schema rejected")};node:=newOperatorPayloadNode(root,true);if node==nil{t.Fatal("structural choice draft unavailable")}
 m:=operatorModel{width:6,height:1,draft:operatorDraft{Dirty:true,Payload:node,InlinePayload:true}};wrapped:=[]string{};for _,line:=range operatorPayloadEnumTree(original){wrapped=append(wrapped,operatorReviewWrap(line,m.width)...)}
 m.draft.ChoiceReview=true;observed:="";for i:=0;i<len(wrapped);i++{view:=m.View();observed+=view;if !strings.Contains(view,wrapped[i]){t.Fatal("narrow actual view omitted focused choice row")};m.editKey(tea.KeyMsg{Type:tea.KeyPgDown,Alt:true})}
 if m.draft.ChoiceOffset!=len(wrapped)-1{t.Fatal("actual narrow keyboard paging did not reach final row")}
 if !strings.Contains(observed,"last-v"){t.Fatal("last original structural member remained inaccessible")};raw,ok:=node.value();if !ok||string(raw)!=string(original){t.Fatal("choice inspection changed original typed value")}
 m.editKey(tea.KeyMsg{Type:tea.KeyHome,Alt:true});if m.draft.ChoiceReview{t.Fatal("choice review could not return to field")}
}
