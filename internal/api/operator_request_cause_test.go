package api

import (
	"context"
	"encoding/json"
	"errors"
	"io"
	"net/http"
	"strings"
	"testing"
)

// Source-only request cases remain UNRUN; fixtures supply no genuine issuer.
func TestPreviewLocalDecodeOriginalsRemainPrivateWithoutRequest(t *testing.T){
	var client *Client
	for _,action:=range []string{"service_start","setup_step_run","declared_action_run","update_install","service_remove_retain_data"}{
		value,failure:=client.OperatorPreview(context.Background(),OperatorPreviewRequest{ServiceID:"root",ActionID:action,DefinitionRevision:"definition",ExpectedRevision:"revision",Input:json.RawMessage(`{"private-input":@}`)})
		var syntax *json.SyntaxError
		if value.PlanID!=""||!errors.As(failure,&syntax)||SafeError(failure)!=OperatorMessage("invalid_request"){t.Fatal("preview discarded actual syntax error or admitted local failure")}
	}
	form:=operatorReferenceFormFixture()
	value,failure:=client.OperatorDeclaredPreview(context.Background(),OperatorPreviewRequest{ServiceID:form.ServiceID,ActionID:"declared_action_run",DefinitionRevision:form.DefinitionRevision,ExpectedRevision:"revision",Input:json.RawMessage(`{"declaredActionId":@}`)},form)
	var syntax *json.SyntaxError
	if value.PlanID!=""||!errors.As(failure,&syntax)||strings.Contains(SafeError(failure),"declaredActionId"){t.Fatal("declared local decoder lost or exposed original")}
	admitted,semantic:=operatorPreviewInputCause("setup_step_run",[]byte(`{"stepId":"","values":{},"includeManual":false,"force":false}`))
	if admitted||semantic!=nil{t.Fatal("semantic step refusal fabricated a decoder cause")}
}

func TestExecutionSourceDecodeRetainsActualOriginalBeforeFormOrMutationRequest(t *testing.T){
	ready:=OperatorAvailability{Supported:true,Available:true}
	detail:=OperatorDetail{Summary:OperatorService{ServiceID:"root",DisplayName:"Original",Description:"",DefinitionRevision:"definition",Generation:"generation",ModelEpoch:"epoch",Installed:"yes",Configured:"yes",Running:"yes",HealthStatus:"unknown",UpdateStatus:"not_checked",CapabilityIDs:[]string{}},Provider:OperatorProvider{Role:"service",Availability:ready},Dependencies:[]OperatorEdge{},Endpoints:[]OperatorEndpoint{},VariableMetadata:[]OperatorVariable{},SetupMetadata:[]OperatorSetupStep{},ActionMetadata:[]OperatorAction{}}
	if !detail.Valid(){t.Fatal("detail fixture violates selected contract")}
	raw,err:=json.Marshal(OperatorEnvelope[OperatorDetail]{ContractVersion:OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:detail});if err!=nil{t.Fatal(err)}
	for _,action:=range []string{"setup_step_run","declared_action_run","update_install"}{
		reads:=0
		client,err:=NewClient("http://127.0.0.1:1",&http.Client{Transport:retainedErrorTransport(func(request *http.Request)(*http.Response,error){if request.Method!="GET"||request.URL.Path!="/api/operator/v2/services/root"{t.Error("local source failure requested form or mutation")};reads++;return &http.Response{StatusCode:200,Header:http.Header{},Body:io.NopCloser(strings.NewReader(string(raw)))},nil})},"");if err!=nil{t.Fatal(err)}
		preview:=OperatorPreview{ServiceID:"root",ActionID:action,DefinitionRevision:"definition",ExpectedRevision:"revision",Generation:"generation",ModelEpoch:"epoch"}
		failure:=client.operatorExecutionAvailable(context.Background(),preview,OperatorExecutionSource{Input:[]byte(`{"private-input":@}`)},&operatorExecutionObservation{})
		var syntax *json.SyntaxError
		if reads!=1||!errors.As(failure,&syntax)||SafeError(failure)!=OperatorMessage("binding_stale"){t.Fatal("execution source original lost or public contract changed")}
	}
}
