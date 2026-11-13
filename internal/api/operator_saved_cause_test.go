package api

import (
	"encoding/json"
	"errors"
	"strconv"
	"strings"
	"testing"
)

// Source-authored cases remain UNRUN pending entire-source and command admission.
func TestSavedRecordDecodeKeepsOriginalCausePrivate(t *testing.T) {
	for _, version := range []int{2, 3} {
		raw := []byte(`{"version":1e999}`)
		var err error
		if version == 2 {
			value, failure := DecodeOperatorSavedV2(raw)
			err = failure
			if value != (OperatorSavedV2{}) { t.Fatal("failure returned saved state") }
		} else {
			value, failure := DecodeOperatorSavedV3(raw)
			err = failure
			if value != (OperatorSavedV3{}) { t.Fatal("failure returned saved state") }
		}
		var original *strconv.NumError
		if !errors.As(err, &original) { t.Fatalf("v%d lost original typed decoder failure", version) }
		var stage *OperatorError
		if !errors.As(err, &stage) || stage.Stage != "storage" { t.Fatal("lost storage presentation") }
		if stage.Error() != SafeError(err) { t.Fatal("safe presentation changed") }
		encoded, encodeErr := json.Marshal(err)
		if encodeErr != nil { t.Fatal(encodeErr) }
		for _, public := range []string{err.Error(), SafeError(err), string(encoded)} {
			if strings.Contains(public, "1e999") || strings.Contains(public, original.Error()) {
				t.Fatal("public projection leaked decoder cause")
			}
		}
	}
}

func TestSavedRecordSemanticRefusalHasNoManufacturedCause(t *testing.T) {
	raw2, err := json.Marshal(OperatorSavedV2{Version:4,OperationID:"exact",ConnectionName:"source",ReconciliationContext:"slrc1_"+strings.Repeat("a",64)})
	if err!=nil { t.Fatal(err) }
	binding:="slrc_"+strings.Repeat("a",64)
	raw3, err := json.Marshal(OperatorSavedV3{Version:4,OperationID:"exact",ServiceID:"root",ActionID:"service_start",Context:OperatorContext{InstanceBinding:binding,WorkspaceBinding:binding,ActorBinding:binding,ClientBinding:binding}})
	if err!=nil { t.Fatal(err) }
	_, v2 := DecodeOperatorSavedV2(raw2)
	_, v3 := DecodeOperatorSavedV3(raw3)
	for _, err := range []error{v2, v3} {
		if err == nil || errors.Unwrap(err) != nil { t.Fatal("semantic refusal manufactured a cause") }
	}
}

func TestSavedRecordSyntaxFailureRetainsOriginal(t *testing.T){
	_, v2:=DecodeOperatorSavedV2([]byte(`{"version":@}`))
	_, v3:=DecodeOperatorSavedV3([]byte(`{"version":@}`))
	for _, failure:=range []error{v2,v3}{
		var original *json.SyntaxError
		if !errors.As(failure,&original){t.Fatal("saved record lost actual SyntaxError")}
	}
}
