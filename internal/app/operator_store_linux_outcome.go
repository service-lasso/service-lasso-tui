//go:build linux

package app

import (
 "os"

 "golang.org/x/sys/unix"
)

func(s *linuxOperationStore)observeWrite(kind string,descriptor int,requested,written int){
 owner:=recordNativeIdentity{Platform:"linux",Kind:kind,Handle:uint64(descriptor)};if known,exists:=s.nativeOwners[descriptor];exists{owner=known;owner.Kind=kind};s.observed.Writes=append(s.observed.Writes,recordNativeWriteObservation{Owner:owner,RequestedBytes:requested,WrittenBytes:written})
}
func linuxNativeIdentity(kind string,descriptor int,stat unix.Stat_t)recordNativeIdentity{return recordNativeIdentity{Platform:"linux",Kind:kind,Handle:uint64(descriptor),IdentityKnown:true,Device:uint64(stat.Dev),Inode:stat.Ino,Mode:uint64(stat.Mode),Size:stat.Size,ModifiedSeconds:stat.Mtim.Sec,ModifiedNanoseconds:stat.Mtim.Nsec,ChangedSeconds:stat.Ctim.Sec,ChangedNanoseconds:stat.Ctim.Nsec}}
func(s *linuxOperationStore)retainIdentityMismatch(kind string,descriptor int,expected,observed unix.Stat_t){s.unresolved=true;s.observed.CleanupRelease=recordReleaseUnresolved;s.observed.IdentityMismatches=append(s.observed.IdentityMismatches,recordNativeIdentityMismatch{Owner:linuxNativeIdentity(kind,descriptor,expected),Observed:linuxNativeIdentity(kind,descriptor,observed)})}

func(s *linuxOperationStore)nativeOwner(kind string,descriptor int)recordNativeIdentity{
 owner:=recordNativeIdentity{Platform:"linux",Kind:kind,Handle:uint64(descriptor)};if prior,exists:=s.nativeOwners[descriptor];exists{owner=prior;owner.Kind=kind};var identity unix.Stat_t
 if err:=unix.Fstat(descriptor,&identity);err==nil{owner=linuxNativeIdentity(kind,descriptor,identity);s.rememberNativeOwner(descriptor,identity)}else{s.unresolved=true;s.observed.CleanupRelease=recordReleaseUnresolved;s.observed.IdentityFailures=append(s.observed.IdentityFailures,recordNativeIdentityFailure{Owner:owner,Cause:err});owner.IdentityKnown=false};return owner
}
func(s *linuxOperationStore)rememberNativeOwner(descriptor int,identity unix.Stat_t){if s.nativeOwners==nil{s.nativeOwners=map[int]recordNativeIdentity{}};s.nativeOwners[descriptor]=linuxNativeIdentity("",descriptor,identity)}
func(s *linuxOperationStore)retainRelease(owner recordNativeIdentity,cause error)error{
 s.observed.Releases=append(s.observed.Releases,recordNativeRelease{Owner:owner,Cause:cause});if cause==nil{return nil};s.unresolved=true;s.observed.CleanupRelease=recordReleaseUnresolved;return cause
}
func(s *linuxOperationStore)retainHelperFailure(kind string,descriptor int,cause error,unresolved bool)error{if cause==nil{return nil};owner:=recordNativeIdentity{Platform:"linux",Kind:kind,Handle:uint64(descriptor)};if known,exists:=s.nativeOwners[descriptor];exists{owner=known;owner.Kind=kind};s.observed.HelperFailures=append(s.observed.HelperFailures,recordNativeHelperFailure{Owner:owner,Cause:cause});if unresolved{s.unresolved=true;s.observed.CleanupRelease=recordReleaseUnresolved};return cause}
func(s *linuxOperationStore)closeDescriptor(kind string,descriptor int)error{owner:=s.nativeOwner(kind,descriptor);cause:=unix.Close(descriptor);delete(s.nativeOwners,descriptor);if cause!=nil{s.observed.RetainedOwners=append(s.observed.RetainedOwners,recordNativeLifetime{Owner:owner,Object:s,CloseAttempted:true,Cause:cause})};return s.retainRelease(owner,cause)}
func(s *linuxOperationStore)closeFile(kind string,file *os.File)error{
 for _,prior:=range s.observed.FileCloses{if prior.File==file{if !prior.Completed{return &recordPublicationUnresolved{}};return prior.Cause}}
 observation:=&recordFileClose{File:file};s.observed.FileCloses=append(s.observed.FileCloses,observation)
 descriptor:=int(file.Fd());observation.Owner=s.nativeOwner(kind,descriptor)
 observation.Attempted=true;cause:=file.Close();observation.Cause=cause;observation.Completed=true
 delete(s.nativeOwners,descriptor)
 if cause!=nil{s.observed.RetainedOwners=append(s.observed.RetainedOwners,recordNativeLifetime{Owner:observation.Owner,Object:file,CloseAttempted:true,Cause:cause})}
 return s.retainRelease(observation.Owner,cause)
}
func(s *linuxOperationStore)storageOutcome()recordStorageOutcome{outcome:=s.observed;outcome.NativeCalls=append([]any(nil),outcome.NativeCalls...);outcome.FileHelpers=append([]*recordFileHelper(nil),s.observed.FileHelpers...);outcome.FileWrites=append([]*recordFileWrite(nil),outcome.FileWrites...);outcome.FileReads=append([]*recordFileRead(nil),outcome.FileReads...);outcome.FileCloses=append([]*recordFileClose(nil),outcome.FileCloses...);outcome.Original=append([]byte(nil),outcome.Original...);outcome.Replacement=append([]byte(nil),outcome.Replacement...);outcome.Releases=append([]recordNativeRelease(nil),outcome.Releases...);outcome.IdentityFailures=append([]recordNativeIdentityFailure(nil),outcome.IdentityFailures...);outcome.HelperFailures=append([]recordNativeHelperFailure(nil),outcome.HelperFailures...);outcome.IdentityMismatches=append([]recordNativeIdentityMismatch(nil),outcome.IdentityMismatches...);outcome.Writes=append([]recordNativeWriteObservation(nil),outcome.Writes...);outcome.RetainedOwners=append([]recordNativeLifetime(nil),outcome.RetainedOwners...);outcome.Presence=append([]recordNativePresenceObservation(nil),outcome.Presence...);outcome.BodyMismatches=append([]recordNativeBodyMismatch(nil),outcome.BodyMismatches...);for index:=range outcome.BodyMismatches{outcome.BodyMismatches[index].Expected=append([]byte(nil),outcome.BodyMismatches[index].Expected...);outcome.BodyMismatches[index].Observed=append([]byte(nil),outcome.BodyMismatches[index].Observed...)};return outcome}
