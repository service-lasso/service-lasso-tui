package api

import (
	"context"
	"net/url"
	"unicode/utf8"
)

func operatorGlobalPage(path,id,key,filter string,cursor *string)string {
	query:=operatorPaging(cursor,25,key,filter)
	if id!=""{query+="&serviceId="+url.QueryEscape(id)}
	return path+query
}

func operatorPageRows[T any](page OperatorPage[T], valid func(T) bool) bool {
	if !page.ValidHeader(25) { return false }
	for _, row := range page.Rows { if !valid(row) { return false } }
	return true
}

func (c *Client) OperatorInbox(ctx context.Context, id, filter string, cursor *string) (OperatorPage[OperatorInboxRow], error) {
	if id!=""&&!OperatorServiceID(id) {return OperatorPage[OperatorInboxRow]{},&OperatorPublicError{Code:"invalid_request"}}
	if !operatorEnum(filter,"all","unread","updates","system","workflow","service","errors","hidden") { return OperatorPage[OperatorInboxRow]{}, &OperatorPublicError{Code:"invalid_request"} }
	return operatorRequest(ctx,c,"GET",operatorGlobalPage("/inbox",id,"filter",filter,cursor),nil,func(p OperatorPage[OperatorInboxRow]) bool {
		return operatorPageRows(p,func(r OperatorInboxRow) bool { return OperatorID(r.RowID)&&OperatorServiceID(r.ServiceID)&&(id==""||r.ServiceID==id)&&operatorEnum(r.Kind,"system","workflow","service","update","security","help","error")&&operatorEnum(r.Source,"runtime","service","workflow","updater","broker","admin-ui","system")&&operatorEnum(r.Severity,"info","success","warning","error","critical")&&operatorEnum(r.Status,"unread","read")&&operatorEnum(r.Visibility,"visible","hidden")&&OperatorTime(r.CreatedAt)&&operatorDisplay(r.Summary,512)&&operatorUniqueEnum(r.CapabilityIDs,128,OperatorCapabilityIDs) })
	},200)
}

func (c *Client) OperatorQueue(ctx context.Context,id,filter string,cursor *string)(OperatorPage[OperatorQueueRow],error) {
	if id!=""&&!OperatorServiceID(id){return OperatorPage[OperatorQueueRow]{},&OperatorPublicError{Code:"invalid_request"}}
	if !operatorEnum(filter,"all","open","acknowledged","deferred"){return OperatorPage[OperatorQueueRow]{},&OperatorPublicError{Code:"invalid_request"}}
	return operatorRequest(ctx,c,"GET",operatorGlobalPage("/queue",id,"status",filter,cursor),nil,func(p OperatorPage[OperatorQueueRow])bool{return operatorPageRows(p,func(r OperatorQueueRow)bool{return OperatorID(r.RowID)&&OperatorServiceID(r.ServiceID)&&(id==""||r.ServiceID==id)&&operatorEnum(r.Kind,"update","recovery","diagnostic","blocked_start","failed_check","config_drift","manual")&&operatorEnum(r.Status,"open","acknowledged","deferred")&&operatorEnum(r.Severity,"info","warning","critical")&&OperatorTime(r.CreatedAt)&&operatorDisplay(r.Summary,512)&&(r.PlanID==nil||OperatorID(*r.PlanID))})},200)
}

func (c *Client) OperatorAudit(ctx context.Context,id,filter string,cursor *string)(OperatorPage[OperatorAuditRow],error) {
	if id!=""&&!OperatorServiceID(id){return OperatorPage[OperatorAuditRow]{},&OperatorPublicError{Code:"invalid_request"}}
	if !operatorEnum(filter,"all","success","failure","denied","skipped"){return OperatorPage[OperatorAuditRow]{},&OperatorPublicError{Code:"invalid_request"}}
	return operatorRequest(ctx,c,"GET",operatorGlobalPage("/audit",id,"status",filter,cursor),nil,func(p OperatorPage[OperatorAuditRow])bool{return operatorPageRows(p,func(r OperatorAuditRow)bool{return OperatorID(r.RowID)&&OperatorServiceID(r.ServiceID)&&(id==""||r.ServiceID==id)&&operatorDisplay(r.Actor,128)&&operatorDisplay(r.Action,128)&&operatorEnum(r.Status,"success","failure","denied","skipped")&&OperatorTime(r.Time)&&operatorDisplay(r.Summary,512)&&operatorEnum(r.Integrity,"verified","failed","unverified")})},200)
}

func (c *Client) OperatorHistory(ctx context.Context,id,filter string,cursor *string)(OperatorPage[OperatorHistoryRow],error) {
	path,err:=operatorServicePath(id);if err!=nil{return OperatorPage[OperatorHistoryRow]{},err}
	if !operatorEnum(filter,"lifecycle","setup","action","update"){return OperatorPage[OperatorHistoryRow]{},&OperatorPublicError{Code:"invalid_request"}}
	return operatorRequest(ctx,c,"GET",path+"/history"+operatorPaging(cursor,25,"kind",filter),nil,func(p OperatorPage[OperatorHistoryRow])bool{return operatorPageRows(p,func(r OperatorHistoryRow)bool{
		if !OperatorID(r.RowID)||r.ServiceID!=id||!operatorEnum(r.Kind,"lifecycle","setup","action","update","health")||!operatorDisplay(r.Summary,512)||(r.StartedAt!=nil&&!OperatorTime(*r.StartedAt))||(r.EndedAt!=nil&&!OperatorTime(*r.EndedAt)){return false}
		return r.Kind==filter&&operatorHistoryState(r)
	})},200)
}

