//go:build windows

package app


import (

 "os"

 "golang.org/x/sys/windows"
)

func(s *windowsOperationStore)observeWrite(kind string,handle windows.Handle,requested,written int){
 owner:=recordNativeIdentity{Platform:"windows",Kind:kind,Handle:uint64(handle)};if known,exists:=s.nativeOwners[handle];exists{owner=known;owner.Kind=kind};s.observed.Writes=append(s.observed.Writes,recordNativeWriteObservation{Owner:owner,RequestedBytes:requested,WrittenBytes:written})
}

type windowsInformationObservation struct{Owner recordNativeIdentity;Handle windows.Handle;Information windows.ByHandleFileInformation;Attempted,Completed bool;Cause error}
func(o *windowsInformationObservation)nativeCallOwner()recordNativeIdentity{return o.Owner}
func(o *windowsInformationObservation)nativeCallFailure()error{return o.Cause}
func(s *windowsOperationStore)observeInformation(kind string,handle windows.Handle)*windowsInformationObservation{
 observation:=&windowsInformationObservation{Handle:handle,Owner:recordNativeIdentity{Platform:"windows",Kind:kind,Handle:uint64(handle)}}
 if prior,exists:=s.nativeOwners[handle];exists{observation.Owner=prior;observation.Owner.Kind=kind}
 s.observed.NativeCalls=append(s.observed.NativeCalls,observation);observation.Attempted=true
 observation.Cause=windows.GetFileInformationByHandle(handle,&observation.Information);observation.Completed=true;return observation
}
func(s *windowsOperationStore)info(handle windows.Handle)(windows.ByHandleFileInformation,error){
 observation:=s.observeInformation("identity-query",handle);identity,cause:=observation.Information,observation.Cause
 if cause!=nil{owner:=recordNativeIdentity{Platform:"windows",Kind:"identity-query",Handle:uint64(handle)};if prior,exists:=s.nativeOwners[handle];exists{owner=prior;owner.Kind="identity-query"};s.unresolved=true;s.observed.CleanupRelease=recordReleaseUnresolved;s.observed.IdentityFailures=append(s.observed.IdentityFailures,recordNativeIdentityFailure{Owner:owner,Cause:cause});return identity,recordJoinedErrors(storageError(),cause)}
 s.rememberNativeOwner(handle,identity);if identity.FileAttributes&windows.FILE_ATTRIBUTE_REPARSE_POINT!=0{return identity,storageError()};return identity,nil
}

