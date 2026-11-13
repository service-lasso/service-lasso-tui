package api

import (
 "encoding/json"
 "strings"
 "testing"
)

func TestOperatorPayloadStructuralNumericEquality(t *testing.T){
 cases:=[][2]string{{`{"n":1}`,`{"n":1.0}`},{`[1,{"x":[2.0]}]`,`[1.0,{"x":[2]}]`},{`{"a":1,"b":2}`,`{"b":2.0,"a":1e0}`}}
 for _,pair:=range cases{a,ae:=operatorJSON([]byte(pair[0]),OperatorRequestBytes);b,be:=operatorJSON([]byte(pair[1]),OperatorRequestBytes);if ae!=nil||be!=nil||!operatorPayloadEqual(a,b){t.Fatal("equivalent nested numeric enum spellings differ")}}
 if operatorPayloadEqual([]any{json.Number("1")},[]any{json.Number("2")})||operatorPayloadEqual(map[string]any{"a":nil},map[string]any{"b":nil}){t.Fatal("structural equality lost positional or property identity")}
}
func TestOperatorCompleteActionFormWithCompactSchema(t *testing.T){
 revision:="revision";available:=OperatorAvailability{Supported:true,Available:true}
 table:=&OperatorPayloadTable{Root:0,Nodes:[]OperatorPayloadNode{{ID:0,Types:[]string{"object"},Constraints:OperatorPayloadConstraintMap{"additionalProperties":json.RawMessage("false")},Properties:[]OperatorPayloadEdge{{Name:"name",Node:1}},Required:[]string{"name"}},{ID:1,Types:[]string{"string","null"},Constraints:OperatorPayloadConstraintMap{},Properties:[]OperatorPayloadEdge{},Required:[]string{}}}}
 action:=OperatorAction{ActionID:"run",Description:"Run",DefinitionRevision:revision,ExpectedRevision:&revision,PayloadPolicy:OperatorPayloadPolicy{Inline:true,Required:true},InputFields:table,PayloadReferences:[]OperatorPayloadReference{},RequiredProfile:"operator",RequiredScopes:[]string{},DeclaredPermission:"run",Availability:available}
 envelope:=OperatorEnvelope[OperatorActionForm]{ContractVersion:OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:OperatorActionForm{ServiceID:"service",DefinitionRevision:revision,Actions:[]OperatorAction{action},Availability:available}}
 raw,err:=json.Marshal(envelope);if err!=nil{t.Fatal(err)};var decoded OperatorEnvelope[OperatorActionForm]
 if decodeOperator(raw,&decoded,OperatorResponseBytes)!=nil||!operatorActionValid(decoded.Data.Actions[0]){t.Fatal("genuine full action form with compact constraints rejected")}
 table.Nodes[1].Constraints["minLength"]=json.RawMessage("null");raw,_=json.Marshal(envelope);if decodeOperator(raw,&decoded,OperatorResponseBytes)==nil{t.Fatal("null compact bound admitted")}
 delete(table.Nodes[1].Constraints,"minLength");table.Nodes[1].Constraints["privatePath"]=json.RawMessage(`"secret"`);raw,_=json.Marshal(envelope);if decodeOperator(raw,&decoded,OperatorResponseBytes)==nil{t.Fatal("unknown compact constraint admitted")}
 delete(table.Nodes[1].Constraints,"privatePath");table.Nodes[0].Properties[0].Node=0;if _,ok:=table.Tree();ok{t.Fatal("cyclic table admitted")}
}
func TestOperatorPayloadSelectorsRetainLogicalPathAfterOverflow(t *testing.T){
 schema:=OperatorPayloadSchema{Types:[]string{"object"},Properties:[]OperatorPayloadProperty{},Required:[]string{}}
 child:=OperatorPayloadSchema{Types:[]string{"string"},Properties:[]OperatorPayloadProperty{},Required:[]string{}}
 name:="a/b~c";schema.Properties=append(schema.Properties,OperatorPayloadProperty{Name:name,Schema:child})
 minimum,maximum:=0,4096;schema.Properties[0].Schema.Constraints.MinLength=&minimum;schema.Properties[0].Schema.Constraints.MaxLength=&maximum;closed:=false;schema.Constraints.AdditionalProperties=&closed
 selectors:=operatorPayloadSelectors(&schema,json.RawMessage(`{"a/b~c":"value"}`));found:=false;for _,selector:=range selectors{if selector=="/input/payload/a~1b~0c"{found=true}};if !found{t.Fatal("original logical name escaping lost")}
 top:=strings.Repeat("a",100);middle:=strings.Repeat("b",20);leaf:=schema.Properties[0].Schema
 nested:=OperatorPayloadSchema{Types:[]string{"object"},Constraints:OperatorPayloadConstraints{AdditionalProperties:&closed},Properties:[]OperatorPayloadProperty{{Name:"tail",Schema:leaf}},Required:[]string{}}
 branch:=OperatorPayloadSchema{Types:[]string{"object"},Constraints:OperatorPayloadConstraints{AdditionalProperties:&closed},Properties:[]OperatorPayloadProperty{{Name:middle,Schema:nested}},Required:[]string{}}
 schema.Properties=[]OperatorPayloadProperty{{Name:top,Schema:branch}};raw,_:=json.Marshal(map[string]any{top:map[string]any{middle:map[string]any{"tail":"value"}}})
 selectors=operatorPayloadSelectors(&schema,raw);for _,selector:=range selectors{if len(selector)>128||selector=="/input/payload/"+top+"/tail"{t.Fatal("overflow fabricated a shortened logical path")}}
}
func TestOperatorPayloadLiteralScalarControlsRemainData(t *testing.T){
 minimum,maximum:=0,4096;schema:=OperatorPayloadSchema{Types:[]string{"string"},Constraints:OperatorPayloadConstraints{MinLength:&minimum,MaxLength:&maximum},Properties:[]OperatorPayloadProperty{},Required:[]string{}}
 original:="line\r\n\t\x00\x07\x1b\u0085\u2028\u2029界";raw,_:=json.Marshal(original)
 if !schema.ValueValid(raw){t.Fatal("authentic literal Unicode scalar payload was narrowed to display grammar")}
 schema.Constraints.Enum=[]json.RawMessage{raw};if !schema.ValueValid(raw){t.Fatal("literal control enum changed value semantics")}
 if operatorDisplay(original,4096){t.Fatal("payload exception widened ordinary display or identifier grammar")}
}
