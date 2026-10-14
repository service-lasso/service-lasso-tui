package api

import (
	"context"
	"encoding/json"
	"net/url"
	"regexp"
)

type OperatorPreviewRequest struct {
	ServiceID string `json:"-"`
	ActionID string `json:"actionId"`
	DefinitionRevision string `json:"definitionRevision"`
	ExpectedRevision string `json:"expectedRevision"`
	Input json.RawMessage `json:"input"`
}
type OperatorExecuteRequest struct {
	PlanID string `json:"planId"`
	IdempotencyKey string `json:"idempotencyKey"`
	ConfirmationID *string `json:"confirmationId"`
	ConfirmationResponse *string `json:"confirmationResponse"`
	ExpectedRevision string `json:"expectedRevision"`
}
type OperatorLifecycleInput struct{}
type OperatorSetupInput struct { StepID string `json:"stepId"`; Values map[string]json.RawMessage `json:"values"`; IncludeManual bool `json:"includeManual"`; Force bool `json:"force"` }
type OperatorDeclaredInput struct { DeclaredActionID string `json:"declaredActionId"`; Payload json.RawMessage `json:"payload,omitempty"`; PayloadRef *string `json:"payloadRef,omitempty"` }
type OperatorUpdateInput struct { CandidateID string `json:"candidateId"` }
type OperatorRemovalInput struct { RetainData bool `json:"retainData"` }
var operatorIdempotency = regexp.MustCompile(`^[A-Za-z0-9_-]{16,128}$`)

