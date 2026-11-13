package api

import (
	"bytes"
	"context"
	"crypto/sha256"
	"encoding/hex"
	"encoding/json"
	"errors"
	"io"
	"net/http"
	"net/url"
	"strconv"
)

type OperatorPublicError struct { Code string; RequestID string; FieldErrors []OperatorFieldError; response *operatorResponseObservation }
func (e *OperatorPublicError) Error() string { return OperatorMessage(e.Code) }
func OperatorMessage(code string) string {
	switch code {
	case "authentication_required":return "Authentication required. Check the selected connection."
	case "permission_denied":return "Permission denied. Check the current operator entitlement."
	case "binding_stale":return "The preview is no longer current. Refresh and create a new preview; the draft is retained."
	case "revision_conflict","source_replaced":return "The source changed. Refresh and create a new preview; the draft is retained."
	case "confirmation_required","confirmation_expired":return "A fresh confirmation is required."
	case "snapshot_expired":return "The snapshot expired. Refresh to start a new snapshot."
	case "operation_unknown":return "Operation outcome unknown. Retain the exact ID; no action was replayed."
	case "storage_unavailable":return "Recovery storage unavailable. Live tracking continues; restart restoration is unavailable."
	case "ownership_unknown":return "Ownership could not be proved. Core retains the original recovery state."
	case "dependency_blocked":return "A required dependency is blocked."
	case "unsupported_capability","unsupported_version","unsupported_selector":return "This runtime does not support the requested operator contract."
	case "validation_failed","invalid_request":return "The submitted fields are invalid. Review the typed form."
	case "source_corrupt":return "Source integrity failed. Original private evidence remains retained."
	default:return "Operator data is unavailable. Retry reads or inspect authoritative recovery."
	}
}
type OperatorFieldError struct { Selector string `json:"selector"`; Code string `json:"code"` }
type operatorFieldError = OperatorFieldError
type operatorErrorBody struct { Code string `json:"code"`; MessageKey string `json:"messageKey"`; Retryability string `json:"retryability"`; FieldErrors []operatorFieldError `json:"fieldErrors"` }
type operatorErrorEnvelope struct { ContractVersion string `json:"contractVersion"`; RequestID string `json:"requestId"`; Error operatorErrorBody `json:"error"` }
var operatorErrorCodes=[]string{"invalid_request","invalid_utf8","duplicate_key","limit_exceeded","unsupported_version","unsupported_capability","unsupported_selector","validation_failed","authentication_required","permission_denied","confirmation_required","confirmation_expired","binding_stale","revision_conflict","operation_conflict","operation_unknown","source_unavailable","source_corrupt","snapshot_expired","source_replaced","storage_unavailable","dependency_blocked","ownership_unknown","not_found","internal_error"}

