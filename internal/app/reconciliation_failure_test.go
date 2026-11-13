package app

import (
	"context"
	"errors"
	"strings"
	"testing"

	"github.com/service-lasso/service-lasso-tui/internal/api"
)

type reconciliationFailureClient struct {
	*recordingDurableClient
	contextCalls int
	contextFailure error
	failBefore bool
}
func(c *reconciliationFailureClient)ReconciliationContext()(string,bool,error){
	c.contextCalls++
	if c.failBefore || c.contextCalls>1{return "",false,c.contextFailure}
	return c.reconciliation,true,nil
}

// Source-authored command/reducer cases remain UNRUN. In-memory retention is
// not durable capture, authority for returned readbacks or finite-budget proof.
func TestCompatibilityPostRequestContextFailureRetainsActualOperationAndOriginals(t *testing.T){
	for _, mode := range []string{"submit","read","cancel"}{t.Run(mode,func(t *testing.T){
		original:=errors.New("private-original-"+mode)
		contextCause:=errors.New("private-post-context-"+mode)
		binding:="slrc1_"+strings.Repeat("a",64)
		operation:=api.Operation{ID:"actual-returned-operation",Status:"running"}
		base:=&recordingDurableClient{reconciliation:binding,operation:operation,submitErr:original,operationErr:original,cancelErr:original,availability:[]api.LifecycleAvailability{{Action:"start",Available:true,RequiresConfirmation:true}}}
		client:=&reconciliationFailureClient{recordingDurableClient:base,contextFailure:contextCause}
		preview:=api.LifecyclePreview{ServiceID:"root",Action:"start",ReconciliationContext:binding}
		submission:=operationSubmission{id:9,client:client,reconciliationContext:binding,preview:&preview,operation:operation}
		var message operationMsg
		switch mode{
		case "submit":submission.operation=api.Operation{};message=submitDurableLifecycle(submission,preview,context.Background())().(operationMsg)
		case "read":message=readOperation(submission,context.Background())().(operationMsg)
		case "cancel":message=cancelOperation(submission,context.Background())().(operationMsg)
		}
		if !message.attempted || message.binding!=binding || message.operation.ID!=operation.ID || !errors.Is(message.err,original) || !errors.Is(message.err,contextCause) || message.cancellation!=(mode=="cancel") {t.Fatal("combined failure lost original returned outcome or dispatch evidence")}
		current:=model{operation:&submission,connections:&fakeConnectionManager{current:"next"}}
		updated,_:=current.Update(message);retained:=updated.(model)
		if len(retained.operationFailures)!=1 || retained.operationFailures[0].operation.ID!=operation.ID || !errors.Is(retained.operationFailures[0].err,original) || !errors.Is(retained.operationFailures[0].err,contextCause) {t.Fatal("reducer discarded actual failed readback or originals")}
		if mode=="submit" && (retained.operation.operation.ID!="" || !retained.operation.uncertainWithoutID) {t.Fatal("failed context read granted returned operation authority")}
		retained.activateConnection(base)
		if len(retained.operationFailures)!=1 || !errors.Is(retained.operationFailures[0].err,original) || strings.Contains(retained.View(),original.Error()) {t.Fatal("navigation erased or exposed private original failure")}
	})}
}

func TestCompatibilityFailedInitialContextDoesNotDispatch(t *testing.T){
	cause:=errors.New("private-initial-context-failure")
	base:=&recordingDurableClient{}
	client:=&reconciliationFailureClient{recordingDurableClient:base,contextFailure:cause,failBefore:true}
	prepared:=prepareLifecycle(client,context.Background(),1,1,"root","start")().(lifecyclePreparedMsg)
	if !errors.Is(prepared.err,cause){t.Fatal("local preparation lost original context failure")}
	submitted:=submitDurableLifecycle(operationSubmission{id:1,client:client},api.LifecyclePreview{ServiceID:"root",Action:"start"},context.Background())().(operationMsg)
	availability,preview,submit,read,cancel:=base.calls()
	if submitted.attempted || !errors.Is(submitted.err,cause) || availability+preview+submit+read+cancel!=0 {t.Fatal("failed context read dispatched or lost original")}
}
