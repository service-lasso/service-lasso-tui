//go:build linux

package app

import("errors";"os";"golang.org/x/sys/unix")

func(s *linuxOperationStore)acquireArbiter()error{
 // The primary lock belongs to the original directory description, not the
 // replaceable custody pathname. All local writers acquire this first.
 if err:=unix.Flock(s.parent(),unix.LOCK_EX|unix.LOCK_NB);err!=nil{return s.retainHelperFailure("parent-exclusive-acquire",s.parent(),err,false)};s.primaryParentFD=s.parent()
 fd,err:=unix.Openat(s.parent(),s.name+".custody",unix.O_RDWR|unix.O_CREAT|unix.O_NOFOLLOW|unix.O_CLOEXEC,0600);if err!=nil{return s.retainHelperFailure("arbiter-open",s.parent(),err,false)}
 file:=os.NewFile(uintptr(fd),s.name+".custody");s.arbiterFile=file;var identity unix.Stat_t;if err:=unix.Fstat(fd,&identity);err!=nil{return s.retainHelperFailure("arbiter-identity",fd,err,true)};s.rememberNativeOwner(fd,identity)
 if identity.Mode&unix.S_IFMT!=unix.S_IFREG||identity.Mode&0777!=0600||identity.Uid!=uint32(unix.Geteuid()){return storageError()}
 if err:=unix.Flock(fd,unix.LOCK_EX|unix.LOCK_NB);err!=nil{return s.retainHelperFailure("arbiter-exclusive-acquire",fd,err,false)};s.arbiterExclusive=true
 arbiter,err:=loadRecordArbiter(file,linuxNativeIdentity("persistent-arbiter",fd,identity),s.nativeOwner("parent-arbitration",s.primaryParentFD),&s.observed);if err!=nil{return err};s.arbiter=arbiter
 if err:=unix.Fsync(s.parent());err!=nil{return s.retainHelperFailure("arbiter-parent-sync",s.parent(),err,true)};return nil
}
func(s *linuxOperationStore)namedArbiterMatches()bool{
 if s.arbiter==nil{return s.arbiterFile==nil};var named unix.Stat_t;if err:=unix.Fstatat(s.primaryParentFD,s.name+".custody",&named,unix.AT_SYMLINK_NOFOLLOW);err!=nil{s.retainHelperFailure("arbiter-named-identity",s.primaryParentFD,err,true);return false};actual:=linuxNativeIdentity("named-arbiter",s.primaryParentFD,named);if !sameRecordArbiterIdentity(s.arbiter.identity,actual){s.unresolved=true;s.observed.CleanupRelease=recordReleaseUnresolved;s.observed.IdentityMismatches=append(s.observed.IdentityMismatches,recordNativeIdentityMismatch{Owner:s.arbiter.identity,Observed:actual});return false};return true
}
func(s *linuxOperationStore)finishPrimaryParent()error{
 if s.primaryParentFD<0{return nil};fd:=s.primaryParentFD;if !s.namedArbiterMatches(){s.unresolved=true};if s.unresolved{if !s.primaryRetained{s.primaryRetained=true;s.observed.RetainedOwners=append(s.observed.RetainedOwners,recordNativeLifetime{Owner:s.nativeOwner("parent-arbitration",fd),Object:s})};return &recordPublicationUnresolved{}};s.primaryParentFD=-1;return s.closeDescriptor("parent-arbitration-last-release",fd)
}
func(s *linuxOperationStore)prepareArbiter(replacement []byte)error{
 if s.arbiter==nil||!s.arbiterExclusive{return storageError()};if err:=s.arbiter.admitCurrent(s.original);err!=nil{s.unresolved=true;s.observed.CleanupRelease=recordReleaseUnresolved;return err};if err:=s.arbiter.prepare(s.original,replacement);err!=nil{return s.retainHelperFailure("arbiter-prepare",int(s.arbiterFile.Fd()),err,true)};return nil
}
func(s *linuxOperationStore)validateArbiterCurrent(raw []byte)error{if s.arbiter==nil{return storageError()};if err:=s.arbiter.admitCurrent(raw);err!=nil{s.unresolved=true;s.observed.CleanupRelease=recordReleaseUnresolved;return err};return nil}
// The persistent arbiter is closed after every other original owner. Its file
// is never unlinked: a later same-file exclusive acquire proves last release.
func(s *linuxOperationStore)finishArbiter()error{
 if s.arbiterFile==nil{return nil};file:=s.arbiterFile;fd:=int(file.Fd());owner:=s.nativeOwner("persistent-arbiter",fd)
 if s.arbiter!=nil&&s.arbiterExclusive{
  record,snapshotErr:=recordArbiterSnapshot(s.storageOutcome());if snapshotErr==nil&&record.Phase!=recordArbiterPoisoned{record.Phase=recordArbiterReady};var writeErr error;if snapshotErr==nil{writeErr=s.arbiter.writeRecord(record)}
  if snapshotErr!=nil||writeErr!=nil{cause:=errors.Join(snapshotErr,writeErr);s.retainHelperFailure("arbiter-final-custody",fd,cause,true);s.observed.RetainedOwners=append(s.observed.RetainedOwners,recordNativeLifetime{Owner:owner,Object:file,Cause:cause});return recordJoinedErrors(&recordPublicationUnresolved{},cause)}
 }
 // An invalid prior journal is never rewritten just because its kernel owner
 // can be acquired. This newly acquired comparison owner is released once.
 s.arbiterFile=nil;if s.arbiter!=nil{s.arbiter.closed=true};cause:=s.closeFile("persistent-arbiter-last-release",file);return cause
}
