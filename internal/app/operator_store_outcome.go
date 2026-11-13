package app

import (
 "bytes"
 "encoding/json"
 "errors"
 "os"
 "io"
)

type recordPublicationEffect string
const(
 recordUnchanged recordPublicationEffect="unchanged"
 recordReplacementApplied recordPublicationEffect="replacement_applied"
 recordClearApplied recordPublicationEffect="clear_applied"
 recordEffectUnresolved recordPublicationEffect="unresolved"
)
type recordCleanupRelease string
const(recordReleased recordCleanupRelease="released";recordReleaseUnresolved recordCleanupRelease="unresolved")

// The original native identity and actual error stay private. A numeric handle
// whose Close failed is not a reusable authority and is never retried here.
type recordNativeIdentity struct{
 Platform,Kind string
 Handle uint64
 IdentityKnown bool
 Device,Inode,Volume,FileIndexHigh,FileIndexLow uint64
 Mode,Attributes uint64
 FileSizeHigh,FileSizeLow uint64
 LastWriteHigh,LastWriteLow uint64
 Size,ModifiedSeconds,ModifiedNanoseconds,ChangedSeconds,ChangedNanoseconds int64
}
type recordNativeRelease struct{Owner recordNativeIdentity;Cause error}
// Original file and once-owned observation remain private after Close failure.
// Repeated callers join this result; they never close a reused native value.
type recordFileClose struct{File *os.File;Owner recordNativeIdentity;Attempted,Completed bool;Cause error}
type recordFileRead struct{File *os.File;Owner recordNativeIdentity;Bound,Offset int64;ReadAt bool;Transferred int;Attempted,Completed bool;Raw []byte;Cause error}
type recordNativeCallOriginal interface{nativeCallOwner()recordNativeIdentity;nativeCallFailure()error}
type recordFileHelper struct{File *os.File;Owner recordNativeIdentity;Kind string;Size,Offset,Position int64;Whence int;Info os.FileInfo;Attempted,Completed bool;Cause error}
type recordFileWrite struct{File *os.File;Owner recordNativeIdentity;Offset int64;WriteAt bool;Transferred int;Attempted,Completed bool;Raw []byte;Cause error}
type recordNativeIdentityFailure struct{Owner recordNativeIdentity;Cause error}
type recordNativeHelperFailure struct{Owner recordNativeIdentity;Cause error}
type recordNativeIdentityMismatch struct{Owner,Observed recordNativeIdentity}
type recordNativeWriteObservation struct{Owner recordNativeIdentity;RequestedBytes,WrittenBytes int}
type recordNativeBodyMismatch struct{Owner recordNativeIdentity;Expected,Observed []byte}
type recordNativePresenceObservation struct{Owner,Observed recordNativeIdentity;ExpectedPresent,ObservedPresent bool}
type recordNativeLifetime struct{Owner recordNativeIdentity;Object any;CloseAttempted bool;Cause error}
type recordStorageOutcome struct{
 PublicationEffect recordPublicationEffect
 CleanupRelease recordCleanupRelease
 OriginalKnown bool
 Original,Replacement []byte
 NativeCalls []any
 FileHelpers []*recordFileHelper
 FileWrites []*recordFileWrite
 FileReads []*recordFileRead
 FileCloses []*recordFileClose
 Releases []recordNativeRelease
 IdentityFailures []recordNativeIdentityFailure
 HelperFailures []recordNativeHelperFailure
 IdentityMismatches []recordNativeIdentityMismatch
 BodyMismatches []recordNativeBodyMismatch
 Presence []recordNativePresenceObservation
 RetainedOwners []recordNativeLifetime
 Writes []recordNativeWriteObservation
}
func(o recordStorageOutcome)replacementApplied(raw []byte)bool{return o.PublicationEffect==recordReplacementApplied&&len(raw)>0&&bytes.Equal(o.Replacement,raw)}
func(o recordStorageOutcome)clearApplied(expected []byte)bool{return o.PublicationEffect==recordClearApplied&&o.OriginalKnown&&bytes.Equal(o.Original,expected)}