func (c *Client) OperatorLogs(ctx context.Context,id,stream string,cursor *string)(OperatorPage[OperatorLogRow],error) {
	return c.OperatorSearchLogs(ctx,id,stream,"",cursor)
}
// Search uses the selected v2 query; no legacy q/query route or source handle
// is inferred. The server owns actual source disclosure and authorization.
func (c *Client) OperatorSearchLogs(ctx context.Context,id,stream,search string,cursor *string)(OperatorPage[OperatorLogRow],error) {
	path,err:=operatorServicePath(id);if err!=nil{return OperatorPage[OperatorLogRow]{},err}
	if !operatorEnum(stream,"combined","stdout","stderr","declared")||!utf8.ValidString(search)||len(search)>256{return OperatorPage[OperatorLogRow]{},&OperatorPublicError{Code:"invalid_request"}}
	query:=operatorPaging(cursor,25,"stream",stream);if search!=""{query+="&search="+url.QueryEscape(search)}
	return operatorRequest(ctx,c,"GET",path+"/logs"+query,nil,func(p OperatorPage[OperatorLogRow])bool{return operatorPageRows(p,func(r OperatorLogRow)bool{return OperatorID(r.RowID)&&r.ServiceID==id&&operatorEnum(r.Stream,"combined","stdout","stderr","declared")&&OperatorID(r.RunID)&&OperatorTime(r.Time)&&operatorDisplay(r.Text,4096)})},200)
}

func operatorHistoryState(r OperatorHistoryRow)bool{
	if r.Outcome==nil{return (r.Kind=="lifecycle"||r.Kind=="update")&&operatorLegalOperationState(r.Status,r.Phase,r.Outcome)}
	switch r.Kind{
	case "setup","action":
		if r.Kind=="action"&&r.Status=="skipped"{return false}
		phase:=r.Status;if r.Status=="succeeded"{phase="completed"}
		return operatorEnum(r.Status,"succeeded","failed","timeout","skipped")&&r.Phase==phase&&*r.Outcome==r.Status
	case "health":return operatorEnum(r.Status,"healthy","unhealthy")&&r.Phase=="observed"&&*r.Outcome==r.Status
	case "update":if r.Phase=="checked"{return operatorEnum(r.Status,"latest","update_available","pinned","unavailable","check_failed")&&*r.Outcome==r.Status}
	}
	return operatorLegalOperationState(r.Status,r.Phase,r.Outcome)
}

func (c *Client) OperatorVariables(ctx context.Context,id,scope string,cursor *string)(OperatorPage[OperatorVariable],error){
	path,err:=operatorServicePath(id);if err!=nil{return OperatorPage[OperatorVariable]{},err};if !operatorEnum(scope,"all","manifest","global","runtime","derived"){return OperatorPage[OperatorVariable]{},&OperatorPublicError{Code:"invalid_request"}}
	return operatorRequest(ctx,c,"GET",path+"/variables"+operatorPaging(cursor,25,"scope",scope),nil,func(p OperatorPage[OperatorVariable])bool{return operatorPageRows(p,operatorVariableValid)},200)
}
func (c *Client) OperatorEndpoints(ctx context.Context,id string,cursor *string)(OperatorPage[OperatorEndpoint],error){
	path,err:=operatorServicePath(id);if err!=nil{return OperatorPage[OperatorEndpoint]{},err}
	return operatorRequest(ctx,c,"GET",path+"/endpoints"+operatorPaging(cursor,25,"",""),nil,func(p OperatorPage[OperatorEndpoint])bool{return operatorPageRows(p,operatorEndpointValid)},200)
}
func (c *Client) OperatorHealthHistory(ctx context.Context,id,status string,cursor *string)(OperatorPage[OperatorHistoryRow],error){
	path,err:=operatorServicePath(id);if err!=nil{return OperatorPage[OperatorHistoryRow]{},err};if !operatorEnum(status,"all","healthy","unhealthy"){return OperatorPage[OperatorHistoryRow]{},&OperatorPublicError{Code:"invalid_request"}}
	return operatorRequest(ctx,c,"GET",path+"/health-history"+operatorPaging(cursor,25,"status",status),nil,func(p OperatorPage[OperatorHistoryRow])bool{return operatorPageRows(p,func(r OperatorHistoryRow)bool{return OperatorID(r.RowID)&&r.ServiceID==id&&r.Kind=="health"&&operatorHistoryState(r)&&(r.StartedAt==nil||OperatorTime(*r.StartedAt))&&(r.EndedAt==nil||OperatorTime(*r.EndedAt))&&operatorDisplay(r.Summary,512)})},200)
}
