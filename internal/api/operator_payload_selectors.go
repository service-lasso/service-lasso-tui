package api

import (
 "context"
 "encoding/json"
 "strconv"
)

func operatorPayloadSelectors(schema *OperatorPayloadSchema,raw json.RawMessage)[]string{
 return operatorPayloadRequestSelectors(schema,raw,false)
}
func operatorPayloadRequestSelectors(schema *OperatorPayloadSchema,raw json.RawMessage,overlay bool)[]string{
 value,err:=operatorJSON(raw,OperatorRequestBytes);valid:=schema.ValueValid(raw);if overlay{valid=schema.OverlayValid(raw)};if err!=nil||!valid{return nil};seen:=map[string]bool{};selectors:=[]string{}
 add:=func(path string){if !seen[path]{seen[path]=true;selectors=append(selectors,path)}}
 var walk func(*OperatorPayloadSchema,any,string,string)
 walk=func(current *OperatorPayloadSchema,value any,full,approved string){
  if len(full)<=128{approved=full};add(approved)
  if object,ok:=value.(map[string]any);ok&&current.Has("object"){for i:=range current.Properties{property:=&current.Properties[i];child,exists:=object[property.Name];if overlay&&!exists{continue};path:=operatorPayloadPointer(full,property.Name);nearest:=approved;if len(path)<=128{nearest=path};add(nearest);if exists{walk(&property.Schema,child,path,nearest)}}}
  if array,ok:=value.([]any);ok&&current.Has("array")&&current.Items!=nil{for index,child:=range array{path:=full+"/"+strconv.Itoa(index);walk(current.Items,child,path,approved)}}
 }
 walk(schema,value,"/input/payload","/input/payload");return selectors
}
type operatorDeclaredPreviewBody struct{
 ActionID string `json:"actionId"`;DefinitionRevision string `json:"definitionRevision"`;ExpectedRevision string `json:"expectedRevision"`;Input json.RawMessage `json:"input"`
 Selectors []string `json:"-"`
}
func(b operatorDeclaredPreviewBody)operatorErrorSelectors()[]string{return b.Selectors}
func(c *Client)OperatorDeclaredPreview(ctx context.Context,request OperatorPreviewRequest,form OperatorActionForm)(OperatorPreview,error){
 invalid:=func()(OperatorPreview,error){return OperatorPreview{},&OperatorPublicError{Code:"invalid_request"}}
 if request.ActionID!="declared_action_run"||request.ServiceID!=form.ServiceID||request.DefinitionRevision!=form.DefinitionRevision||!OperatorServiceID(request.ServiceID){return invalid()}
 var input OperatorDeclaredInput;if err:=decodeOperator(request.Input,&input,OperatorRequestBytes);err!=nil{return OperatorPreview{},OperatorInvalidRequestCause(err)};inputOK,inputErr:=OperatorDeclaredInputValidCause(form,input);if inputErr!=nil{return OperatorPreview{},OperatorInvalidRequestCause(inputErr)};if !inputOK{return invalid()}
 selectors:=[]string{"actionId","definitionRevision","expectedRevision","input","declaredActionId","payload","payloadRef"};matched:=false
 for _,action:=range form.Actions{if action.ActionID!=input.DeclaredActionID{continue};if action.DefinitionRevision!=request.DefinitionRevision||action.ExpectedRevision==nil||*action.ExpectedRevision!=request.ExpectedRevision{return invalid()};matched=true;if input.PayloadRef!=nil{selectors=append(selectors,"/input/payloadRef")};if len(input.Payload)>0{if action.InputFields==nil{return invalid()};schema,ok,schemaErr:=action.InputFields.TreeCause();if schemaErr!=nil{return OperatorPreview{},OperatorInvalidRequestCause(schemaErr)};if !ok{return invalid()};selectors=append(selectors,operatorPayloadRequestSelectors(schema,input.Payload,input.PayloadRef!=nil)...)};break};if !matched{return invalid()}
 body:=operatorDeclaredPreviewBody{request.ActionID,request.DefinitionRevision,request.ExpectedRevision,request.Input,selectors}
 return operatorRequest(ctx,c,"POST","/services/"+request.ServiceID+"/preview",body,func(p OperatorPreview)bool{return operatorPreviewValid(p)&&p.ServiceID==request.ServiceID&&p.ActionID==request.ActionID&&p.DefinitionRevision==request.DefinitionRevision&&p.ExpectedRevision==request.ExpectedRevision},200)
}
