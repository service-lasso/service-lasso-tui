package api

import (
 "encoding/json"
 "errors"
 "strconv"
 "testing"
)

// Source-only cause and useful-positive cases remain UNRUN.
func payloadCauseForm()OperatorActionForm{
 revision:="revision";available:=OperatorAvailability{Supported:true,Available:true}
 table:=&OperatorPayloadTable{Root:0,Nodes:[]OperatorPayloadNode{{ID:0,Types:[]string{"object"},Constraints:OperatorPayloadConstraintMap{"additionalProperties":json.RawMessage("false")},Properties:[]OperatorPayloadEdge{{Name:"name",Node:1}},Required:[]string{"name"}},{ID:1,Types:[]string{"string","null"},Constraints:OperatorPayloadConstraintMap{},Properties:[]OperatorPayloadEdge{},Required:[]string{}}}}
 action:=OperatorAction{ActionID:"run",Description:"Run",DefinitionRevision:revision,ExpectedRevision:&revision,PayloadPolicy:OperatorPayloadPolicy{Inline:true,Required:true},InputFields:table,PayloadReferences:[]OperatorPayloadReference{},RequiredProfile:"operator",RequiredScopes:[]string{},DeclaredPermission:"run",Availability:available}
 return OperatorActionForm{ServiceID:"service",DefinitionRevision:revision,Actions:[]OperatorAction{action},Availability:available}
}

func TestDeclaredSchemaCauseAndUsefulNullUnion(t *testing.T){
 form:=payloadCauseForm();input:=OperatorDeclaredInput{DeclaredActionID:"run",Payload:json.RawMessage(`{"name":null}`)}
 ok,err:=OperatorDeclaredInputValidCause(form,input);if !ok||err!=nil{t.Fatal("useful null union lost")}
 form.Actions[0].InputFields.Nodes[1].Constraints["minLength"]=json.RawMessage("1e999")
 root,ok,err:=form.Actions[0].InputFields.TreeCause();var number *strconv.NumError
 if root!=nil||ok||!errors.As(err,&number){t.Fatal("compact constraint original or zero tree failure lost")}
 ok,err=OperatorDeclaredInputValidCause(form,input);if ok||!errors.As(err,&number){t.Fatal("form refusal erased actual table cause")}
 form.Actions[0].InputFields.Nodes[1].Constraints["minLength"]=json.RawMessage("null")
 root,ok,err=form.Actions[0].InputFields.TreeCause();if root!=nil||ok||err!=nil{t.Fatal("semantic constraint null refusal invented a cause")}
}

func TestSchemaRootEnumDecodeAndNestedNumericOriginals(t *testing.T){
 form:=payloadCauseForm();root,ok,err:=form.Actions[0].InputFields.TreeCause();if !ok||err!=nil{t.Fatal("fixture must start as genuine complete root")}
 root.Constraints.Enum=[]json.RawMessage{json.RawMessage(`{"name":@}`)}
 ok,err=root.ValidRootCause();var syntax *json.SyntaxError;if ok||!errors.As(err,&syntax){t.Fatal("root enum decoder original lost")}
 equal,err:=operatorPayloadEqualCause([]any{json.Number("1e999")},[]any{json.Number("2e999")})
 if equal||err==nil{t.Fatal("numeric equivalence failures lost")}
 joined,ok:=err.(interface{Unwrap()[]error});if !ok||len(joined.Unwrap())!=2{t.Fatal("both numeric originals must remain reachable")}
 for index,original:=range joined.Unwrap(){var number *strconv.NumError;if !errors.As(original,&number)||number.Num!=[]string{"1e999","2e999"}[index]{t.Fatal("actual numeric spelling original replaced")}}
 equal,err=operatorPayloadEqualCause(map[string]any{"name":json.Number("1.0")},map[string]any{"name":json.Number("1e0")});if !equal||err!=nil{t.Fatal("useful structural numeric equivalence lost")}
}
