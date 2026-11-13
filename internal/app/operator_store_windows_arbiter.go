//go:build windows

package app

import("errors";"os";"golang.org/x/sys/windows")

func(s *windowsOperationStore)acquireArbiter()error{
 name:=s.name+".custody";access:=uint32(windows.GENERIC_READ|windows.GENERIC_WRITE)
 handle,err:=s.child(s.parent(),name,false,false,access,0)
 if err==windows.ERROR_FILE_NOT_FOUND||err==windows.STATUS_OBJECT_NAME_NOT_FOUND{handle,err=s.child(s.parent(),name,false,true,access,0)}
 if err!=nil{return s.retainHelperFailure("arbiter-exclusive-acquire",s.parent(),err,false)}
 file:=os.NewFile(uintptr(handle),name);s.arbiterFile=file;s.arbiterExclusive=true;identity,err:=s.info(handle);if err!=nil{return err}
 if identity.FileAttributes&windows.FILE_ATTRIBUTE_DIRECTORY!=0{return storageError()}
 owner:=s.nativeOwner("persistent-arbiter",handle);if s.unresolved{return &recordPublicationUnresolved{}};arbiter,err:=loadRecordArbiter(file,owner,owner,&s.observed);if err!=nil{return err};s.arbiter=arbiter;return nil
}
func(s *windowsOperationStore)prepareArbiter(replacement []byte)error{
 if err:=s.namedArbiterMatches();err!=nil{return err};if s.unresolved{return &recordPublicationUnresolved{}}
 if s.arbiter==nil||!s.arbiterExclusive{return storageError()};if err:=s.arbiter.admitCurrent(s.original);err!=nil{s.unresolved=true;s.observed.CleanupRelease=recordReleaseUnresolved;return err};if err:=s.arbiter.prepare(s.original,replacement);err!=nil{return s.retainHelperFailure("arbiter-prepare",windows.Handle(s.arbiterFile.Fd()),err,true)};return nil
}
func(s *windowsOperationStore)validateArbiterCurrent(raw []byte)error{if s.arbiter==nil{return storageError()};if err:=s.namedArbiterMatches();err!=nil{return err};if s.unresolved{return &recordPublicationUnresolved{}};if err:=s.arbiter.admitCurrent(raw);err!=nil{s.unresolved=true;s.observed.CleanupRelease=recordReleaseUnresolved;return err};return nil}
func(s *windowsOperationStore)finishArbiter()error{
 if s.arbiterFile==nil{return nil};file:=s.arbiterFile;handle:=windows.Handle(file.Fd());owner:=s.nativeOwner("persistent-arbiter",handle)
 var comparisonErr error;if s.arbiter!=nil{comparisonErr=s.namedArbiterMatches();if comparisonErr!=nil{s.unresolved=true;s.observed.CleanupRelease=recordReleaseUnresolved}}
 if s.arbiter!=nil&&s.arbiterExclusive{
  record,snapshotErr:=recordArbiterSnapshot(s.nativeCleanupOutcome());if snapshotErr==nil&&record.Phase!=recordArbiterPoisoned{record.Phase=recordArbiterReady};var writeErr error;if snapshotErr==nil{writeErr=s.arbiter.writeRecord(record)}
  if snapshotErr!=nil||writeErr!=nil{cause:=errors.Join(comparisonErr,snapshotErr,writeErr);s.retainHelperFailure("arbiter-final-custody",handle,cause,true);s.observed.RetainedOwners=append(s.observed.RetainedOwners,recordNativeLifetime{Owner:owner,Object:file,Cause:cause});return recordJoinedErrors(&recordPublicationUnresolved{},cause)}
 }
 s.arbiterFile=nil;if s.arbiter!=nil{s.arbiter.closed=true};cause:=s.closeFile("persistent-arbiter-last-release",file);return recordJoinedErrors(comparisonErr,cause)
}

// The exclusive directory is acquired before the marker. It remains the
// original namespace owner while marker comparisons and final journal writes
// run; no pathname reopen can replace that authority.
func(s *windowsOperationStore)namedArbiterMatches()error{
 if s.exclusiveParent==0||s.arbiterFile==nil||s.arbiter==nil{return storageError()}
 parentIdentity,err:=s.info(s.exclusiveParent);if err!=nil{return err}
 if !winSame(s.exclusiveParentIdentity,parentIdentity){s.retainIdentityMismatch("arbiter-parent-comparison",s.exclusiveParent,s.exclusiveParentIdentity,parentIdentity);return storageError()}
 if parentIdentity.FileAttributes&windows.FILE_ATTRIBUTE_DIRECTORY==0{return s.retainHelperFailure("arbiter-parent-kind",s.exclusiveParent,storageError(),true)}
 handle,err:=s.child(s.exclusiveParent,s.name+".custody",false,false,windows.FILE_READ_ATTRIBUTES,windows.FILE_SHARE_READ|windows.FILE_SHARE_WRITE|windows.FILE_SHARE_DELETE)
 if err!=nil{return s.retainHelperFailure("arbiter-named-comparison",s.exclusiveParent,err,true)}
 named,identityErr:=s.info(handle);held,heldErr:=s.info(windows.Handle(s.arbiterFile.Fd()));var mismatch error
 if identityErr==nil&&heldErr==nil&&!winSame(named,held){s.retainIdentityMismatch("arbiter-named-comparison",handle,held,named);mismatch=storageError()}
 return recordJoinedErrors(identityErr,heldErr,mismatch,s.closeHandle("arbiter-named-comparison",handle))
}

func(s *windowsOperationStore)finishExclusiveParent()error{
 if s.exclusiveParent==0{return nil};handle:=s.exclusiveParent;owner:=s.nativeOwner("exclusive-parent-last-release",handle)
 if s.unresolved{s.observed.RetainedOwners=append(s.observed.RetainedOwners,recordNativeLifetime{Owner:owner,Object:s,Cause:&recordPublicationUnresolved{}});return &recordPublicationUnresolved{}}
 // Exactly one attempt; failure does not authorize retrying a numeric handle.
 s.exclusiveParent=0;cause:=s.closeHandle("exclusive-parent-last-release",handle)
 return cause
}
