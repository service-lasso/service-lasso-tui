package app

import (
	"context"


	"github.com/service-lasso/service-lasso-tui/internal/api"
)

type operatorRestoration struct {
	Record api.OperatorSavedV3
	Raw []byte
	Readback *api.OperatorReadback
	Legacy bool
	Warning error
	Migration []byte
}
// Recovery proposals retain raw original/migration bytes and server context
// only inside the owner. Ordinary JSON cannot serialize those private inputs.
func(operatorRestoration)MarshalJSON()([]byte,error){return []byte("{}"),nil}

func restoreOperator(ctx context.Context,client *api.Client,connection string,store fileOperationStore)(result operatorRestoration,failure error){
	raw,storageErr:=store.readRaw();defer func(){if storageErr!=nil{result.Warning=recordJoinedErrors(result.Warning,storageErr);failure=recordJoinedErrors(failure,storageErr);result.Migration=nil}}();if raw==nil{return operatorRestoration{},storageErr}
	result=operatorRestoration{Raw:append([]byte(nil),raw...)}
	three,threeErr:=api.DecodeOperatorSavedV3(raw)
	two,twoErr:=api.DecodeOperatorSavedV2(raw)
	if threeErr!=nil&&twoErr!=nil{return result,recordJoinedErrors(storageError(),threeErr,twoErr)}
	fresh,err:=client.OperatorContext(ctx);if err!=nil{return result,err}
	id:=three.OperationID
	if threeErr!=nil {
		result.Legacy=true
		if two.ConnectionName!=connection||two.ReconciliationContext!=fresh.LegacyDigest(){return result,&api.OperatorError{Stage:"context"}}
		id=two.OperationID
	}else if three.Context!=fresh{return result,&api.OperatorError{Stage:"context"}}
	// This trusted route must enforce exact own actor/client operation visibility.
	// A missing route or missing root identity stops migration, never list lookup.
	readback,err:=client.OperatorOperation(ctx,id);if err!=nil{return result,err}
	result.Readback=&readback
	after,err:=client.OperatorContext(ctx);if err!=nil||after!=fresh{return result,api.OperatorContextCause(err)}
	if threeErr==nil&&(readback.Operation.ServiceID!=three.ServiceID||readback.Operation.ActionID!=three.ActionID){return result,&api.OperatorError{Stage:"response"}}
	if threeErr==nil{result.Record=three;return result,nil}
	three=api.OperatorSavedV3{Version:3,OperationID:id,ServiceID:readback.Operation.ServiceID,ActionID:readback.Operation.ActionID,Context:fresh}
	replacement,err:=savedV3Bytes(three);if err!=nil{result.Warning=err;return result,nil}
	// Return a proposal. Only the current reducer ticket may publish it after
	// confirming the connection epoch; a cancelled old worker never migrates.
	result.Record=three;result.Migration=replacement;return result,nil
}

func persistOperator(store fileOperationStore,expected []byte,operation api.OperatorOperation,binding api.OperatorContext)([]byte,error){
	if !operation.Valid()||!binding.Valid(){return nil,storageError()}
	record:=api.OperatorSavedV3{Version:3,OperationID:operation.OperationID,ServiceID:operation.ServiceID,ActionID:operation.ActionID,Context:binding}
	raw,err:=savedV3Bytes(record);if err!=nil{return nil,err};if err=store.replaceRaw(expected,raw);err!=nil{if outcome,ok:=recordOutcome(err);ok&&outcome.replacementApplied(raw){return append([]byte(nil),raw...),err};return nil,err};return raw,nil
}
func clearOperator(store fileOperationStore,expected []byte,operation api.OperatorOperation)error{
	if !operation.CanClear(){return storageError()};record,err:=api.DecodeOperatorSavedV3(expected)
	if err!=nil{return recordJoinedErrors(storageError(),err)}
	if record.OperationID!=operation.OperationID||record.ServiceID!=operation.ServiceID||record.ActionID!=operation.ActionID{return storageError()}
	return store.clearRaw(expected)
}

// Bracket exact-own-ID readback with its original trusted context. A rotated
// context or foreign correlation preserves the retained operation and record;
// neither polling nor a denied read submits or discovers a replacement ID.
func readOperatorOwn(ctx context.Context,client *api.Client,binding api.OperatorContext,original api.OperatorOperation)(api.OperatorReadback,error){
 if !binding.Valid()||!original.Valid(){return api.OperatorReadback{},&api.OperatorError{Stage:"context"}}
 if err:=operatorSameContext(ctx,client,binding);err!=nil{return api.OperatorReadback{},err}
 readback,err:=client.OperatorOperation(ctx,original.OperationID);if err!=nil{return api.OperatorReadback{},err}
 if err:=operatorSameContext(ctx,client,binding);err!=nil{return readback,err}
 if !readback.Operation.SameRoot(original){return readback,&api.OperatorError{Stage:"response"}}
 return readback,nil
}
