package app

import (
	"context"
	"encoding/json"
	"errors"
	"strings"
	"testing"

	"github.com/service-lasso/service-lasso-tui/internal/api"
)

type editorEntropyFailure struct{cause error}
func(r editorEntropyFailure)Read([]byte)(int,error){return 0,r.cause}

// Source-only original/error/editor cases remain UNRUN.
func TestEditorDecodeFailuresPreserveOriginalDraftWithoutDispatch(t *testing.T){
	for _,kind:=range []string{"file_entry","root_entry"}{t.Run(kind,func(t *testing.T){
		binding:=selectedOperatorTestContext("a")
		raw:=[]byte(`{"private-source":@}`)
		field:=api.OperatorField{Selector:"/private-selector",Type:kind,Value:raw,Availability:api.OperatorAvailability{Supported:true,Available:true}}
		model:=operatorModel{binding:binding,draft:operatorDraft{Action:"config_patch",Context:binding,Input:[]byte("private-original-input"),Text:"private-editor-text",Fields:[]api.OperatorField{field},Values:map[string][]byte{field.Selector:[]byte("private-original-value")}}}
		if model.saveEditorField()||model.draft.Sent||model.loading||len(model.editorFailures)!=1{t.Fatal("local malformed field dispatched or lost failure")}
		failure:=model.editorFailures[0]
		var syntax *json.SyntaxError
		if !errors.As(failure.Err,&syntax)||failure.Draft.Context!=binding||string(failure.Draft.Input)!="private-original-input"||string(failure.Draft.Fields[0].Value)!=string(raw)||failure.Draft.Text!="private-editor-text"{t.Fatal("editor failure lost original cause/input/context")}
		encoded,err:=json.Marshal(failure);if err!=nil||string(encoded)!="{}"||strings.Contains(model.errorText,"private"){t.Fatal("private editor original leaked")}
		model.draft.Input[0]='X';model.draft.Values[field.Selector][0]='X'
		if string(failure.Draft.Input)!="private-original-input"||string(failure.Draft.Values[field.Selector])!="private-original-value"{t.Fatal("later draft edits changed retained original bytes")}
	})}
}

func TestEditorFieldNullRefusalDoesNotInventCause(t *testing.T){
	text,err:=operatorFieldTextCause(api.OperatorField{Type:"string",Value:json.RawMessage(`null`)})
	if text!=""||err!=nil{t.Fatal("null display invented parser failure")}
	_,err=operatorEditedValue(api.OperatorField{Type:"file_entry",Value:json.RawMessage(`null`)},"text")
	if err==nil||errors.Unwrap(err)!=nil{t.Fatal("null file object admitted or fabricated cause")}
}

func TestEditorEntropyFailureKeepsOriginalAndDoesNotMarkDispatch(t *testing.T){
	binding:=selectedOperatorTestContext("a")
	preview:=api.OperatorPreview{PlanID:"plan",ServiceID:"root",ActionID:"queue_plan_execute",DefinitionRevision:"definition",ExpectedRevision:"revision",Generation:"generation",ModelEpoch:"epoch",Targets:[]string{},Effects:[]string{},Availability:api.OperatorAvailability{Supported:true,Available:true},ExpiresAt:"2099-10-08T00:00:00.000Z"}
	if !api.OperatorConfirmationResponseValid(preview,""){t.Fatal("invalid original preview fixture")}
	model:=operatorModel{ctx:context.Background(),epoch:1,binding:binding,service:"root",journey:"queue",draft:operatorDraft{ReadConnection:1,ReadService:"root",ReadJourney:"queue",Service:"root",Context:binding,Preview:&preview,Input:[]byte("private-input")}}
	model.draft.ReadFilter=model.filter()
	model.acceptPageSelection(model.ticket())
	cause:=errors.New("private-original-entropy")
	command:=model.submitPreviewWithEntropy(editorEntropyFailure{cause:cause})
	if command!=nil||model.draft.Sent||model.loading||model.draft.Idempotency!=""||model.draft.MutationTicket!=nil||len(model.editorFailures)!=1||!errors.Is(model.editorFailures[0].Err,cause)||model.editorFailures[0].Draft.Preview!=&preview{t.Fatal("local entropy failure lost original preview or marked dispatch")}
	if strings.Contains(model.errorText,cause.Error()){t.Fatal("entropy original entered public error")}
}

