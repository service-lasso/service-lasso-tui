package app

import (
	"encoding/json"
	"errors"
	"strings"
	"testing"
)

// Source-only cases: no tests or native execution have been admitted.
func TestTerminalRefreshJSONExcludesActualPrivateBindingTicketAndCause(t *testing.T) {
	binding := selectedOperatorTestContext("a")
	cause := errors.New("private-terminal-refresh-original")
	ticket := operatorTicket{Binding:binding,Search:"private-terminal-ticket"}
	failure := operatorTerminalRefreshFailure{Epoch:3,Sequence:7,Binding:binding,Err:cause}
	result := operatorTerminalRefreshed{Epoch:3,Sequence:7,Binding:binding,Ticket:ticket,Err:cause}
	for _, value := range []any{failure,result} {
		encoded, err := json.Marshal(value)
		if err != nil { t.Fatal(err) }
		for _, private := range []string{binding.InstanceBinding,binding.WorkspaceBinding,binding.ActorBinding,binding.ClientBinding,cause.Error(),ticket.Search} {
			if private == "" || strings.Contains(string(encoded),private) { t.Fatal("serialized private terminal-refresh state") }
		}
		var fields map[string]json.RawMessage
		if err := json.Unmarshal(encoded,&fields); err != nil { t.Fatal(err) }
		for _, name := range []string{"Binding","Ticket","Err"} {
			if _, exists := fields[name]; exists { t.Fatal("private field remains publicly serializable") }
		}
	}
	if failure.Binding != binding || result.Binding != binding || result.Ticket.Search != ticket.Search || !errors.Is(failure.Err,cause) || !errors.Is(result.Err,cause) {
		t.Fatal("serialization repair lost internal attempt identity")
	}
}
