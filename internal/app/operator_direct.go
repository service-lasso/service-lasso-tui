package app

import (
 "context"
 "errors"
 "strconv"
 "strings"

 tea "github.com/charmbracelet/bubbletea"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

type operatorDirectResult struct{Ticket operatorTicket `json:"-"`;Binding api.OperatorContext `json:"-"`;Meta *api.OperatorMeta;Err error `json:"-"`;Dispatched bool}
func operatorSameContext(ctx context.Context,client *api.Client,binding api.OperatorContext)error{fresh,err:=client.OperatorContext(ctx);if err!=nil{return err};if fresh!=binding{return &api.OperatorError{Stage:"context"}};return nil}
func(m *operatorModel)submitDirect()tea.Cmd{
 if m.mutationBlocked()||m.draft.Sent||!m.draft.Review{return nil};if !m.draftSelectionCurrent(){m.errorText=api.SafeError(&api.OperatorError{Stage:"context"});return nil};d,ticket,client,ctx,service:=m.draft,m.ticket(),m.client,m.ctx,m.draft.Service;m.draft.Sent=true;m.draft.MutationTicket=&ticket;m.loading=true
 return func()tea.Msg{
  result:=operatorDirectResult{Ticket:ticket,Binding:d.Context}
  fail:=func(err error)tea.Msg{result.Err=err;return result}
  if err:=operatorSameContext(ctx,client,d.Context);err!=nil{return fail(err)}
  discovery,err:=client.OperatorDiscovery(ctx);if err!=nil{return fail(err)};if allowed,code:=operatorCapabilityAdmission(&discovery,d.Action);!allowed{return fail(&api.OperatorPublicError{Code:code})}
  switch d.Action{
  case "service_meta_patch":
   observation:=operatorReadMetadata(ctx,client,ticket,service);if observation.Err!=nil{return fail(observation.Err)};current:=observation.Value;if d.MetadataGeneration==""||observation.Key.Generation!=d.MetadataGeneration||!current.Availability.Available||current.SourceRevision==nil||*current.SourceRevision!=d.Expected{return fail(&api.OperatorPublicError{Code:"revision_conflict"})}
   favorite,position,err:=operatorMetadataPatchInput(d.Selector,d.Text);if err!=nil{return fail(err)}
   if err:=operatorSameContext(ctx,client,d.Context);err!=nil{return fail(err)};result.Dispatched=true;meta,err:=client.OperatorPatchMeta(ctx,service,d.Expected,favorite,position);result.Meta=&meta;result.Err=err;return result
  case "inbox_mark_read","inbox_set_visibility":
   if d.InboxRow==nil{return fail(&api.OperatorPublicError{Code:"invalid_request"})};row,err:=operatorFreshInbox(ctx,client,*d.InboxRow,d.Expected,d.Action);if err!=nil{return fail(err)}
   operation:="mark_read";var visibility *string;if d.Action=="inbox_set_visibility"{operation="set_visibility";if d.Text!="hidden"&&d.Text!="visible"{return fail(&api.OperatorPublicError{Code:"invalid_request"})};value:=d.Text;visibility=&value}
   if err:=operatorSameContext(ctx,client,d.Context);err!=nil{return fail(err)};result.Dispatched=true;_,result.Err=client.OperatorInboxPatch(ctx,row,d.Expected,operation,visibility);return result
  case "queue_acknowledge","queue_defer":
   if d.QueueRow==nil{return fail(&api.OperatorPublicError{Code:"invalid_request"})};row,err:=operatorFreshQueue(ctx,client,*d.QueueRow,d.Expected);if err!=nil{return fail(err)}
   if d.Action=="queue_defer"&&!api.OperatorTime(d.Text){return fail(&api.OperatorPublicError{Code:"invalid_request"})};if err:=operatorSameContext(ctx,client,d.Context);err!=nil{return fail(err)};result.Dispatched=true
   if d.Action=="queue_acknowledge"{_,result.Err=client.OperatorQueueAcknowledge(ctx,row,d.Expected)}else{_,result.Err=client.OperatorQueueDefer(ctx,row,d.Expected,d.Text)};return result
  };return fail(&api.OperatorPublicError{Code:"unsupported_capability"})
 }
}
func operatorFreshInbox(ctx context.Context,client *api.Client,original api.OperatorInboxRow,revision,action string)(api.OperatorInboxRow,error){
 state:=operatorPageState[api.OperatorInboxRow]{};var cursor *string;var found *api.OperatorInboxRow
 for{page,err:=client.OperatorInbox(ctx,original.ServiceID,"all",cursor);if err!=nil{return api.OperatorInboxRow{},err};if page.SourceRevision!=revision{return api.OperatorInboxRow{},&api.OperatorPublicError{Code:"revision_conflict"}};if err=state.append(page,cursor,func(r api.OperatorInboxRow)string{return r.RowID});err!=nil{return api.OperatorInboxRow{},err};for _,row:=range page.Rows{if row.RowID==original.RowID{available:=false;for _,id:=range row.CapabilityIDs{if id==action{available=true}};if !available{return api.OperatorInboxRow{},&api.OperatorPublicError{Code:"permission_denied"}};copy:=row;found=&copy}};if state.Next==nil{break};cursor=state.Next}
 if found==nil{return api.OperatorInboxRow{},&api.OperatorPublicError{Code:"not_found"}};return *found,nil
}
func operatorFreshQueue(ctx context.Context,client *api.Client,original api.OperatorQueueRow,revision string)(api.OperatorQueueRow,error){
 state:=operatorPageState[api.OperatorQueueRow]{};var cursor *string;var found *api.OperatorQueueRow
 for{page,err:=client.OperatorQueue(ctx,original.ServiceID,"all",cursor);if err!=nil{return api.OperatorQueueRow{},err};if page.SourceRevision!=revision{return api.OperatorQueueRow{},&api.OperatorPublicError{Code:"revision_conflict"}};if err=state.append(page,cursor,func(r api.OperatorQueueRow)string{return r.RowID});err!=nil{return api.OperatorQueueRow{},err};for _,row:=range page.Rows{if row.RowID==original.RowID{copy:=row;found=&copy}};if state.Next==nil{break};cursor=state.Next}
 if found==nil{return api.OperatorQueueRow{},&api.OperatorPublicError{Code:"not_found"}};return *found,nil
}

func operatorMetadataPatchInput(selector,text string)(*bool,**api.OperatorPosition,error){
 invalid:=func(cause error)(*bool,**api.OperatorPosition,error){return nil,nil,api.OperatorInvalidRequestCause(cause)}
 switch selector{
 case "favorite":value,err:=strconv.ParseBool(text);if err!=nil{return invalid(err)};return &value,nil,nil
 case "position":var value *api.OperatorPosition;if text!="automatic"{parts:=strings.Split(text,",");if len(parts)!=2{return invalid(nil)};x,xErr:=strconv.Atoi(strings.TrimSpace(parts[0]));y,yErr:=strconv.Atoi(strings.TrimSpace(parts[1]));if xErr!=nil||yErr!=nil{return invalid(errors.Join(xErr,yErr))};if x < -100000||x>100000||y < -100000||y>100000{return invalid(nil)};value=&api.OperatorPosition{X:x,Y:y}};return nil,&value,nil
 default:return invalid(nil)
 }
}
