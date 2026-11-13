package api

import (
 "net/url"
 "reflect"
 "strings"
)

type operatorErrorPolicy struct{Status int;Retry string}
var operatorErrorPolicies=map[string]operatorErrorPolicy{
 "invalid_request":{400,"never"},"invalid_utf8":{400,"never"},"duplicate_key":{400,"never"},"limit_exceeded":{400,"never"},"unsupported_version":{400,"never"},
 "unsupported_capability":{422,"never"},"unsupported_selector":{422,"never"},"validation_failed":{422,"new_preview"},"authentication_required":{401,"never"},"permission_denied":{403,"never"},
 "confirmation_required":{409,"new_preview"},"confirmation_expired":{409,"new_preview"},"binding_stale":{409,"new_preview"},"revision_conflict":{409,"new_preview"},"operation_conflict":{409,"never"},
 "operation_unknown":{503,"never"},"source_unavailable":{503,"read_retry"},"source_corrupt":{503,"read_retry"},"snapshot_expired":{410,"read_retry"},"source_replaced":{409,"read_retry"},
 "storage_unavailable":{503,"read_retry"},"dependency_blocked":{422,"new_preview"},"ownership_unknown":{503,"never"},"not_found":{404,"never"},"internal_error":{500,"never"},
}
func operatorErrorPolicyValid(body operatorErrorBody,status int)bool{policy,ok:=operatorErrorPolicies[body.Code];return ok&&status==policy.Status&&body.MessageKey=="operator.error."+body.Code&&body.Retryability==policy.Retry}
type operatorErrorSelectorOwner interface{operatorErrorSelectors()[]string}
func operatorErrorSelectorAllowed(selector,path string,body any)bool{
 if !operatorDisplay(selector,128)||selector==""{return false}
 if owner,ok:=body.(operatorErrorSelectorOwner);ok{for _,candidate:=range owner.operatorErrorSelectors(){if selector==candidate{return true}}}
 // Only schema-owned wire members are admitted. Raw action/file/payload data
 // is never traversed to manufacture a field name, path or diagnostic grant.
 if body!=nil{
  value:=reflect.ValueOf(body);if value.Kind()==reflect.Pointer{value=value.Elem()}
  if value.Kind()==reflect.Struct{typ:=value.Type();for i:=0;i<typ.NumField();i++{name:=strings.Split(typ.Field(i).Tag.Get("json"),",")[0];if name!=""&&name!="-"&&selector==name{return true}}}
  if values,ok:=body.(map[string]any);ok{for name:=range values{if operatorEnum(name,"expectedRevision","favorite","position")&&name==selector{return true}}}
 }
 parsed,err:=url.Parse(path);if err==nil{for name:=range parsed.Query(){if operatorEnum(name,"cursor","limit","filter","scope","status","kind","stream","serviceId","sourceHandle","follow","search")&&name==selector{return true}}}
 return false
}
