package app

import (
	"context"
	"encoding/json"
	"errors"
	"strconv"
	"strings"
	"testing"

	"github.com/service-lasso/service-lasso-tui/internal/api"
)

type privateSavedCauseHeld struct{raw []byte;reads,publishes,removes int}
func(h *privateSavedCauseHeld)read()([]byte,error){h.reads++;return append([]byte(nil),h.raw...),nil}
func(h *privateSavedCauseHeld)publish(raw []byte)error{h.publishes++;h.raw=append([]byte(nil),raw...);return nil}
func(h *privateSavedCauseHeld)remove()error{h.removes++;h.raw=nil;return nil}
func(h *privateSavedCauseHeld)close()error{return nil}
func(h *privateSavedCauseHeld)retainOwnership(){}

// Source-only connected caller cases remain UNRUN and perform no native work.
func TestSavedStoreCallersPreserveDecodeOriginalsAndRetainedBytes(t *testing.T){
	raw:=[]byte(`{"version":1e999}`)
	held:=&privateSavedCauseHeld{raw:append([]byte(nil),raw...)}
	store:=fileOperationStore{path:"source-only-held-record",held:held}
	loaded,loadErr:=store.Load()
	if loaded!=nil{t.Fatal("failed stored record was admitted")}
	saveErr:=store.Save(persistedOperation{Version:2,OperationID:"exact",ConnectionName:"source",Binding:"slrc1_"+strings.Repeat("a",64)})
	replaceErr:=store.replaceRaw(raw,raw)
	restored,restoreErr:=restoreOperator(context.Background(),nil,"source",store)
	operation,_,_:=terminalRefreshFixture(t)
	clearErr:=clearOperator(store,raw,operation)
	for _,failure:=range []error{loadErr,saveErr,replaceErr,restoreErr,clearErr}{
		var original *strconv.NumError
		if !errors.As(failure,&original){t.Fatal("stored-record caller lost actual numeric decoder failure")}
		encoded,err:=json.Marshal(failure);if err!=nil{t.Fatal(err)}
		if strings.Contains(string(encoded),"1e999")||strings.Contains(api.SafeError(failure),"1e999"){t.Fatal("stored-record original leaked")}
	}
	if held.publishes!=0||held.removes!=0||string(held.raw)!=string(raw)||string(restored.Raw)!=string(raw)||restored.Readback!=nil||restored.Migration!=nil{t.Fatal("decode failure changed retained original or dispatched storage")}
}

func TestStorageWarningRetainsActualReplacementDecodeFailure(t *testing.T){
	original:=[]byte("original-private-record")
	operation,_,_:=terminalRefreshFixture(t)
	failure:=&recordStorageOutcomeError{Outcome:recordStorageOutcome{PublicationEffect:recordReplacementApplied,CleanupRelease:recordReleaseUnresolved,OriginalKnown:true,Original:original,Replacement:[]byte(`{"version":1e999}`)}}
	model:=operatorModel{saved:append([]byte(nil),original...),binding:selectedOperatorTestContext("a"),operation:&api.OperatorReadback{Operation:operation}}
	model.storageWarning(failure)
	if !model.custodyBlocked||string(model.saved)!=string(original)||len(model.custodyErrors)!=2||model.custodyErrors[0]!=failure{t.Fatal("invalid replacement changed original custody or lost storage outcome")}
	var actual *strconv.NumError
	if !errors.As(model.custodyErrors[1],&actual){t.Fatal("observed replacement decoder original lost")}
}