func operatorResponseError(raw []byte,status int,method,path string,body any)error{
		var envelope operatorErrorEnvelope
		decodeErr:=decodeOperator(raw,&envelope,OperatorErrorBytes)
		if decodeErr!=nil||envelope.ContractVersion!=OperatorContract||!OperatorID(envelope.RequestID)||!operatorEnum(envelope.Error.Code,operatorErrorCodes...)||!operatorErrorPolicyValid(envelope.Error,status)||envelope.Error.FieldErrors==nil||len(envelope.Error.FieldErrors)>32{fallback:=safeHTTPError(method,"/api/operator/v2",status).(*OperatorError);var decoded *OperatorError;if errors.As(decodeErr,&decoded)&&decoded.Unwrap()!=nil{fallback.cause=decodeErr};return fallback}
		for _,field:=range envelope.Error.FieldErrors{if !operatorErrorSelectorAllowed(field.Selector,path,body)||!operatorEnum(field.Code,operatorErrorCodes...){return &OperatorError{Stage:"response"}}}
		return &OperatorPublicError{Code:envelope.Error.Code,RequestID:envelope.RequestID,FieldErrors:append([]OperatorFieldError(nil),envelope.Error.FieldErrors...)}
}
// Private bounded observations are not complete-body, durable or native ACK proof.
type operatorResponseObservation struct{Response *http.Response;Body io.ReadCloser;Raw []byte;Status,Limit int;ReadAttempted,CloseAttempted bool;ReadError,CloseError error}
func operatorTransportFailure(stage string,response *http.Response,cause error)error{
 failure:=&OperatorError{Stage:stage,cause:cause}
 // http.Client may return a response whose body it already closed. Retain the
 // actual object, never manufacture another read/Close observation here.
 if response!=nil{failure.response=&operatorResponseObservation{Response:response,Body:response.Body,Status:response.StatusCode}}
 return failure
}
func operatorReadResponse(response *http.Response,limit int)([]byte,error,*operatorResponseObservation){
 observation:=&operatorResponseObservation{Response:response,Body:response.Body,Status:response.StatusCode,Limit:limit}
 return finishObservedResponse(observation)
}
func finishObservedResponse(observation *operatorResponseObservation)([]byte,error,*operatorResponseObservation){
 response:=observation.Response;limit:=observation.Limit
 observation.ReadAttempted=true
 raw,readErr:=io.ReadAll(io.LimitReader(response.Body,int64(limit+1)))
 observation.Raw=raw;observation.ReadError=readErr
 observation.CloseAttempted=true
 closeErr:=response.Body.Close()
 observation.CloseError=closeErr
 return raw,errors.Join(readErr,closeErr),observation
}
func retainOperatorResponse(err error,observation *operatorResponseObservation)error{
 if err==nil||observation==nil{return err};switch failure:=err.(type){case *OperatorError:failure.response=observation;case *OperatorPublicError:failure.response=observation};return err
}
func operatorRequest[T any](ctx context.Context,c *Client,method,path string,body any,valid func(T)bool,statuses ...int)(result T,resultErr error){
	var zero T
 occurrence,enrollmentErr:=c.beginRequest(ctx,method,path,body);if enrollmentErr!=nil{return zero,enrollmentErr};defer func(){occurrence.Result=result;occurrence.Failure=resultErr}()
	var source io.Reader
	if body!=nil { raw,err:=json.Marshal(body);occurrence.Encoded=raw;if err!=nil{return zero,&OperatorError{Stage:"invalid_request",cause:err}};if len(raw)>OperatorRequestBytes{return zero,&OperatorPublicError{Code:"invalid_request"}};source=bytes.NewReader(raw);occurrence.Source=source }
	req,err:=http.NewRequestWithContext(ctx,method,c.baseURL+"/api/operator/v2"+path,source);occurrence.Request=req;if err!=nil{return zero,&OperatorError{Stage:"configuration",cause:err}}
	if source!=nil{req.Header.Set("Content-Type","application/json")}
	if c.operatorToken!=""{if c.authMode==AuthModeLocalAdmin{req.Header.Set("x-service-lasso-admin-token",c.operatorToken)}else if c.authMode==AuthModeOAuthBearer{req.Header.Set("Authorization","Bearer "+c.operatorToken)}}
	response,err:=c.http.Do(req);occurrence.Response=response;if err!=nil{return zero,operatorTransportFailure("transport",response,err)}
	limit:=OperatorResponseBytes;if response.StatusCode<200||response.StatusCode>=300{limit=OperatorErrorBytes}
	occurrence.Observation=&operatorResponseObservation{};raw,err,observation:=readObservedResponse(occurrence.Observation,response,limit);defer func(){resultErr=retainOperatorResponse(resultErr,observation)}();if err!=nil{return zero,&OperatorError{Stage:"response",cause:err}};if len(raw)>limit{return zero,&OperatorError{Stage:"response"}}
	if response.StatusCode<200||response.StatusCode>=300 {
		return zero,operatorResponseError(raw,response.StatusCode,method,path,body)
	}
	accepted:=false;for _,status:=range statuses{if response.StatusCode==status{accepted=true}};if !accepted{return zero,&OperatorError{Stage:"response"}}
	var envelope OperatorEnvelope[T];occurrence.Destination=&envelope
	if decodeErr:=decodeOperator(raw,&envelope,OperatorResponseBytes);decodeErr!=nil{return zero,decodeErr};if envelope.ContractVersion!=OperatorContract||!OperatorID(envelope.RequestID)||!OperatorTime(envelope.GeneratedAt)||!valid(envelope.Data){return zero,&OperatorError{Stage:"response"}}
	return envelope.Data,nil
}
func operatorServicePath(id string)(string,error){if !OperatorServiceID(id){return "",&OperatorPublicError{Code:"invalid_request"}};return "/services/"+url.PathEscape(id),nil}
func operatorPaging(cursor *string,limit int,filterKey,filter string)string{q:=url.Values{};q.Set("limit",strconv.Itoa(limit));if cursor!=nil{q.Set("cursor",*cursor)};if filterKey!=""{q.Set(filterKey,filter)};return "?"+q.Encode()}
func (c *Client) OperatorDiscovery(ctx context.Context)(OperatorDiscovery,error){return operatorRequest(ctx,c,"GET","/discovery",nil,func(d OperatorDiscovery)bool{return d.Valid()},200)}
func (c *Client) OperatorServices(ctx context.Context,cursor *string,filter string)(OperatorPage[OperatorService],error){
	if !operatorEnum(filter,"all","installed","running","unavailable"){return OperatorPage[OperatorService]{},&OperatorPublicError{Code:"invalid_request"}}
	return operatorRequest(ctx,c,"GET","/services"+operatorPaging(cursor,25,"filter",filter),nil,func(p OperatorPage[OperatorService])bool{if !p.ValidHeader(25){return false};seen:=map[string]bool{};for _,s:=range p.Rows{if !s.Valid()||seen[s.ServiceID]{return false};seen[s.ServiceID]=true};return true},200)
}
func (c *Client) OperatorDetail(ctx context.Context,id string)(OperatorDetail,error){path,err:=operatorServicePath(id);if err!=nil{return OperatorDetail{},err};return operatorRequest(ctx,c,"GET",path,nil,func(d OperatorDetail)bool{return d.Valid()&&d.Summary.ServiceID==id},200)}
func (c *Client) OperatorLifecycleAvailability(ctx context.Context,id string)(OperatorLifecycleAvailability,error){path,err:=operatorServicePath(id);if err!=nil{return OperatorLifecycleAvailability{},err};return operatorRequest(ctx,c,"GET",path+"/lifecycle-availability",nil,func(a OperatorLifecycleAvailability)bool{return a.ServiceID==id&&a.Valid()},200)}
func (c *Client) OperatorMeta(ctx context.Context,id string)(OperatorMeta,error){path,err:=operatorServicePath(id);if err!=nil{return OperatorMeta{},err};return operatorRequest(ctx,c,"GET",path+"/meta",nil,func(m OperatorMeta)bool{return m.Valid()&&m.ServiceID==id},200)}
func (c *Client) OperatorPatchMeta(ctx context.Context,id,revision string,favorite *bool,position **OperatorPosition)(OperatorMeta,error){
	path,err:=operatorServicePath(id);if err!=nil{return OperatorMeta{},err};if !OperatorID(revision)||(favorite==nil&&position==nil){return OperatorMeta{},&OperatorPublicError{Code:"invalid_request"}}
	body:=map[string]any{"expectedRevision":revision};if favorite!=nil{body["favorite"]=*favorite};if position!=nil{if *position!=nil&&((*position).X < -100000||(*position).X>100000||(*position).Y < -100000||(*position).Y>100000){return OperatorMeta{},&OperatorPublicError{Code:"invalid_request"}};body["position"]=*position}
	return operatorRequest(ctx,c,"PATCH",path+"/meta",body,func(m OperatorMeta)bool{return m.Valid()&&m.ServiceID==id&&m.Availability.Available},200)
}
func (c *Client) OperatorGraph(ctx context.Context,cursor *string)(OperatorPage[OperatorGraphGroup],error){return operatorRequest(ctx,c,"GET","/graph"+operatorPaging(cursor,10,"",""),nil,func(p OperatorPage[OperatorGraphGroup])bool{if !p.ValidHeader(10)||p.Truncated{return false};for _,group:=range p.Rows{if !operatorGraphGroupValid(group){return false}};return true},200)}
func (c *Client) OperatorConfigForm(ctx context.Context,id string)(OperatorConfigForm,error){path,err:=operatorServicePath(id);if err!=nil{return OperatorConfigForm{},err};return operatorRequest(ctx,c,"GET",path+"/config-form",nil,func(f OperatorConfigForm)bool{return f.ServiceID==id&&f.Valid()},200)}
func (c *Client) OperatorSetupForm(ctx context.Context,id string)(OperatorSetupForm,error){path,err:=operatorServicePath(id);if err!=nil{return OperatorSetupForm{},err};return operatorRequest(ctx,c,"GET",path+"/setup-form",nil,func(f OperatorSetupForm)bool{if f.ServiceID!=id||!OperatorID(f.DefinitionRevision)||!f.Availability.Valid()||f.Steps==nil||len(f.Steps)>128||!f.Availability.Available&&len(f.Steps)!=0{return false};seen:=map[string]bool{};for _,s:=range f.Steps{if !operatorSetupValid(s)||seen[s.StepID]{return false};seen[s.StepID]=true};return true},200)}
func (c *Client) OperatorActionForm(ctx context.Context,id string)(OperatorActionForm,error){path,err:=operatorServicePath(id);if err!=nil{return OperatorActionForm{},err};return operatorRequest(ctx,c,"GET",path+"/action-form",nil,func(f OperatorActionForm)bool{return f.ServiceID==id&&f.Valid()},200)}
func (c *Client) OperatorUpdateForm(ctx context.Context,id string)(OperatorUpdateForm,error){path,err:=operatorServicePath(id);if err!=nil{return OperatorUpdateForm{},err};return operatorRequest(ctx,c,"GET",path+"/update-form",nil,func(f OperatorUpdateForm)bool{return f.ServiceID==id&&OperatorID(f.DefinitionRevision)&&operatorEnum(f.Mode,"disabled","notify","download","install")&&operatorEnum(f.RunningService,"skip","require-stopped","stop-start","restart")&&(f.CandidateID==nil||OperatorID(*f.CandidateID))&&operatorEnum(f.CheckStatus,"pinned","latest","update_available","unavailable","check_failed","not_checked","unknown")&&operatorRevisionGrant(f.ExpectedRevision,f.Availability)},200)}
func (c *Client) OperatorRemovalForm(ctx context.Context,id string)(OperatorRemovalForm,error){path,err:=operatorServicePath(id);if err!=nil{return OperatorRemovalForm{},err};return operatorRequest(ctx,c,"GET",path+"/removal-form",nil,func(f OperatorRemovalForm)bool{if f.ServiceID!=id||!OperatorID(f.DefinitionRevision)||!OperatorID(f.Generation)||!operatorRevisionGrant(f.ExpectedRevision,f.Availability)||len(f.Categories)!=6{return false};seen:=map[string]bool{};for _,cat:=range f.Categories{if seen[cat.Code]||!operatorEnum(cat.Code,"runtime_artifacts","config","userdata","backups","logs","private_recovery")||!operatorEnum(cat.Disposition,"retain","eligible_remove","unknown")||(cat.Code!="runtime_artifacts"&&cat.Disposition=="eligible_remove"){return false};seen[cat.Code]=true};return true},200)}