func (c *Client) OperatorPreview(ctx context.Context, request OperatorPreviewRequest) (OperatorPreview,error) {
	if !OperatorServiceID(request.ServiceID)||!OperatorID(request.ActionID)||!OperatorID(request.DefinitionRevision)||!OperatorID(request.ExpectedRevision){return OperatorPreview{},&OperatorPublicError{Code:"invalid_request"}}
	admitted,err:=operatorPreviewInputCause(request.ActionID,request.Input);if err!=nil{return OperatorPreview{},OperatorInvalidRequestCause(err)};if !admitted{return OperatorPreview{},&OperatorPublicError{Code:"invalid_request"}}
	body:=struct{ActionID string `json:"actionId"`;DefinitionRevision string `json:"definitionRevision"`;ExpectedRevision string `json:"expectedRevision"`;Input json.RawMessage `json:"input"`}{request.ActionID,request.DefinitionRevision,request.ExpectedRevision,request.Input}
	return operatorRequest(ctx,c,"POST","/services/"+request.ServiceID+"/preview",body,func(p OperatorPreview)bool{return operatorPreviewValid(p)&&p.ServiceID==request.ServiceID&&p.ActionID==request.ActionID&&p.DefinitionRevision==request.DefinitionRevision&&p.ExpectedRevision==request.ExpectedRevision},200)
}
func operatorPreviewInput(action string, raw []byte) bool {
	admitted,_:=operatorPreviewInputCause(action,raw);return admitted
}
func operatorPreviewInputCause(action string,raw []byte)(bool,error){
	switch action {
	case "service_start","service_stop","service_restart","service_install","service_configure","update_check","update_download":
		var input OperatorLifecycleInput;err:=decodeOperator(raw,&input,OperatorRequestBytes);return err==nil,err
	case "setup_step_run":
		// Values are checked against the fetched declaration before request creation.
		var input struct{StepID string `json:"stepId"`; Values json.RawMessage `json:"values"`; IncludeManual bool `json:"includeManual"`; Force bool `json:"force"`}
		if err:=decodeOperator(raw,&input,OperatorRequestBytes);err!=nil{return false,err};if !OperatorID(input.StepID){return false,nil};value,err:=operatorJSON(input.Values,OperatorRequestBytes);object,ok:=value.(map[string]any);return err==nil&&ok&&len(object)<=64,err
	case "declared_action_run":
		var input OperatorDeclaredInput;if err:=decodeOperator(raw,&input,OperatorRequestBytes);err!=nil{return false,err};return OperatorID(input.DeclaredActionID)&&(input.PayloadRef==nil||OperatorID(*input.PayloadRef)),nil
	case "update_install":
		var input OperatorUpdateInput;if err:=decodeOperator(raw,&input,OperatorRequestBytes);err!=nil{return false,err};return OperatorID(input.CandidateID),nil
	case "service_remove_retain_data":
		var input OperatorRemovalInput;if err:=decodeOperator(raw,&input,OperatorRequestBytes);err!=nil{return false,err};return input.RetainData,nil
	};return false,nil
}
func operatorResultValid(result OperatorResult,operation OperatorOperation)bool {
	if result.OperationID!=operation.OperationID||result.ActionID!=operation.ActionID||result.ServiceID!=operation.ServiceID||operation.Outcome==nil||!operatorServiceIDs(result.Targets,256)||!operatorUniqueEnum(result.Effects,128,OperatorEffects)||!operatorDisplay(result.Summary,512)||result.ResultingState==nil||len(result.ResultingState)>256{return false}
	seen:=map[string]bool{};for _,state:=range result.ResultingState{if !OperatorServiceID(state.ServiceID)||seen[state.ServiceID]{return false};seen[state.ServiceID]=true};return true
}
func operatorExecuteRequestValid(request OperatorExecuteRequest,preview OperatorPreview)error{
	if !operatorPreviewValid(preview)||!preview.Availability.Available||request.PlanID!=preview.PlanID||request.ExpectedRevision!=preview.ExpectedRevision||!operatorIdempotency.MatchString(request.IdempotencyKey){return &OperatorPublicError{Code:"invalid_request"}}
	if preview.Confirmation.Required {if request.ConfirmationID==nil||*request.ConfirmationID!=*preview.Confirmation.ID||request.ConfirmationResponse==nil||!OperatorConfirmationResponseValid(preview,*request.ConfirmationResponse){return &OperatorPublicError{Code:"confirmation_required"}}}else if request.ConfirmationID!=nil||request.ConfirmationResponse!=nil{return &OperatorPublicError{Code:"invalid_request"}};return nil
}
func (c *Client) operatorExecute(ctx context.Context,request OperatorExecuteRequest,preview OperatorPreview)(OperatorSubmission,error) {
	if err:=operatorExecuteRequestValid(request,preview);err!=nil{return OperatorSubmission{},err}
	return operatorRequest(ctx,c,"POST","/services/"+preview.ServiceID+"/execute",request,func(s OperatorSubmission)bool{
		if !s.Operation.Valid()||s.Operation.ServiceID!=preview.ServiceID||s.Operation.ActionID!=preview.ActionID{return false}
		if s.Kind=="accepted"{return s.Result==nil};return s.Kind=="completed"&&s.Operation.Outcome!=nil&&s.Result!=nil&&operatorResultValid(*s.Result,s.Operation)
	},200,202)
}
func (c *Client) OperatorOperation(ctx context.Context,id string)(OperatorReadback,error) {
	if !OperatorID(id){return OperatorReadback{},&OperatorPublicError{Code:"invalid_request"}}
	return operatorRequest(ctx,c,"GET","/operations/"+url.PathEscape(id),nil,func(r OperatorReadback)bool{return r.Operation.Valid()&&r.Operation.OperationID==id&&(r.Result==nil||r.Operation.Outcome!=nil&&operatorResultValid(*r.Result,r.Operation))},200)
}
func (c *Client) OperatorCancel(ctx context.Context,operation OperatorOperation)(OperatorCancel,error) {
	if !operation.Valid()||!operation.CancellationSupported||operation.Outcome!=nil{return OperatorCancel{},&OperatorPublicError{Code:"unsupported_capability"}}
	request:=struct{}{}
	return operatorRequest(ctx,c,"POST","/operations/"+url.PathEscape(operation.OperationID)+"/cancel",request,func(r OperatorCancel)bool{return r.OperationID==operation.OperationID&&r.ActionID==operation.ActionID&&operatorEnum(r.Status,"requested","unsupported","too_late")},200)
}