// An outcome is attached only by actual held-store observers. This type does
// not manufacture a durability receipt or implement native restart custody.
type recordStorageOutcomeError struct{Outcome recordStorageOutcome `json:"-"`;Primary,Cleanup error `json:"-"`}
// Ordinary serialization has the same fixed safe presentation as Error. The
// actual observed outcome, native owners and cause graph remain inspectable
// only through the internal object and Unwrap, never exported JSON fields.
func(e *recordStorageOutcomeError)MarshalJSON()([]byte,error){return json.Marshal(struct{Message string `json:"message"`}{Message:e.Error()})}
func(e *recordStorageOutcomeError)Error()string{
 if e.Outcome.PublicationEffect==recordEffectUnresolved{return "Local operation record publication is unresolved. Original recovery custody requires review."}
 if e.Outcome.PublicationEffect==recordReplacementApplied{if e.Outcome.CleanupRelease==recordReleaseUnresolved{return "Local operation record was replaced. Cleanup release remains unresolved."};return "Local operation record was replaced. A separate storage failure requires review."}
 if e.Outcome.PublicationEffect==recordClearApplied{if e.Outcome.CleanupRelease==recordReleaseUnresolved{return "Local operation record was removed. Cleanup release remains unresolved."};return "Local operation record was removed. A separate storage failure requires review."}
 if e.Outcome.CleanupRelease==recordReleaseUnresolved{return "Local operation record cleanup release is unresolved. The observed record effect and original owner are retained."}
 if !e.Outcome.OriginalKnown{return "Local operation record storage could not establish original record custody."}
 return "Local operation record could not be published. Original record is retained."
}
func(e *recordStorageOutcomeError)Unwrap()[]error{causes:=[]error{};if e.Primary!=nil{causes=append(causes,e.Primary)};if e.Cleanup!=nil{causes=append(causes,e.Cleanup)};for _,call:=range e.Outcome.NativeCalls{if original,ok:=call.(recordNativeCallOriginal);ok{if cause:=original.nativeCallFailure();cause!=nil{causes=append(causes,cause)}}};for _,helper:=range e.Outcome.FileHelpers{if helper.Cause!=nil{causes=append(causes,helper.Cause)}};for _,write:=range e.Outcome.FileWrites{if write.Cause!=nil{causes=append(causes,write.Cause)}};for _,read:=range e.Outcome.FileReads{if read.Cause!=nil{causes=append(causes,read.Cause)}};for _,failure:=range e.Outcome.IdentityFailures{if failure.Cause!=nil{causes=append(causes,failure.Cause)}};for _,failure:=range e.Outcome.HelperFailures{if failure.Cause!=nil{causes=append(causes,failure.Cause)}};for _,failure:=range e.Outcome.Releases{if failure.Cause!=nil{causes=append(causes,failure.Cause)}};for _,owner:=range e.Outcome.RetainedOwners{if owner.Cause!=nil{causes=append(causes,owner.Cause)}};return causes}
func recordOutcome(err error)(recordStorageOutcome,bool){var observed *recordStorageOutcomeError;if !errors.As(err,&observed){return recordStorageOutcome{},false};return observed.Outcome,true}

// Joining native failures must not publish their Error strings. The actual
// joined graph remains available to custody inspection, including nil success.
type recordJoinedError struct{cause error}
func recordJoinedErrors(causes ...error)error{joined:=errors.Join(causes...);if joined==nil{return nil};return &recordJoinedError{cause:joined}}
func(e *recordJoinedError)Unwrap()error{return e.cause}
func(e *recordJoinedError)Error()string{var outcome *recordStorageOutcomeError;var unresolved *recordPublicationUnresolved;hasUnresolved:=errors.As(e.cause,&unresolved);if errors.As(e.cause,&outcome){if hasUnresolved&&outcome.Outcome.PublicationEffect!=recordEffectUnresolved&&outcome.Outcome.CleanupRelease!=recordReleaseUnresolved{return outcome.Error()+" "+unresolved.Error()};return outcome.Error()};if hasUnresolved{return unresolved.Error()};return storageError().Error()}
func(e *recordJoinedError)MarshalJSON()([]byte,error){return json.Marshal(struct{Message string `json:"message"`}{Message:e.Error()})}