// The original canonical-v1 digest is retained byte-for-byte for saved2.
func (c OperatorContext) LegacyDigest()string{value:=struct{ContractVersion string `json:"contractVersion"`;Context OperatorContext `json:"context"`}{"service-lasso-durable-reconciliation-context.v1",c};raw,err:=json.Marshal(value);if err!=nil||!c.Valid(){return ""};digest:=sha256.Sum256(raw);return "slrc1_"+hex.EncodeToString(digest[:])}
func (c *Client) OperatorContext(ctx context.Context)(result OperatorContext,resultErr error){
	// This route deliberately keeps the existing context envelope/version.
 occurrence,enrollmentErr:=c.beginRequest(ctx,"GET","/reconciliation-context",nil);if enrollmentErr!=nil{return result,enrollmentErr};defer func(){occurrence.Result=result;occurrence.Failure=resultErr}()
	var value struct{ContractVersion string `json:"contractVersion"`;Context OperatorContext `json:"context"`}
	req,err:=http.NewRequestWithContext(ctx,"GET",c.baseURL+"/api/operator/v2/reconciliation-context",nil);occurrence.Request=req;if err!=nil{return OperatorContext{},&OperatorError{Stage:"configuration",cause:err}}
	if c.authMode==AuthModeLocalAdmin{req.Header.Set("x-service-lasso-admin-token",c.operatorToken)}else{req.Header.Set("Authorization","Bearer "+c.operatorToken)}
	response,err:=c.http.Do(req);occurrence.Response=response;if err!=nil{return OperatorContext{},operatorTransportFailure("transport",response,err)}
	limit:=OperatorContextBytes;if response.StatusCode!=200{limit=OperatorErrorBytes};occurrence.Observation=&operatorResponseObservation{};raw,readErr,observation:=readObservedResponse(occurrence.Observation,response,limit);defer func(){resultErr=retainOperatorResponse(resultErr,observation)}()
	if response.StatusCode!=200{if readErr!=nil{return OperatorContext{},&OperatorError{Stage:"response",cause:readErr}};if len(raw)>OperatorErrorBytes{return OperatorContext{},&OperatorError{Stage:"response"}};return OperatorContext{},operatorResponseError(raw,response.StatusCode,"GET","/reconciliation-context",nil)}
	if readErr!=nil{return OperatorContext{},&OperatorError{Stage:"context",cause:readErr}};if decodeErr:=decodeOperator(raw,&value,OperatorContextBytes);decodeErr!=nil{return OperatorContext{},&OperatorError{Stage:"context",cause:decodeErr}};if value.ContractVersion!="service-lasso-durable-reconciliation-context.v1"||!value.Context.Valid(){return OperatorContext{},&OperatorError{Stage:"context"}};return value.Context,nil
}
