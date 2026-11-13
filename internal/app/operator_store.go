package app

import (
 "bytes"
 "encoding/json"
 "errors"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

type heldOperationStore interface {
 read()([]byte,error)
 publish([]byte)error
 remove()error
 close()error
 retainOwnership()
}
type heldRecordOutcome interface{storageOutcome()recordStorageOutcome}
type heldNativeCleanupOutcome interface{nativeCleanupOutcome()recordStorageOutcome}
// A recovery failure after an applied rename is distinct from an untouched
// Save failure. Old preimage custody is retained privately; the warning must
// remain visible and no old-record restoration or successful clear is claimed.
type recordPublicationUnresolved struct{}
func(*recordPublicationUnresolved)Error()string{return "Local operation record custody is unresolved. Retained recovery evidence requires review."}
type recordRecovery struct{Version int `json:"version"`;Kind string `json:"kind"`;Original []byte `json:"original"`;Replacement []byte `json:"replacement"`;BackupName string `json:"backupName"`;PreparedName string `json:"preparedName"`}
func recoveryBytes(kind string,original,replacement []byte,backup,prepared string)([]byte,error){raw,err:=json.Marshal(recordRecovery{1,kind,original,replacement,backup,prepared});if err!=nil{return nil,storageErrorCause(err)};if len(raw)>16384{return nil,storageError()};return raw,nil}
func storageError()error{return &api.OperatorError{Stage:"storage"}}
func storageErrorCause(cause error)error{return api.OperatorStorageCause(cause)}
func(s fileOperationStore) publication(action func(fileOperationStore)error)error{
 if s.path==""{return storageError()}
 if s.held!=nil{return observedHeldRecordFailure(s.held,action(s))}
 held,err:=acquireOperationStore(s.path);if err!=nil{return recordJoinedErrors(err)}
 s.held=held
 err=action(s);var unresolved *recordPublicationUnresolved;if errors.As(err,&unresolved){held.retainOwnership()};var observation func()recordStorageOutcome;if observer,ok:=held.(heldRecordOutcome);ok{observation=observer.storageOutcome}else if observer,ok:=held.(heldNativeCleanupOutcome);ok{observation=observer.nativeCleanupOutcome};if observation!=nil{outcome:=observation();if outcome.PublicationEffect==recordEffectUnresolved||outcome.CleanupRelease==recordReleaseUnresolved{held.retainOwnership()}}
 closeErr:=held.close();if observation!=nil{outcome:=observation();if err!=nil||closeErr!=nil||outcome.CleanupRelease==recordReleaseUnresolved||outcome.PublicationEffect==recordEffectUnresolved{return &recordStorageOutcomeError{Outcome:outcome,Primary:err,Cleanup:closeErr}};return nil};if closeErr!=nil{return recordJoinedErrors(err,&recordPublicationUnresolved{},closeErr)};return recordJoinedErrors(err)
}
// A supplied held owner stays with its caller. Returning from a nested helper
// does not release it, but its actual effect/release observations must survive.
func observedHeldRecordFailure(held heldOperationStore,primary error)error{
 var outcome recordStorageOutcome
 observed:=false
 if owner,ok:=held.(heldRecordOutcome);ok{outcome=owner.storageOutcome();observed=true}else if owner,ok:=held.(heldNativeCleanupOutcome);ok{outcome=owner.nativeCleanupOutcome();observed=true}
 if !observed{return recordJoinedErrors(primary)}
 if outcome.PublicationEffect==recordEffectUnresolved||outcome.CleanupRelease==recordReleaseUnresolved{held.retainOwnership()}
 if primary!=nil||outcome.PublicationEffect==recordEffectUnresolved||outcome.CleanupRelease==recordReleaseUnresolved{return &recordStorageOutcomeError{Outcome:outcome,Primary:primary}}
 return nil
}
func(s fileOperationStore) readRaw()([]byte,error){
 if s.held!=nil{raw,err:=s.held.read();return raw,observedHeldRecordFailure(s.held,err)}
 var raw []byte
 err:=s.publication(func(owned fileOperationStore)error{var err error;raw,err=owned.held.read();return err});return raw,err
}
func(s fileOperationStore) publishRaw(raw []byte)error{
 if s.held==nil||len(raw)==0||len(raw)>4096{return storageError()};return observedHeldRecordFailure(s.held,s.held.publish(raw))
}
func(s fileOperationStore) replaceRaw(expected,replacement []byte)error{
 if _,err:=api.DecodeOperatorSavedV3(replacement);err!=nil{return storageErrorCause(err)}
 return s.publication(func(owned fileOperationStore)error{
  current,err:=owned.readRaw();if err!=nil{return err};if !bytes.Equal(current,expected){return storageError()};return owned.publishRaw(replacement)
 })
}
func(s fileOperationStore) clearRaw(expected []byte)error{
 return s.publication(func(owned fileOperationStore)error{
  current,err:=owned.readRaw();if err!=nil{return err};if current==nil{return nil};if !bytes.Equal(current,expected){return storageError()};return owned.held.remove()
 })
}
func savedV3Bytes(value api.OperatorSavedV3)([]byte,error){
 raw,err:=json.Marshal(value);if err!=nil{return nil,storageErrorCause(err)};if _,err=api.DecodeOperatorSavedV3(raw);err!=nil{return nil,storageErrorCause(err)};return raw,nil
}
