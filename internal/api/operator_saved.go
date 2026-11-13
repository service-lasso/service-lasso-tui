package api

import "regexp"

type OperatorSavedV2 struct { Version int `json:"version"`; OperationID string `json:"operationId"`; ConnectionName string `json:"connectionName"`; ReconciliationContext string `json:"reconciliationContext"` }
type OperatorSavedV3 struct { Version int `json:"version"`; OperationID string `json:"operationId"`; ServiceID string `json:"serviceId"`; ActionID string `json:"actionId"`; Context OperatorContext `json:"context"` }
var legacyContextDigest = regexp.MustCompile(`^slrc1_[a-f0-9]{64}$`)
func DecodeOperatorSavedV2(raw []byte)(OperatorSavedV2,error){
	var value OperatorSavedV2
	if err:=decodeOperator(raw,&value,4096);err!=nil{return OperatorSavedV2{},&OperatorError{Stage:"storage",cause:err}}
	if value.Version!=2||!OperatorID(value.OperationID)||!operatorDisplay(value.ConnectionName,128)||value.ConnectionName==""||!legacyContextDigest.MatchString(value.ReconciliationContext){return OperatorSavedV2{},&OperatorError{Stage:"storage"}}
	return value,nil
}
func DecodeOperatorSavedV3(raw []byte)(OperatorSavedV3,error){
	var value OperatorSavedV3
	if err:=decodeOperator(raw,&value,4096);err!=nil{return OperatorSavedV3{},&OperatorError{Stage:"storage",cause:err}}
	if value.Version!=3||!OperatorID(value.OperationID)||!OperatorServiceID(value.ServiceID)||!operatorEnum(value.ActionID,"service_start","service_stop","service_restart","service_install","service_configure","update_check","update_download","setup_step_run","update_install","declared_action_run","service_remove_retain_data","config_patch","queue_plan_execute")||!value.Context.Valid(){return OperatorSavedV3{},&OperatorError{Stage:"storage"}}
	return value,nil
}