// Preserve the actual io.ReadAll slice and error before any identity check or
// cleanup can refuse it. Bound is the existing caller's read extent, not a new
// output quota, EOF claim or complete-runtime allocation grant.
func recordReadFile(outcome *recordStorageOutcome,file *os.File,owner recordNativeIdentity,bound int64)([]byte,error){
 observation:=&recordFileRead{File:file,Owner:owner,Bound:bound}
 outcome.FileReads=append(outcome.FileReads,observation)
 if !owner.IdentityKnown{cause:=storageError();observation.Cause=cause;return nil,cause}
 observation.Attempted=true
 raw,cause:=io.ReadAll(io.LimitReader(file,bound))
 observation.Raw=raw;observation.Transferred=len(raw);observation.Cause=cause;observation.Completed=true
 return raw,cause
}
func recordReadFileAt(outcome *recordStorageOutcome,file *os.File,owner recordNativeIdentity,raw []byte,offset int64)(int,error){
 if outcome==nil{return file.ReadAt(raw,offset)}
 observation:=&recordFileRead{File:file,Owner:owner,Bound:int64(len(raw)),Raw:raw,Offset:offset,ReadAt:true}
 outcome.FileReads=append(outcome.FileReads,observation);if !owner.IdentityKnown{cause:=storageError();observation.Cause=cause;return 0,cause};observation.Attempted=true
 n,cause:=file.ReadAt(raw,offset);observation.Transferred=n;observation.Cause=cause;observation.Completed=true
 return n,cause
}
func recordWriteFile(outcome *recordStorageOutcome,file *os.File,owner recordNativeIdentity,raw []byte,at bool,offset int64)(int,error){
 if outcome==nil{if at{return file.WriteAt(raw,offset)};return file.Write(raw)}
 observation:=&recordFileWrite{File:file,Owner:owner,Raw:raw,WriteAt:at,Offset:offset}
 outcome.FileWrites=append(outcome.FileWrites,observation);if !owner.IdentityKnown{cause:=storageError();observation.Cause=cause;return 0,cause};observation.Attempted=true
 var n int;var cause error;if at{n,cause=file.WriteAt(raw,offset)}else{n,cause=file.Write(raw)}
 observation.Transferred=n;observation.Cause=cause;observation.Completed=true
 return n,cause
}
// Keep the actual metadata object and original helper result with the same
// original file. A failed identity query never authorizes a new helper effect.
func recordFileStat(outcome *recordStorageOutcome,file *os.File,owner recordNativeIdentity)(os.FileInfo,error){
 if outcome==nil{return file.Stat()}
 observation:=&recordFileHelper{File:file,Owner:owner,Kind:"stat"};outcome.FileHelpers=append(outcome.FileHelpers,observation)
 if !owner.IdentityKnown{cause:=storageError();observation.Cause=cause;return nil,cause};observation.Attempted=true
 info,cause:=file.Stat();observation.Info=info;observation.Cause=cause;observation.Completed=true;return info,cause
}
func recordFileSync(outcome *recordStorageOutcome,file *os.File,owner recordNativeIdentity)error{
 if outcome==nil{return file.Sync()}
 observation:=&recordFileHelper{File:file,Owner:owner,Kind:"sync"};outcome.FileHelpers=append(outcome.FileHelpers,observation)
 if !owner.IdentityKnown{cause:=storageError();observation.Cause=cause;return cause};observation.Attempted=true
 cause:=file.Sync();observation.Cause=cause;observation.Completed=true;return cause
}
func recordFileTruncate(outcome *recordStorageOutcome,file *os.File,owner recordNativeIdentity,size int64)error{
 if outcome==nil{return file.Truncate(size)}
 observation:=&recordFileHelper{File:file,Owner:owner,Kind:"truncate",Size:size};outcome.FileHelpers=append(outcome.FileHelpers,observation)
 if !owner.IdentityKnown{cause:=storageError();observation.Cause=cause;return cause};observation.Attempted=true
 cause:=file.Truncate(size);observation.Cause=cause;observation.Completed=true;return cause
}
func recordFileSeek(outcome *recordStorageOutcome,file *os.File,owner recordNativeIdentity,offset int64,whence int)(int64,error){
 if outcome==nil{return file.Seek(offset,whence)}
 observation:=&recordFileHelper{File:file,Owner:owner,Kind:"seek",Offset:offset,Whence:whence};outcome.FileHelpers=append(outcome.FileHelpers,observation)
 if !owner.IdentityKnown{cause:=storageError();observation.Cause=cause;return 0,cause};observation.Attempted=true
 position,cause:=file.Seek(offset,whence);observation.Position=position;observation.Cause=cause;observation.Completed=true;return position,cause
}