func(s *windowsOperationStore)rememberNativeOwner(handle windows.Handle,identity windows.ByHandleFileInformation){if s.nativeOwners==nil{s.nativeOwners=map[windows.Handle]recordNativeIdentity{}};s.nativeOwners[handle]=recordNativeIdentity{Platform:"windows",Handle:uint64(handle),IdentityKnown:true,Volume:uint64(identity.VolumeSerialNumber),FileIndexHigh:uint64(identity.FileIndexHigh),FileIndexLow:uint64(identity.FileIndexLow),Attributes:uint64(identity.FileAttributes)}}
func(s *windowsOperationStore)nativeOwner(kind string,handle windows.Handle)recordNativeIdentity{
 owner:=recordNativeIdentity{Platform:"windows",Kind:kind,Handle:uint64(handle)};if prior,exists:=s.nativeOwners[handle];exists{owner=prior;owner.Kind=kind};observation:=s.observeInformation(kind,handle);identity,err:=observation.Information,observation.Cause
 if err==nil{s.rememberNativeOwner(handle,identity);owner=s.nativeOwners[handle];owner.Kind=kind}else{s.unresolved=true;s.observed.CleanupRelease=recordReleaseUnresolved;s.observed.IdentityFailures=append(s.observed.IdentityFailures,recordNativeIdentityFailure{Owner:owner,Cause:err});owner.IdentityKnown=false};return owner
}
func(s *windowsOperationStore)retainRelease(owner recordNativeIdentity,cause error)error{s.observed.Releases=append(s.observed.Releases,recordNativeRelease{Owner:owner,Cause:cause});if cause==nil{return nil};s.unresolved=true;s.observed.CleanupRelease=recordReleaseUnresolved;return cause}
func(s *windowsOperationStore)retainHelperFailure(kind string,handle windows.Handle,cause error,unresolved bool)error{if cause==nil{return nil};owner:=recordNativeIdentity{Platform:"windows",Kind:kind,Handle:uint64(handle)};if known,exists:=s.nativeOwners[handle];exists{owner=known;owner.Kind=kind};s.observed.HelperFailures=append(s.observed.HelperFailures,recordNativeHelperFailure{Owner:owner,Cause:cause});if unresolved{s.unresolved=true;s.observed.CleanupRelease=recordReleaseUnresolved};return cause}
func(s *windowsOperationStore)retainIdentityMismatch(kind string,handle windows.Handle,original,observed windows.ByHandleFileInformation){owner:=recordNativeIdentity{Platform:"windows",Kind:kind,Handle:uint64(handle),IdentityKnown:true,Volume:uint64(original.VolumeSerialNumber),FileIndexHigh:uint64(original.FileIndexHigh),FileIndexLow:uint64(original.FileIndexLow),Attributes:uint64(original.FileAttributes),FileSizeHigh:uint64(original.FileSizeHigh),FileSizeLow:uint64(original.FileSizeLow),LastWriteHigh:uint64(original.LastWriteTime.HighDateTime),LastWriteLow:uint64(original.LastWriteTime.LowDateTime)};actual:=recordNativeIdentity{Platform:"windows",Kind:kind,Handle:uint64(handle),IdentityKnown:true,Volume:uint64(observed.VolumeSerialNumber),FileIndexHigh:uint64(observed.FileIndexHigh),FileIndexLow:uint64(observed.FileIndexLow),Attributes:uint64(observed.FileAttributes),FileSizeHigh:uint64(observed.FileSizeHigh),FileSizeLow:uint64(observed.FileSizeLow),LastWriteHigh:uint64(observed.LastWriteTime.HighDateTime),LastWriteLow:uint64(observed.LastWriteTime.LowDateTime)};s.observed.IdentityMismatches=append(s.observed.IdentityMismatches,recordNativeIdentityMismatch{Owner:owner,Observed:actual});s.unresolved=true;s.observed.CleanupRelease=recordReleaseUnresolved}
func(s *windowsOperationStore)closeHandle(kind string,handle windows.Handle)error{owner:=s.nativeOwner(kind,handle);cause:=windows.CloseHandle(handle);delete(s.nativeOwners,handle);if cause!=nil{s.observed.RetainedOwners=append(s.observed.RetainedOwners,recordNativeLifetime{Owner:owner,Object:s,CloseAttempted:true,Cause:cause})};return s.retainRelease(owner,cause)}
func(s *windowsOperationStore)closeFile(kind string,file *os.File)error{
 for _,prior:=range s.observed.FileCloses{if prior.File==file{if !prior.Completed{return &recordPublicationUnresolved{}};return prior.Cause}}
 observation:=&recordFileClose{File:file};s.observed.FileCloses=append(s.observed.FileCloses,observation)
 handle:=windows.Handle(file.Fd());observation.Owner=s.nativeOwner(kind,handle)
 observation.Attempted=true;cause:=file.Close();observation.Cause=cause;observation.Completed=true
 delete(s.nativeOwners,handle)
 if cause!=nil{s.observed.RetainedOwners=append(s.observed.RetainedOwners,recordNativeLifetime{Owner:observation.Owner,Object:file,CloseAttempted:true,Cause:cause})}
 return s.retainRelease(observation.Owner,cause)
}
// The shared observer retains unresolved Windows effects and actual native
// cleanup observations. It admits no applied-record receipt before actual
// namespace request durability has been established.
func(s *windowsOperationStore)nativeCleanupOutcome()recordStorageOutcome{outcome:=s.observed;outcome.NativeCalls=append([]any(nil),outcome.NativeCalls...);outcome.FileHelpers=append([]*recordFileHelper(nil),s.observed.FileHelpers...);outcome.FileWrites=append([]*recordFileWrite(nil),outcome.FileWrites...);outcome.FileReads=append([]*recordFileRead(nil),outcome.FileReads...);outcome.FileCloses=append([]*recordFileClose(nil),outcome.FileCloses...);outcome.Original=append([]byte(nil),outcome.Original...);outcome.Replacement=append([]byte(nil),outcome.Replacement...);outcome.Releases=append([]recordNativeRelease(nil),outcome.Releases...);outcome.IdentityFailures=append([]recordNativeIdentityFailure(nil),outcome.IdentityFailures...);outcome.HelperFailures=append([]recordNativeHelperFailure(nil),outcome.HelperFailures...);outcome.IdentityMismatches=append([]recordNativeIdentityMismatch(nil),outcome.IdentityMismatches...);outcome.Writes=append([]recordNativeWriteObservation(nil),outcome.Writes...);outcome.RetainedOwners=append([]recordNativeLifetime(nil),outcome.RetainedOwners...);outcome.Presence=append([]recordNativePresenceObservation(nil),outcome.Presence...);outcome.BodyMismatches=append([]recordNativeBodyMismatch(nil),outcome.BodyMismatches...);for index:=range outcome.BodyMismatches{outcome.BodyMismatches[index].Expected=append([]byte(nil),outcome.BodyMismatches[index].Expected...);outcome.BodyMismatches[index].Observed=append([]byte(nil),outcome.BodyMismatches[index].Observed...)};return outcome}
