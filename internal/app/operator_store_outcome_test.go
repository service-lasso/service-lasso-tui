package app

import (
 "bytes"
 "errors"
 "strings"
 "testing"

 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// Algebra/reducer source cases only; no native publication or restart proof.
func TestOperatorSettledEffectWarningDoesNotClaimOriginalBytes(t *testing.T){
 for _,effect:=range []recordPublicationEffect{recordReplacementApplied,recordClearApplied}{primary:=errors.New("private primary cause");observed:=&recordStorageOutcomeError{Outcome:recordStorageOutcome{PublicationEffect:effect,CleanupRelease:recordReleased,OriginalKnown:true,Original:[]byte("A"),Replacement:[]byte("B")},Primary:primary};warning:=observed.Error();if strings.Contains(warning,"Original record is retained")||strings.Contains(warning,"could not be published")||strings.Contains(warning,"private primary"){t.Fatal("settled effect warning contradicted actual effect or leaked cause")};if !errors.Is(observed,primary){t.Fatal("settled effect discarded primary cause")};if effect==recordReplacementApplied&&!strings.Contains(warning,"was replaced")||effect==recordClearApplied&&!strings.Contains(warning,"was removed"){t.Fatal("warning omitted actual settled effect")}}
}
func TestOperatorObservedAppliedRecordKeepsIndependentCleanupCustody(t *testing.T){
 binding:=selectedOperatorTestContext("a");operation:=api.OperatorOperation{OperationID:"operation",ServiceID:"service",ActionID:"service_start"}
 raw,err:=savedV3Bytes(api.OperatorSavedV3{Version:3,OperationID:operation.OperationID,ServiceID:operation.ServiceID,ActionID:operation.ActionID,Context:binding});if err!=nil{t.Fatal(err)}
 original:=[]byte("retained-original");cause:=errors.New("private native path and release error")
 observed:=&recordStorageOutcomeError{Outcome:recordStorageOutcome{PublicationEffect:recordReplacementApplied,CleanupRelease:recordReleaseUnresolved,OriginalKnown:true,Original:original,Replacement:raw,Releases:[]recordNativeRelease{{Owner:recordNativeIdentity{Platform:"linux",Kind:"parent",Handle:17,IdentityKnown:true,Device:1,Inode:2},Cause:cause}}},Cleanup:cause}
 model:=operatorModel{saved:append([]byte(nil),original...),binding:binding,operation:&api.OperatorReadback{Operation:operation},legacyRecord:true};model.storageWarning(observed)
 if !bytes.Equal(model.saved,raw)||model.legacyRecord||!model.custodyBlocked||model.custodyOutcome==nil||model.operation.Operation.OperationID!="operation"{t.Fatal("settled publication bytes or independent own-ID custody lost")}
 if strings.Contains(model.custodyWarning,"private native")||!errors.Is(observed,cause){t.Fatal("private cause leaked or disappeared")}
 if model.beginEdit()!=nil||model.submitPreview()!=nil||model.submitDirect()!=nil||model.cancelOperation()!=nil{t.Fatal("unresolved custody admitted mutation")}
 foreign:=operatorModel{saved:[]byte("record-B"),binding:binding,operation:&api.OperatorReadback{Operation:operation}};foreign.storageWarning(observed);if string(foreign.saved)!="record-B"{t.Fatal("late A outcome erased B")}
}
func TestOperatorObservedClearAndUnresolvedEffectStayDistinct(t *testing.T){
 original:=[]byte("record-A");operation:=&api.OperatorReadback{Operation:api.OperatorOperation{OperationID:"own-ID"}}
 model:=operatorModel{saved:append([]byte(nil),original...),operation:operation};model.storageWarning(&recordStorageOutcomeError{Outcome:recordStorageOutcome{PublicationEffect:recordClearApplied,CleanupRelease:recordReleaseUnresolved,OriginalKnown:true,Original:original},Cleanup:errors.New("release")})
 if model.saved!=nil||model.operation!=operation||!model.custodyBlocked{t.Fatal("settled clear confused with lost own-ID or released custody")}
 unresolved:=operatorModel{saved:append([]byte(nil),original...),operation:operation};unresolved.storageWarning(&recordStorageOutcomeError{Outcome:recordStorageOutcome{PublicationEffect:recordEffectUnresolved,CleanupRelease:recordReleaseUnresolved,OriginalKnown:true,Original:original,Replacement:[]byte("unknown-B")},Primary:errors.New("publication")})
 if !bytes.Equal(unresolved.saved,original)||!unresolved.custodyBlocked||unresolved.custodyOutcome.PublicationEffect!=recordEffectUnresolved{t.Fatal("unproved effect became unchanged/applied authority")}
}
