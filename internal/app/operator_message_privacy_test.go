package app

import (
	"encoding/json"
	"errors"
	"strings"
	"testing"

	"github.com/service-lasso/service-lasso-tui/internal/api"
)

// Private test-only snapshots bypass public serialization so existing editor
// immutability tests still compare actual fields rather than two empty objects.
func privateOperatorDraftSnapshot(draft operatorDraft)([]byte,error){
	type privateSnapshot operatorDraft
	return json.Marshal(privateSnapshot(draft))
}

// Source cases remain UNRUN. No raw snapshot is persisted or published.
func TestOperatorMessagesClosePrivateJSONWithoutErasingOwnerState(t *testing.T){
	binding:=selectedOperatorTestContext("a")
	cause:=errors.New("private-message-original-cause")
	privateRef:="private-payload-reference"
	raw:=[]byte("private-retained-record")
	ticket:=operatorTicket{Binding:binding,Search:"private-message-ticket"}
	restored:=operatorRestoration{Raw:raw,Migration:raw,Warning:cause,Record:api.OperatorSavedV3{Context:binding}}
	draft:=operatorDraft{Context:binding,Input:raw,Values:map[string][]byte{"private-selector":raw},PayloadRef:&privateRef,Text:"private-editor-text",MutationTicket:&ticket}
	for _,value:=range []any{
		operatorDirectResult{Ticket:ticket,Binding:binding,Err:cause},
		operatorLoaded{Ticket:ticket,Cursor:&privateRef,Data:map[string]any{"private-data":raw},Err:cause},
		operatorBooted{Context:binding,Restore:restored,Err:cause},
		operatorPoll{Binding:binding,Err:cause},
		operatorPreviewed{Ticket:ticket,Preview:api.OperatorPreview{PlanID:"private-preview-plan"},Err:cause},
		operatorSubmitted{Ticket:ticket,Binding:binding,Expected:raw,Err:cause},
		operatorCancelled{Binding:binding,Err:cause},
		operatorDraftBinding{Ticket:ticket,Err:cause},
		restored,draft,
	}{
		encoded,err:=json.Marshal(value);if err!=nil{t.Fatal(err)}
		for _,private:=range []string{binding.InstanceBinding,binding.WorkspaceBinding,binding.ActorBinding,binding.ClientBinding,cause.Error(),ticket.Search,privateRef,"private-selector","private-editor-text","private-preview-plan"}{
			if private==""||strings.Contains(string(encoded),private){t.Fatal("private message state entered JSON")}
		}
		var fields map[string]json.RawMessage;if err:=json.Unmarshal(encoded,&fields);err!=nil{t.Fatal(err)}
		for _,name:=range []string{"Binding","Context","Ticket","Expected","Input","Values","PayloadRef","Restore","Warning","Raw","Migration","Data","Err","Preview"}{if _,exists:=fields[name];exists{t.Fatal("private JSON field remains exported")}}
	}
	if draft.Context!=binding || draft.PayloadRef!=&privateRef || string(draft.Input)!=string(raw) || restored.Record.Context!=binding || !errors.Is(restored.Warning,cause){t.Fatal("JSON projection erased internal original state")}
	before,err:=privateOperatorDraftSnapshot(draft);if err!=nil{t.Fatal(err)}
	draft.Text="changed-private-editor-text"
	after,err:=privateOperatorDraftSnapshot(draft);if err!=nil||string(before)==string(after){t.Fatal("private snapshot no longer detects actual draft changes")}
}
