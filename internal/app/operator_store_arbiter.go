package app

import (
 "bytes"
 "crypto/sha256"
 "encoding/binary"
 "encoding/json"

 "io"
 "os"
 "reflect"
 "unicode/utf8"
)

// Private persistent arbitration data is distinct from the saved2/3 wire DTO.
// A valid ready record never proves the last exclusive native owner released.
const recordArbiterSlotBytes=65536
const recordArbiterHeaderBytes=48
const recordArbiterOwnerLimit=128
type recordArbiterPhase string
const(recordArbiterPrepared recordArbiterPhase="prepared";recordArbiterSettled recordArbiterPhase="settled";recordArbiterPoisoned recordArbiterPhase="poisoned";recordArbiterReady recordArbiterPhase="ready_to_release")
type recordArbiterCause struct{Kind string;Owner recordNativeIdentity;Message string}
type recordArbiterRecord struct{
 Version int `json:"version"`
 Generation uint64 `json:"generation"`
 Phase recordArbiterPhase `json:"phase"`
 Identity recordNativeIdentity `json:"identity"`
 ExclusionIdentity recordNativeIdentity `json:"exclusionIdentity"`
 OriginalKnown bool `json:"originalKnown"`
 Original []byte `json:"original"`
 Replacement []byte `json:"replacement"`
 Effect recordPublicationEffect `json:"effect"`
 Cleanup recordCleanupRelease `json:"cleanup"`
 Causes []recordArbiterCause `json:"causes"`
 IdentityMismatches []recordNativeIdentityMismatch `json:"identityMismatches"`
 BodyMismatches []recordNativeBodyMismatch `json:"bodyMismatches"`
 Writes []recordNativeWriteObservation `json:"writes"`
 Presence []recordNativePresenceObservation `json:"presence"`
}
type recordArbiter struct{file *os.File;identity,exclusionIdentity recordNativeIdentity;generation uint64;slot int;prior *recordArbiterRecord;closed bool;observed *recordStorageOutcome}
// Native object identity is not a descriptor number or mutable file length.
// Windows eligibility (non-directory/non-reparse/private security) is checked
// independently; VolumeSerialNumber + FileIndex identify the original object.
func sameRecordArbiterIdentity(a,b recordNativeIdentity)bool{if !a.IdentityKnown||!b.IdentityKnown||a.Platform!=b.Platform{return false};if a.Platform=="windows"{return a.Volume==b.Volume&&a.FileIndexHigh==b.FileIndexHigh&&a.FileIndexLow==b.FileIndexLow};return a.Device==b.Device&&a.Inode==b.Inode&&a.Mode==b.Mode}

