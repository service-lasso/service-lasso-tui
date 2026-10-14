package api

import (
	"context"
	"net/url"
)

type OperatorInboxMutation struct{RowID string `json:"rowId"`;SourceRevision string `json:"sourceRevision"`;Status string `json:"status"`;Visibility string `json:"visibility"`}
type OperatorQueueMutation struct{RowID string `json:"rowId"`;SourceRevision string `json:"sourceRevision"`;Status string `json:"status"`}
func(c *Client)OperatorInboxPatch(ctx context.Context,row OperatorInboxRow,revision,operation string,visibility *string)(OperatorInboxMutation,error){
	if !OperatorID(row.RowID)||!OperatorID(revision)||!operatorEnum(operation,"mark_read","set_visibility")||operation=="mark_read"&&visibility!=nil||operation=="set_visibility"&&(visibility==nil||!operatorEnum(*visibility,"visible","hidden")){return OperatorInboxMutation{},&OperatorPublicError{Code:"invalid_request"}}
	body:=struct{ExpectedRevision string `json:"expectedRevision"`;Op string `json:"op"`;Visibility *string `json:"visibility,omitempty"`}{revision,operation,visibility}
	return operatorRequest(ctx,c,"PATCH","/inbox/"+url.PathEscape(row.RowID),body,func(r OperatorInboxMutation)bool{return r.RowID==row.RowID&&OperatorID(r.SourceRevision)&&operatorEnum(r.Status,"unread","read")&&operatorEnum(r.Visibility,"visible","hidden")&&(operation!="mark_read"||r.Status=="read")&&(operation!="set_visibility"||r.Visibility==*visibility)},200)
}
func(c *Client)OperatorQueueAcknowledge(ctx context.Context,row OperatorQueueRow,revision string)(OperatorQueueMutation,error){
	if !OperatorID(row.RowID)||!OperatorID(revision){return OperatorQueueMutation{},&OperatorPublicError{Code:"invalid_request"}}
	body:=struct{ExpectedRevision string `json:"expectedRevision"`}{revision}
	return operatorRequest(ctx,c,"POST","/queue/"+url.PathEscape(row.RowID)+"/acknowledge",body,func(r OperatorQueueMutation)bool{return r.RowID==row.RowID&&OperatorID(r.SourceRevision)&&r.Status=="acknowledged"},200)
}
func(c *Client)OperatorQueueDefer(ctx context.Context,row OperatorQueueRow,revision,until string)(OperatorQueueMutation,error){
	if !OperatorID(row.RowID)||!OperatorID(revision)||!OperatorTime(until){return OperatorQueueMutation{},&OperatorPublicError{Code:"invalid_request"}}
	body:=struct{ExpectedRevision string `json:"expectedRevision"`;DeferredUntil string `json:"deferredUntil"`}{revision,until}
	return operatorRequest(ctx,c,"POST","/queue/"+url.PathEscape(row.RowID)+"/defer",body,func(r OperatorQueueMutation)bool{return r.RowID==row.RowID&&OperatorID(r.SourceRevision)&&r.Status=="deferred"},200)
}
func(c *Client)OperatorQueuePreview(ctx context.Context,row OperatorQueueRow,revision string)(OperatorPreview,error){
	if !OperatorID(row.RowID)||!OperatorID(revision)||!OperatorServiceID(row.ServiceID){return OperatorPreview{},&OperatorPublicError{Code:"invalid_request"}}
	body:=struct{ExpectedRevision string `json:"expectedRevision"`}{revision}
	return operatorRequest(ctx,c,"POST","/queue/"+url.PathEscape(row.RowID)+"/preview",body,func(p OperatorPreview)bool{return operatorPreviewValid(p)&&p.ServiceID==row.ServiceID&&p.ActionID=="queue_plan_execute"&&p.ExpectedRevision==revision},200)
}