func TestConfigFieldCandidateFailurePreservesPreGestureDraft(t *testing.T){
	ready:=api.OperatorAvailability{Supported:true,Available:true}
	bad:=api.OperatorField{Selector:"next-field",Type:"string",Value:json.RawMessage(`{"private-next":@}`),Availability:ready}
	original:=operatorDraft{Action:"config_patch",Selector:"prior-field",Text:"private-prior-text",Input:[]byte("private-prior-input"),Context:selectedOperatorTestContext("a"),Values:map[string][]byte{"prior-field":[]byte(`"prior"`)}}
	model:=operatorModel{config:&api.OperatorConfigForm{Fields:[]api.OperatorField{bad}},draft:original}
	before,err:=privateOperatorDraftSnapshot(original);if err!=nil{t.Fatal(err)}
	if model.chooseConfigField()||len(model.editorFailures)!=1{t.Fatal("rejected field candidate was adopted or error lost")}
	after,err:=privateOperatorDraftSnapshot(model.draft);if err!=nil||string(before)!=string(after){t.Fatal("failed candidate changed pre-gesture draft")}
	failure:=model.editorFailures[0];var syntax *json.SyntaxError
	if !errors.As(failure.Err,&syntax)||failure.Draft.Selector!="prior-field"||failure.Candidate==nil||failure.Candidate.Selector!=bad.Selector||string(failure.Candidate.Fields[0].Value)!=string(bad.Value){t.Fatal("candidate/original/cause evidence lost")}
}

func TestNullEntryReplacementIsSemanticRefusalWithoutPanicOrParserCause(t *testing.T){
	ready:=api.OperatorAvailability{Supported:true,Available:true}
	field:=api.OperatorField{Selector:"entry",Type:"file_entry"}
	entry:=api.OperatorCatalogEntry{Selector:"entry",Availability:ready,EditTargets:[]api.OperatorCatalogTarget{{TargetHandle:"issued-target",Availability:ready}}}
	model:=operatorModel{draft:operatorDraft{EditEntry:&entry}}
	value,err:=model.editedEntryValue(field,[]byte(`null`))
	if value!=nil||err==nil||errors.Unwrap(err)!=nil{t.Fatal("null object admitted or manufactured parser cause")}
}

func TestEntryControlsFailurePreservesOriginalChoicesAndRejectedCause(t *testing.T){
	ready:=api.OperatorAvailability{Supported:true,Available:true}
	field:=api.OperatorField{Selector:"entry",Type:"template_entry",Value:json.RawMessage(`{"targetHandle":@}`)}
	entry:=api.OperatorCatalogEntry{Selector:"entry",Availability:ready}
	original:=operatorDraft{TargetIndex:7,SourceIndex:9,Input:[]byte("private-original-entry"),Fields:[]api.OperatorField{field}}
	model:=operatorModel{config:&api.OperatorConfigForm{Catalogs:[]api.OperatorCatalog{{Entries:[]api.OperatorCatalogEntry{entry}}}},draft:original}
	if model.selectEntryControls(field)||model.draft.TargetIndex!=7||model.draft.SourceIndex!=9||model.draft.EditEntry!=nil||len(model.editorFailures)!=1{t.Fatal("failed entry read published choices or lost original")}
	var syntax *json.SyntaxError
	if !errors.As(model.editorFailures[0].Err,&syntax)||model.editorFailures[0].Candidate==nil||model.editorFailures[0].Candidate.EditEntry==nil{t.Fatal("entry candidate/decoder original lost")}
}
