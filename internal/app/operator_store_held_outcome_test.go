package app

import (
 "bytes"
 "context"
 "github.com/service-lasso/service-lasso-tui/internal/api"
 "encoding/json"
 "errors"
 "testing"
)

// Controlled original held-store outcomes exercise actual legacy and migration
// callers. They remain SOURCE fixtures, not native persistence qualifications.
type selectedOutcomeHeld struct{raw []byte;observed recordStorageOutcome;cause error;reads,publishes,removes,closes,retains int}
func(h *selectedOutcomeHeld)read()([]byte,error){h.reads++;return h.raw,nil}
func(h *selectedOutcomeHeld)publish(raw []byte)error{h.publishes++;h.raw=raw;h.observed.Replacement=raw;h.observed.PublicationEffect=recordReplacementApplied;h.observed.CleanupRelease=recordReleaseUnresolved;return h.cause}
func(h *selectedOutcomeHeld)remove()error{h.removes++;h.raw=nil;h.observed.PublicationEffect=recordClearApplied;h.observed.CleanupRelease=recordReleaseUnresolved;return h.cause}
func(h *selectedOutcomeHeld)close()error{h.closes++;return h.cause}
func(h *selectedOutcomeHeld)retainOwnership(){h.retains++}
func(h *selectedOutcomeHeld)storageOutcome()recordStorageOutcome{return h.observed}
func TestLegacyHeldSaveAndClearPreserveSettledEffectAndReleaseOriginal(t *testing.T){
 record:=persistedOperation{Version:2,OperationID:"exact-A",ConnectionName:"profile",Binding:selectedOperatorTestContext("a").LegacyDigest()}
 cause:=errors.New("private original release cause")
 held:=&selectedOutcomeHeld{cause:cause,observed:recordStorageOutcome{PublicationEffect:recordUnchanged,CleanupRelease:recordReleased,OriginalKnown:true}}
 store:=fileOperationStore{path:"controlled-held-source",held:held}
 failure:=store.Save(record);observed,ok:=recordOutcome(failure)
 if !ok||observed.PublicationEffect!=recordReplacementApplied||observed.CleanupRelease!=recordReleaseUnresolved||!errors.Is(failure,cause)||held.publishes!=1||held.closes!=0||held.retains==0{t.Fatal("legacy Save dropped effect/release or closed caller owner")}
 var actual persistedOperation;if err:=decodeOperationMetadata(held.raw,&actual);err!=nil||actual!=record{t.Fatal("legacy Save lost actual settled bytes")}
 original,_:=json.Marshal(record)
 held=&selectedOutcomeHeld{raw:original,cause:cause,observed:recordStorageOutcome{PublicationEffect:recordUnchanged,CleanupRelease:recordReleased,OriginalKnown:true,Original:original}}
 store.held=held;failure=store.Clear(record);observed,ok=recordOutcome(failure)
 if !ok||!observed.clearApplied(original)||observed.CleanupRelease!=recordReleaseUnresolved||!errors.Is(failure,cause)||held.raw!=nil||held.removes!=1||held.closes!=0{t.Fatal("legacy Clear replaced settled effect with unchanged or released")}
}
func TestHeldV3PersistenceAdoptsSettledReplacementWithReleaseWarning(t *testing.T){
 operation,_,_:=terminalRefreshFixture(t);binding:=selectedOperatorTestContext("a")
 // Source fixtures supply no principal or native entitlement.
 original:=[]byte("exact-old-record")
 cause:=errors.New("private cleanup original")
 held:=&selectedOutcomeHeld{raw:original,cause:cause,observed:recordStorageOutcome{PublicationEffect:recordUnchanged,CleanupRelease:recordReleased,OriginalKnown:true,Original:original}}
 store:=fileOperationStore{path:"controlled-held-source",held:held}
 raw,failure:=persistOperator(store,original,operation,binding)
 observed,ok:=recordOutcome(failure)
 if !ok||!observed.replacementApplied(raw)||!bytes.Equal(raw,held.raw)||!errors.Is(failure,cause)||held.closes!=0{t.Fatal("settled migration/persistence bytes or original release warning lost")}
}
func TestLegacyLateClearDoesNotEraseHeldNewOwner(t *testing.T){
 a:=persistedOperation{Version:2,OperationID:"A",ConnectionName:"profile",Binding:selectedOperatorTestContext("a").LegacyDigest()};b:=a;b.OperationID="B"
 original,_:=json.Marshal(b)
 held:=&selectedOutcomeHeld{raw:original,observed:recordStorageOutcome{PublicationEffect:recordUnchanged,CleanupRelease:recordReleased,OriginalKnown:true,Original:original}}
 failure:=(fileOperationStore{path:"controlled-held-source",held:held}).Clear(a)
 observed,ok:=recordOutcome(failure)
 if failure==nil||!ok||observed.PublicationEffect!=recordUnchanged||held.removes!=0||!bytes.Equal(held.raw,original)||held.closes!=0{t.Fatal("late A erased B or lost unchanged held outcome")}
}
func TestBootstrapMigrationAdoptsAppliedBytesWithoutDiscardingReleaseFailure(t *testing.T){
 operation,discovery,_:=terminalRefreshFixture(t);binding:=selectedOperatorTestContext("a")
 migrated:=api.OperatorSavedV3{Version:3,OperationID:operation.OperationID,ServiceID:operation.ServiceID,ActionID:operation.ActionID,Context:binding}
 replacement,err:=savedV3Bytes(migrated);if err!=nil{t.Fatal(err)}
 original:=[]byte("controlled-v2-preimage");cause:=errors.New("private migration cleanup original")
 held:=&selectedOutcomeHeld{raw:original,cause:cause,observed:recordStorageOutcome{PublicationEffect:recordUnchanged,CleanupRelease:recordReleased,OriginalKnown:true,Original:original}}
 model:=operatorModel{epoch:1,ctx:context.Background(),journey:"overview",store:fileOperationStore{path:"controlled-held-source",held:held}}
 updated,_:=model.Update(operatorBooted{Epoch:1,Discovery:discovery,Context:binding,Restore:operatorRestoration{Record:migrated,Raw:original,Migration:replacement,Legacy:true,Readback:&api.OperatorReadback{Operation:operation}}})
 actual:=updated.(operatorModel)
 if !bytes.Equal(actual.saved,replacement)||!bytes.Equal(held.raw,replacement)||actual.legacyRecord||held.publishes!=1||held.closes!=0{t.Fatal("migration caller retained stale preimage after observed applied replacement")}
 if !actual.custodyBlocked||actual.custodyWarning==""||len(actual.custodyErrors)!=1||!errors.Is(actual.custodyErrors[0],cause){t.Fatal("migration cleanup uncertainty was hidden by applied bytes")}
}