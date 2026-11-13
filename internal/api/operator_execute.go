package api

import (
 "context"
 "time"
)

type OperatorExecutionSource struct{
 Input []byte
 ConfigOperations []OperatorConfigOperation
 QueueRow *OperatorQueueRow
}
type OperatorSubmissionFailure struct{Dispatched bool;Cause error `json:"-"`}
func(e *OperatorSubmissionFailure)Error()string{return "Operator submission could not be reconciled."}
func(e *OperatorSubmissionFailure)Unwrap()error{return e.Cause}
// The last HTTP observation before POST is the fresh trusted four-part
// context. Availability does not replace the original immutable preview.
func(c *Client)OperatorExecute(ctx context.Context,request OperatorExecuteRequest,preview OperatorPreview,binding OperatorContext,source OperatorExecutionSource)(OperatorSubmission,error){
 if !binding.Valid()||!operatorPreviewValid(preview){return OperatorSubmission{},&OperatorPublicError{Code:"invalid_request"}}
 if err:=operatorExecuteRequestValid(request,preview);err!=nil{return OperatorSubmission{},err}
 if err:=operatorPreviewExpiryAt(preview,time.Now());err!=nil{return OperatorSubmission{},err}
 before,err:=c.OperatorContext(ctx);if err!=nil{return OperatorSubmission{},err};if before!=binding{return OperatorSubmission{},&OperatorError{Stage:"context"}}
 observed:=operatorExecutionObservation{};if err=c.operatorExecutionAvailable(ctx,preview,source,&observed);err!=nil{return OperatorSubmission{},err}
 after,err:=c.OperatorContext(ctx);if err!=nil{return OperatorSubmission{},err};if after!=binding{return OperatorSubmission{},&OperatorError{Stage:"context"}}
 if err:=operatorPreviewExpiryAt(preview,time.Now());err!=nil{return OperatorSubmission{},err}
 if observed.SnapshotExpiry!=""&&OperatorSnapshotExpiredAt(observed.SnapshotExpiry,time.Now()){return OperatorSubmission{},&OperatorPublicError{Code:"snapshot_expired"}}
 result,err:=c.operatorExecute(ctx,request,preview);if err!=nil{return result,&OperatorSubmissionFailure{Dispatched:true,Cause:err}};return result,nil
}
type operatorExecutionObservation struct{SnapshotExpiry string}
func(c *Client)operatorExecutionAvailable(ctx context.Context,p OperatorPreview,source OperatorExecutionSource,observed *operatorExecutionObservation)error{
 stale:=func()error{return &OperatorPublicError{Code:"binding_stale"}}
 if !operatorEnum(p.ActionID,"service_install","service_configure","service_start","service_stop","service_restart","update_check","update_download"){
  detail,err:=c.OperatorDetail(ctx,p.ServiceID);if err!=nil{return err};if detail.Summary.DefinitionRevision!=p.DefinitionRevision||detail.Summary.Generation!=p.Generation||detail.Summary.ModelEpoch!=p.ModelEpoch{return stale()}
 }
 switch p.ActionID{
 case "service_install","service_configure","service_start","service_stop","service_restart","update_check","update_download":
  current,err:=c.OperatorLifecycleAvailability(ctx,p.ServiceID);if err!=nil{return err};if !current.Admits(p){return stale()}
 case "config_patch":
  current,err:=c.OperatorConfigForm(ctx,p.ServiceID);if err!=nil{return err};if current.DefinitionRevision!=p.DefinitionRevision||current.ModelEpoch!=p.ModelEpoch||!current.ValidateOperations(source.ConfigOperations){return stale()}
 case "setup_step_run":
  var input OperatorSetupInput;if err:=decodeOperator(source.Input,&input,OperatorRequestBytes);err!=nil{return &OperatorError{Stage:"binding_stale",cause:err}};current,err:=c.OperatorSetupForm(ctx,p.ServiceID);if err!=nil{return err};if current.DefinitionRevision!=p.DefinitionRevision||!OperatorSetupInputValid(current,input){return stale()};matched:=false;for _,step:=range current.Steps{if step.StepID==input.StepID&&step.ExpectedRevision!=nil&&*step.ExpectedRevision==p.ExpectedRevision{matched=true}};if !matched{return stale()}
 case "declared_action_run":
  var input OperatorDeclaredInput;if err:=decodeOperator(source.Input,&input,OperatorRequestBytes);err!=nil{return &OperatorError{Stage:"binding_stale",cause:err}};current,err:=c.OperatorActionForm(ctx,p.ServiceID);if err!=nil{return err};inputOK,inputErr:=OperatorDeclaredInputValidCause(current,input);if inputErr!=nil{return &OperatorError{Stage:"binding_stale",cause:inputErr}};if current.DefinitionRevision!=p.DefinitionRevision||!inputOK{return stale()};matched:=false;for _,action:=range current.Actions{if action.ActionID==input.DeclaredActionID&&action.DefinitionRevision==p.DefinitionRevision&&action.ExpectedRevision!=nil&&*action.ExpectedRevision==p.ExpectedRevision{matched=true}};if !matched{return stale()}
 case "update_install":
  var input OperatorUpdateInput;if err:=decodeOperator(source.Input,&input,OperatorRequestBytes);err!=nil{return &OperatorError{Stage:"binding_stale",cause:err}};current,err:=c.OperatorUpdateForm(ctx,p.ServiceID);if err!=nil{return err};if !current.Availability.Available||current.DefinitionRevision!=p.DefinitionRevision||current.CandidateID==nil||*current.CandidateID!=input.CandidateID||current.ExpectedRevision==nil||*current.ExpectedRevision!=p.ExpectedRevision{return stale()}
 case "service_remove_retain_data":
  current,err:=c.OperatorRemovalForm(ctx,p.ServiceID);if err!=nil{return err};if !current.Availability.Available||current.DefinitionRevision!=p.DefinitionRevision||current.Generation!=p.Generation||current.ExpectedRevision==nil||*current.ExpectedRevision!=p.ExpectedRevision{return stale()}
 case "queue_plan_execute":
  if source.QueueRow==nil||source.QueueRow.ServiceID!=p.ServiceID{return stale()};found:=false;var cursor *string;seen:=map[string]bool{};rows:=map[string]bool{};snapshot,expiry:="","";count:=0
  for{page,err:=c.OperatorQueue(ctx,p.ServiceID,"open",cursor);if err!=nil{return err};if OperatorSnapshotExpiredAt(page.ExpiresAt,time.Now()){return &OperatorPublicError{Code:"snapshot_expired"}};observed.SnapshotExpiry=page.ExpiresAt;if page.SourceRevision!=p.ExpectedRevision{return stale()};if cursor==nil{snapshot=page.SnapshotID;expiry=page.ExpiresAt}else if snapshot!=page.SnapshotID||expiry!=page.ExpiresAt{return stale()};if page.NextCursor!=nil&&len(page.Rows)==0{return stale()};count+=len(page.Rows);if count>4096{return stale()};for _,row:=range page.Rows{if rows[row.RowID]{return stale()};rows[row.RowID]=true;if row.RowID==source.QueueRow.RowID{if row.PlanID==nil||source.QueueRow.PlanID==nil||*row.PlanID!=*source.QueueRow.PlanID||row.Status!="open"{return stale()};found=true}};if page.NextCursor==nil{break};if seen[*page.NextCursor]{return stale()};seen[*page.NextCursor]=true;cursor=page.NextCursor};if !found{return stale()}
 default:return &OperatorPublicError{Code:"unsupported_capability"}
 };return nil
}