func recordArbiterRecordValid(r recordArbiterRecord)bool{
 if r.Version!=1||r.Generation==0||r.Generation>9007199254740991||!r.Identity.IdentityKnown||!r.ExclusionIdentity.IdentityKnown||len(r.Original)>4096||len(r.Replacement)>4096||!r.OriginalKnown&&r.Original!=nil||r.Causes==nil||len(r.Causes)>recordArbiterOwnerLimit{return false}
 switch r.Phase{case recordArbiterPrepared,recordArbiterSettled,recordArbiterPoisoned,recordArbiterReady:default:return false}
 switch r.Effect{case recordUnchanged,recordReplacementApplied,recordClearApplied,recordEffectUnresolved:default:return false}
 if r.Cleanup!=recordReleased&&r.Cleanup!=recordReleaseUnresolved{return false};if r.Phase==recordArbiterReady&&(r.Effect==recordEffectUnresolved||r.Cleanup!=recordReleased){return false}
 for _,cause:=range r.Causes{if !utf8.ValidString(cause.Message)||len(cause.Message)>4096||len(cause.Kind)>128{return false}}
 if len(r.IdentityMismatches)+len(r.BodyMismatches)+len(r.Writes)+len(r.Presence)>recordArbiterOwnerLimit{return false};for _,mismatch:=range r.BodyMismatches{if len(mismatch.Expected)>4096||len(mismatch.Observed)>4097{return false}}
 return true
}
func recordArbiterSlot(raw []byte)(*recordArbiterRecord,bool,error){
 if len(raw)!=recordArbiterSlotBytes{return nil,false,storageError()};if bytes.Equal(raw,make([]byte,len(raw))){return nil,true,nil}
 if string(raw[:8])!="SLARC001"{return nil,false,storageError()};length:=int(binary.LittleEndian.Uint32(raw[8:12]));if length<=0||length>recordArbiterSlotBytes-recordArbiterHeaderBytes{return nil,false,storageError()}
 body:=raw[recordArbiterHeaderBytes:recordArbiterHeaderBytes+length];digest:=sha256.Sum256(body);if !bytes.Equal(digest[:],raw[16:48])||!bytes.Equal(raw[12:16],[]byte{0,0,0,0})||!bytes.Equal(raw[recordArbiterHeaderBytes+length:],make([]byte,recordArbiterSlotBytes-recordArbiterHeaderBytes-length)){return nil,false,storageError()}
 value,err:=recordArbiterJSON(body);if err!=nil{return nil,false,err};shapeOK,shapeErr:=recordArbiterShapeCause(value,reflect.TypeOf(recordArbiterRecord{}));if shapeErr!=nil{return nil,false,storageErrorCause(shapeErr)};if !shapeOK{return nil,false,storageError()};object,ok:=value.(map[string]any);if !ok||len(object)!=15{return nil,false,storageError()};for _,key:=range []string{"version","generation","phase","identity","exclusionIdentity","originalKnown","original","replacement","effect","cleanup","causes","identityMismatches","bodyMismatches","writes","presence"}{if _,exists:=object[key];!exists{return nil,false,storageError()}};if _,ok:=object["originalKnown"].(bool);!ok{return nil,false,storageError()};var record recordArbiterRecord;decoder:=json.NewDecoder(bytes.NewReader(body));decoder.DisallowUnknownFields();if err:=decoder.Decode(&record);err!=nil{return nil,false,err};if !recordArbiterRecordValid(record){return nil,false,storageError()};return &record,false,nil
}
// Called only after positively acquired same-file kernel exclusivity. Any torn
// nonempty slot remains uncertainty; the older valid slot is not a shortcut.
func loadRecordArbiter(file *os.File,identity,exclusionIdentity recordNativeIdentity,observations ...*recordStorageOutcome)(*recordArbiter,error){
 var observed *recordStorageOutcome;if len(observations)>0{observed=observations[0]}
 before,err:=recordFileStat(observed,file,identity);if err!=nil{return nil,err};if !before.Mode().IsRegular(){return nil,storageError()}
 arbiter:=&recordArbiter{file:file,identity:identity,exclusionIdentity:exclusionIdentity,slot:1,observed:observed};if before.Size()==0{return arbiter,nil};if before.Size()!=2*recordArbiterSlotBytes{return nil,storageError()}
 raw:=make([]byte,2*recordArbiterSlotBytes);if _,err:=recordReadFileAt(observed,file,identity,raw,0);err!=nil{return nil,err};after,err:=recordFileStat(observed,file,identity);if err!=nil{return nil,err};if !os.SameFile(before,after)||before.Size()!=after.Size()||before.ModTime()!=after.ModTime(){return nil,storageError()}
 first,firstEmpty,firstErr:=recordArbiterSlot(raw[:recordArbiterSlotBytes]);second,secondEmpty,secondErr:=recordArbiterSlot(raw[recordArbiterSlotBytes:]);if firstErr!=nil||secondErr!=nil{return nil,recordJoinedErrors(storageError(),firstErr,secondErr)}
 if firstEmpty&&secondEmpty{return arbiter,nil};selected:=first;slot:=0;if firstEmpty||!secondEmpty&&second.Generation>first.Generation{selected=second;slot=1};if !firstEmpty&&!secondEmpty&&(first.Generation==second.Generation||first.Generation+1!=second.Generation&&second.Generation+1!=first.Generation){return nil,storageError()}
 if !sameRecordArbiterIdentity(selected.Identity,identity)||!sameRecordArbiterIdentity(selected.ExclusionIdentity,exclusionIdentity)||selected.Phase!=recordArbiterReady{return nil,&recordPublicationUnresolved{}}
 arbiter.generation=selected.Generation;arbiter.slot=slot;arbiter.prior=selected;return arbiter,nil
}
func(a *recordArbiter)admitCurrent(raw []byte)error{
 if a.prior==nil{return nil};r:=a.prior;if !r.OriginalKnown{return &recordPublicationUnresolved{}};expected:=r.Original;switch r.Effect{case recordReplacementApplied:expected=r.Replacement;case recordClearApplied:expected=nil;case recordUnchanged:default:return &recordPublicationUnresolved{}};if !bytes.Equal(raw,expected){return &recordPublicationUnresolved{}};return nil
}
func(a *recordArbiter)writeRecord(record recordArbiterRecord)error{
 if a.closed||a.generation>=9007199254740991{return storageError()};record.Version=1;record.Generation=a.generation+1;record.Identity=a.identity;record.ExclusionIdentity=a.exclusionIdentity;if !recordArbiterRecordValid(record){return storageError()};body,err:=json.Marshal(record);if err!=nil{return err};if len(body)>recordArbiterSlotBytes-recordArbiterHeaderBytes{return storageError()}
 raw:=make([]byte,recordArbiterSlotBytes);copy(raw,"SLARC001");binary.LittleEndian.PutUint32(raw[8:12],uint32(len(body)));digest:=sha256.Sum256(body);copy(raw[16:48],digest[:]);copy(raw[recordArbiterHeaderBytes:],body)
 // Full-size zero initialization has no product record effects. A torn private
 // initialization is denied on restart instead of being accepted as empty.
 info,err:=recordFileStat(a.observed,a.file,a.identity);if err!=nil{return err};if info.Size()==0{if err:=recordFileTruncate(a.observed,a.file,a.identity,2*recordArbiterSlotBytes);err!=nil{return err};if err:=recordFileSync(a.observed,a.file,a.identity);err!=nil{return err}}
 next:=1-a.slot;written,writeErr:=recordWriteFile(a.observed,a.file,a.identity,raw,true,int64(next*recordArbiterSlotBytes));if writeErr!=nil{return writeErr};if written!=len(raw){return io.ErrShortWrite};if err:=recordFileSync(a.observed,a.file,a.identity);err!=nil{return err};observed:=make([]byte,len(raw));if _,err:=recordReadFileAt(a.observed,a.file,a.identity,observed,int64(next*recordArbiterSlotBytes));err!=nil{return err};if !bytes.Equal(raw,observed){return storageError()};a.generation=record.Generation;a.slot=next;a.prior=&record;return nil
}
func(a *recordArbiter)prepare(original,replacement []byte)error{return a.writeRecord(recordArbiterRecord{Phase:recordArbiterPrepared,OriginalKnown:true,Original:append([]byte(nil),original...),Replacement:append([]byte(nil),replacement...),Effect:recordUnchanged,Cleanup:recordReleased,Causes:[]recordArbiterCause{}})}
func recordArbiterSnapshot(outcome recordStorageOutcome)(recordArbiterRecord,error){
 record:=recordArbiterRecord{Phase:recordArbiterSettled,OriginalKnown:outcome.OriginalKnown,Original:append([]byte(nil),outcome.Original...),Replacement:append([]byte(nil),outcome.Replacement...),Effect:outcome.PublicationEffect,Cleanup:outcome.CleanupRelease,Causes:[]recordArbiterCause{}}
 record.IdentityMismatches=append([]recordNativeIdentityMismatch(nil),outcome.IdentityMismatches...);record.Writes=append([]recordNativeWriteObservation(nil),outcome.Writes...);record.Presence=append([]recordNativePresenceObservation(nil),outcome.Presence...);record.BodyMismatches=append([]recordNativeBodyMismatch(nil),outcome.BodyMismatches...);for index:=range record.BodyMismatches{record.BodyMismatches[index].Expected=append([]byte(nil),record.BodyMismatches[index].Expected...);record.BodyMismatches[index].Observed=append([]byte(nil),record.BodyMismatches[index].Observed...)}
 add:=func(kind string,owner recordNativeIdentity,cause error)error{if cause==nil{return nil};message:=cause.Error();if len(record.Causes)>=recordArbiterOwnerLimit||len(message)>4096||!utf8.ValidString(message){return storageError()};record.Causes=append(record.Causes,recordArbiterCause{Kind:kind,Owner:owner,Message:message});return nil}
 for _,call:=range outcome.NativeCalls{if original,ok:=call.(recordNativeCallOriginal);ok{if err:=add("native-call",original.nativeCallOwner(),original.nativeCallFailure());err!=nil{return record,err}}};for _,read:=range outcome.FileReads{if err:=add("file-read",read.Owner,read.Cause);err!=nil{return record,err}};for _,write:=range outcome.FileWrites{if err:=add("file-write",write.Owner,write.Cause);err!=nil{return record,err}};for _,helper:=range outcome.FileHelpers{if err:=add("file-"+helper.Kind,helper.Owner,helper.Cause);err!=nil{return record,err}};for _,failure:=range outcome.IdentityFailures{if err:=add("identity",failure.Owner,failure.Cause);err!=nil{return record,err}};for _,failure:=range outcome.HelperFailures{if err:=add("helper",failure.Owner,failure.Cause);err!=nil{return record,err}};for _,release:=range outcome.Releases{if err:=add("release",release.Owner,release.Cause);err!=nil{return record,err}}
 if !outcome.OriginalKnown||outcome.PublicationEffect==recordEffectUnresolved||outcome.CleanupRelease==recordReleaseUnresolved{record.Phase=recordArbiterPoisoned};return record,nil
}
