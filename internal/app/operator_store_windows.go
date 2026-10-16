//go:build windows

package app

import (
	"crypto/rand"
	"encoding/hex"

	"os"
	"path/filepath"
	"strings"
	"unsafe"

	"golang.org/x/sys/windows"
)

type windowsOperationStore struct { parents []windows.Handle; name string; lock windows.Handle; target *os.File; identity windows.ByHandleFileInformation; original []byte;unresolved bool;journalObject *windowsPrepared;observed recordStorageOutcome;nativeOwners map[windows.Handle]recordNativeIdentity;arbiter *recordArbiter;arbiterFile *os.File;arbiterExclusive bool;exclusiveParent windows.Handle;exclusiveParentIdentity windows.ByHandleFileInformation;closeAttempted,closeCompleted bool;closeResult error }
func winInfo(handle windows.Handle)(windows.ByHandleFileInformation,error){var info windows.ByHandleFileInformation;err:=windows.GetFileInformationByHandle(handle,&info);if err!=nil{return info,recordJoinedErrors(storageError(),err)};if info.FileAttributes&windows.FILE_ATTRIBUTE_REPARSE_POINT!=0{return info,storageError()};return info,nil}
func winSame(a,b windows.ByHandleFileInformation)bool{return a.VolumeSerialNumber==b.VolumeSerialNumber&&a.FileIndexHigh==b.FileIndexHigh&&a.FileIndexLow==b.FileIndexLow&&a.FileAttributes==b.FileAttributes}
// Actual NT call arguments/results stay private with the original local owner.
// A failing call's returned handle is an observation, never close authority.
type windowsChildObservation struct{
 Owner recordNativeIdentity
 Parent windows.Handle
 Name string
 Directory,Create bool
 Access,Share,Disposition,Options uint32
 Unicode *windows.NTUnicodeString
 Attributes windows.OBJECT_ATTRIBUTES
 Handle windows.Handle
 Status windows.IO_STATUS_BLOCK
 Attempted,Completed bool
 Cause error
}
func(o *windowsChildObservation)nativeCallOwner()recordNativeIdentity{return o.Owner}
func(o *windowsChildObservation)nativeCallFailure()error{return o.Cause}
func(s *windowsOperationStore)child(parent windows.Handle,name string,directory,create bool,access,share uint32)(windows.Handle,error){
 observation:=&windowsChildObservation{Parent:parent,Name:name,Directory:directory,Create:create,Access:access|windows.SYNCHRONIZE,Share:share}
 observation.Owner=recordNativeIdentity{Platform:"windows",Kind:"NtCreateFile-parent",Handle:uint64(parent)};if prior,exists:=s.nativeOwners[parent];exists{observation.Owner=prior;observation.Owner.Kind="NtCreateFile-parent"}
 s.observed.NativeCalls=append(s.observed.NativeCalls,observation)
 if name==""||name=="."||name==".."||strings.ContainsAny(name,"\\/:"){observation.Cause=storageError();return 0,observation.Cause}
 unicode,err:=windows.NewNTUnicodeString(name);observation.Unicode=unicode
 if err!=nil{observation.Cause=err;return 0,storageErrorCause(err)}
 observation.Attributes=windows.OBJECT_ATTRIBUTES{RootDirectory:parent,ObjectName:unicode,Attributes:windows.OBJ_CASE_INSENSITIVE};observation.Attributes.Length=uint32(unsafe.Sizeof(observation.Attributes))
 observation.Disposition=windows.FILE_OPEN
 if create{observation.Disposition=windows.FILE_CREATE;observation.Attributes.SecurityDescriptor,err=windows.SecurityDescriptorFromString("D:P(A;;FA;;;SY)(A;;FA;;;OW)");if err!=nil{observation.Cause=err;return 0,storageErrorCause(err)}}
 observation.Options=uint32(windows.FILE_OPEN_REPARSE_POINT|windows.FILE_SYNCHRONOUS_IO_NONALERT)|nativeFileWriteThrough;if directory{observation.Options|=windows.FILE_DIRECTORY_FILE}else{observation.Options|=windows.FILE_NON_DIRECTORY_FILE}
 observation.Attempted=true
 err=windows.NtCreateFile(&observation.Handle,observation.Access,&observation.Attributes,&observation.Status,nil,windows.FILE_ATTRIBUTE_NORMAL,share,observation.Disposition,observation.Options,0,0)
 observation.Cause=err;observation.Completed=true
 if err!=nil{return 0,err}
 handle:=observation.Handle;info,err:=s.info(handle);if err!=nil||(info.FileAttributes&windows.FILE_ATTRIBUTE_DIRECTORY!=0)!=directory{return 0,recordJoinedErrors(storageError(),err,s.closeHandle("rejected-child",handle))};return handle,nil
}
func acquireOperationStore(path string)(heldOperationStore,error){
	path,err:=filepath.Abs(path);if err!=nil{return nil,storageErrorCause(err)};volume:=filepath.VolumeName(path)
	// This local private store accepts a drive volume; UNC/device paths are not
	// silently treated as equivalent atomic local publication authorities.
	if len(volume)!=2||volume[1]!=':'{return nil,storageError()}
	rootName,err:=windows.UTF16PtrFromString(volume+"\\");if err!=nil{return nil,storageErrorCause(err)}
	s:=&windowsOperationStore{name:filepath.Base(path),observed:recordStorageOutcome{PublicationEffect:recordUnchanged,CleanupRelease:recordReleased}}
	root,err:=windows.CreateFile(rootName,windows.FILE_READ_ATTRIBUTES|windows.SYNCHRONIZE,windows.FILE_SHARE_READ|windows.FILE_SHARE_WRITE,nil,windows.OPEN_EXISTING,windows.FILE_FLAG_BACKUP_SEMANTICS|windows.FILE_FLAG_OPEN_REPARSE_POINT|windows.FILE_FLAG_WRITE_THROUGH,0)
	if err!=nil{return nil,recordJoinedErrors(storageError(),err)};info,err:=winInfo(root);if err!=nil||info.FileAttributes&windows.FILE_ATTRIBUTE_DIRECTORY==0{cleanup:=s.closeHandle("unidentified-root",root);return nil,&recordStorageOutcomeError{Outcome:s.nativeCleanupOutcome(),Primary:recordJoinedErrors(storageError(),err),Cleanup:cleanup}}
	s.parents=[]windows.Handle{root};s.rememberNativeOwner(root,info)
	fail:=func(causes ...error)(heldOperationStore,error){cleanup:=s.close();return nil,&recordStorageOutcomeError{Outcome:s.nativeCleanupOutcome(),Primary:recordJoinedErrors(append([]error{storageError()},causes...)...),Cleanup:cleanup}}
	if err:=qualifyWindowsNamespace(root);err!=nil{cleanup:=s.close();return nil,&recordStorageOutcomeError{Outcome:s.nativeCleanupOutcome(),Primary:err,Cleanup:cleanup}}
	parts:=strings.Split(strings.TrimPrefix(filepath.Dir(path),volume+"\\"),"\\")
	for index,name:=range parts{
		parent:=s.parents[len(s.parents)-1];access:=uint32(windows.FILE_READ_ATTRIBUTES);share:=uint32(windows.FILE_SHARE_READ|windows.FILE_SHARE_WRITE)
		if index==len(parts)-1{access|=windows.FILE_LIST_DIRECTORY;share=0}
		handle,err:=s.child(parent,name,true,false,access,share)
		if (err==windows.ERROR_FILE_NOT_FOUND||err==windows.ERROR_PATH_NOT_FOUND||err==windows.STATUS_OBJECT_NAME_NOT_FOUND)&&index==len(parts)-1{handle,err=s.child(parent,name,true,true,access,share)}
		if err!=nil{return fail(err)};s.parents=append(s.parents,handle);if index==len(parts)-1{s.exclusiveParent=handle;identity,identityErr:=s.info(handle);if identityErr!=nil{return fail(identityErr)};s.exclusiveParentIdentity=identity}
	}
	if err:=s.acquireArbiter();err!=nil{return fail(err)};s.lock,err=s.child(s.parent(),s.name+".publication",true,true,windows.FILE_READ_ATTRIBUTES|windows.DELETE,windows.FILE_SHARE_READ|windows.FILE_SHARE_WRITE);if err!=nil{return fail(err)}
	recovery,recoveryErr:=s.child(s.parent(),s.name+".publication-recovery.json",false,false,windows.FILE_READ_ATTRIBUTES,windows.FILE_SHARE_READ|windows.FILE_SHARE_WRITE|windows.FILE_SHARE_DELETE)
	if recoveryErr==nil{cleanup:=s.closeHandle("existing-recovery-marker",recovery);return fail(cleanup)};if recoveryErr!=windows.ERROR_FILE_NOT_FOUND&&recoveryErr!=windows.STATUS_OBJECT_NAME_NOT_FOUND{return fail(recoveryErr)};return s,nil
}
func(s *windowsOperationStore)parent()windows.Handle{return s.parents[len(s.parents)-1]}
func(s *windowsOperationStore)read()([]byte,error){
	if s.target!=nil{prior:=s.target;s.target=nil;if err:=s.closeFile("prior-record",prior);err!=nil{return nil,err}}
	handle,err:=s.child(s.parent(),s.name,false,false,windows.GENERIC_READ|windows.DELETE,windows.FILE_SHARE_READ|windows.FILE_SHARE_DELETE)
	if err==windows.ERROR_FILE_NOT_FOUND||err==windows.STATUS_OBJECT_NAME_NOT_FOUND{if err:=s.validateArbiterCurrent(nil);err!=nil{return nil,err};s.original=nil;s.observed.OriginalKnown=true;s.observed.Original=nil;return nil,nil};if err!=nil{return nil,recordJoinedErrors(storageError(),s.retainHelperFailure("record-open",s.parent(),err,false))}
	file:=os.NewFile(uintptr(handle),s.name);before,err:=s.info(handle);if err!=nil||before.FileSizeHigh!=0||before.FileSizeLow==0||before.FileSizeLow>4096{return nil,recordJoinedErrors(storageError(),err,s.closeFile("rejected-record",file))}
	raw,err:=recordReadFile(&s.observed,file,s.nativeOwner("record-read",windows.Handle(file.Fd())),4097);after,infoErr:=s.info(handle)
	if err!=nil||infoErr!=nil||len(raw)>4096{return nil,recordJoinedErrors(storageError(),s.retainHelperFailure("record-read",handle,err,false),infoErr,s.closeFile("rejected-record",file))};if !winSame(before,after)||before.FileSizeHigh!=after.FileSizeHigh||before.FileSizeLow!=after.FileSizeLow||before.LastWriteTime!=after.LastWriteTime{s.retainIdentityMismatch("record-readback",handle,before,after);return nil,recordJoinedErrors(storageError(),s.closeFile("rejected-record",file))}
	s.target=file;s.identity=after;if !s.currentTarget(){s.target=nil;return nil,recordJoinedErrors(storageError(),s.closeFile("rejected-current-record",file))};if err:=s.validateArbiterCurrent(raw);err!=nil{return nil,err};s.original=append([]byte(nil),raw...);s.observed.OriginalKnown=true;s.observed.Original=append([]byte(nil),raw...);return raw,nil
}
func(s *windowsOperationStore)currentTarget()(matches bool){
	handle,err:=s.child(s.parent(),s.name,false,false,windows.FILE_READ_ATTRIBUTES,windows.FILE_SHARE_READ|windows.FILE_SHARE_DELETE)
	if err==nil{defer func(){if s.closeHandle("comparison-record",handle)!=nil||s.observed.CleanupRelease==recordReleaseUnresolved{matches=false}}()}
	if s.target==nil{if err==windows.ERROR_FILE_NOT_FOUND||err==windows.STATUS_OBJECT_NAME_NOT_FOUND{return true};if err!=nil{s.retainHelperFailure("target-absence",s.parent(),err,true)}else{observed:=s.nativeOwner("unexpected-target",handle);s.unresolved=true;s.observed.CleanupRelease=recordReleaseUnresolved;s.observed.Presence=append(s.observed.Presence,recordNativePresenceObservation{Owner:recordNativeIdentity{Platform:"windows",Kind:"expected-target-absence",Handle:uint64(s.parent())},Observed:observed,ExpectedPresent:false,ObservedPresent:true})};return false};if err!=nil{s.retainHelperFailure("target-open",s.parent(),err,true);return false}
	info,err:=s.info(handle);if err!=nil{return false};if !winSame(info,s.identity)||info.FileSizeHigh!=s.identity.FileSizeHigh||info.FileSizeLow!=s.identity.FileSizeLow||info.LastWriteTime!=s.identity.LastWriteTime{s.retainIdentityMismatch("target-identity",handle,s.identity,info);return false};return !s.unresolved
}
type windowsRename struct { Flags uint32; RootDirectory windows.Handle; FileNameLength uint32; FileName [1]uint16 }
// Native relative rename binds both the prepared file handle and held parent.
// FILE_RENAME_POSIX_SEMANTICS keeps the original held object valid after the
// replacement. Unsupported filesystems/OS implementations fail, never fall
// back to os.Rename. Microsoft FILE_RENAME_INFO/INFORMATION defines this API.
type windowsSetInformationObservation struct{
 Owner recordNativeIdentity
 Kind string
 Handle,Parent windows.Handle
 Name string
 Replace bool
 Name16 []uint16
 Buffer []byte
 Rename *windowsRename
 Flags,Class,RequestedBytes uint32
 Attempted,Completed bool
 Cause error
}
func(o *windowsSetInformationObservation)nativeCallOwner()recordNativeIdentity{return o.Owner}
func(o *windowsSetInformationObservation)nativeCallFailure()error{return o.Cause}
func(s *windowsOperationStore)rename(handle,parent windows.Handle,name string,replace bool)error{
 observation:=&windowsSetInformationObservation{Kind:"rename",Handle:handle,Parent:parent,Name:name,Replace:replace,Class:windows.FileRenameInfoEx}
 s.observed.NativeCalls=append(s.observed.NativeCalls,observation);observation.Owner=s.nativeOwner("rename",handle)
 if !observation.Owner.IdentityKnown{observation.Cause=storageError();return observation.Cause}
 name16,err:=windows.UTF16FromString(name);observation.Name16=name16;if err!=nil{observation.Cause=err;return storageErrorCause(err)};units:=name16[:len(name16)-1]
 var header windowsRename;offset:=unsafe.Offsetof(header.FileName);observation.Buffer=make([]byte,int(unsafe.Sizeof(header))+len(units)*2)
 observation.Rename=(*windowsRename)(unsafe.Pointer(&observation.Buffer[0]));observation.Rename.RootDirectory=parent;observation.Rename.FileNameLength=uint32(len(units)*2)
 if replace{observation.Rename.Flags=windows.FILE_RENAME_REPLACE_IF_EXISTS|windows.FILE_RENAME_POSIX_SEMANTICS}
 copy(unsafe.Slice((*uint16)(unsafe.Pointer(&observation.Buffer[offset])),len(units)),units)
 observation.RequestedBytes=uint32(len(observation.Buffer));observation.Attempted=true
 observation.Cause=windows.SetFileInformationByHandle(handle,observation.Class,&observation.Buffer[0],observation.RequestedBytes);observation.Completed=true;return observation.Cause
}
func(s *windowsOperationStore)delete(handle windows.Handle)error{
 observation:=&windowsSetInformationObservation{Kind:"delete",Handle:handle,Class:windows.FileDispositionInfoEx,Flags:windows.FILE_DISPOSITION_DELETE|windows.FILE_DISPOSITION_POSIX_SEMANTICS,RequestedBytes:4}
 s.observed.NativeCalls=append(s.observed.NativeCalls,observation);observation.Owner=s.nativeOwner("delete",handle)
 if !observation.Owner.IdentityKnown{observation.Cause=storageError();return observation.Cause}
 observation.Attempted=true;observation.Cause=windows.SetFileInformationByHandle(handle,observation.Class,(*byte)(unsafe.Pointer(&observation.Flags)),observation.RequestedBytes);observation.Completed=true;return observation.Cause
}
type windowsPrepared struct{file *os.File;name string;identity windows.ByHandleFileInformation;raw []byte;retained bool;discardAttempted,discardCompleted bool;discardResult error}
func(s *windowsOperationStore)prepare(raw []byte)(*windowsPrepared,error){
	if s.unresolved{return nil,&recordPublicationUnresolved{}}
	var nonce [16]byte;if _,err:=rand.Read(nonce[:]);err!=nil{return nil,storageErrorCause(err)}
	name:=".operation-reconciliation-"+hex.EncodeToString(nonce[:]);handle,err:=s.child(s.parent(),name,false,true,windows.GENERIC_READ|windows.GENERIC_WRITE|windows.DELETE,windows.FILE_SHARE_READ|windows.FILE_SHARE_DELETE);if err!=nil{return nil,recordJoinedErrors(storageError(),err)}
	writer:=os.NewFile(uintptr(handle),name);written,writeErr:=recordWriteFile(&s.observed,writer,s.nativeOwner("prepared-write",windows.Handle(writer.Fd())),raw,false,0);s.observeWrite("prepared-write",handle,len(raw),written);syncErr:=recordFileSync(&s.observed,writer,s.nativeOwner("prepared-sync",windows.Handle(writer.Fd())))
	var duplicate windows.Handle;dupErr:=windows.DuplicateHandle(windows.CurrentProcess(),handle,windows.CurrentProcess(),&duplicate,0,false,windows.DUPLICATE_SAME_ACCESS)
	if dupErr!=nil{prepared:=&windowsPrepared{file:writer,name:name,raw:append([]byte(nil),raw...)};prepared.identity,err=s.info(handle);return nil,recordJoinedErrors(storageError(),writeErr,syncErr,dupErr,err,s.discardPrepared(prepared))}
	prepared:=&windowsPrepared{file:os.NewFile(uintptr(duplicate),name),name:name,raw:append([]byte(nil),raw...)};prepared.identity,err=s.info(duplicate);closeErr:=s.closeFile("prepared-writer",writer)
	if writeErr!=nil||written!=len(raw)||syncErr!=nil||err!=nil||closeErr!=nil||!s.preparedMatches(prepared,name)||s.unresolved{return nil,recordJoinedErrors(storageError(),writeErr,syncErr,err,closeErr,s.discardPrepared(prepared))};return prepared,nil
}
func(s *windowsOperationStore)preparedMatches(p *windowsPrepared,name string)(matches bool){
	handle,err:=s.child(s.parent(),name,false,false,windows.FILE_READ_ATTRIBUTES,windows.FILE_SHARE_READ|windows.FILE_SHARE_WRITE|windows.FILE_SHARE_DELETE);if err!=nil{s.retainHelperFailure("prepared-named-open",s.parent(),err,true);return false};defer func(){priorIdentityFailures:=len(s.observed.IdentityFailures);if s.closeHandle("comparison-prepared",handle)!=nil||len(s.observed.IdentityFailures)!=priorIdentityFailures{matches=false}}()
	named,err:=s.info(handle);heldHandle:=windows.Handle(p.file.Fd());held,heldErr:=s.info(heldHandle);if err!=nil||heldErr!=nil{return false};if !winSame(named,p.identity){s.retainIdentityMismatch("prepared-named-identity",handle,p.identity,named);return false};if !winSame(held,p.identity)||held.FileSizeHigh!=0||int(held.FileSizeLow)!=len(p.raw){s.retainIdentityMismatch("prepared-held-identity",heldHandle,p.identity,held);return false}
	if _,err:=recordFileSeek(&s.observed,p.file,s.nativeOwner("prepared-seek",windows.Handle(p.file.Fd())),0,0);err!=nil{s.retainHelperFailure("prepared-readback-seek",heldHandle,err,true);return false};raw,err:=recordReadFile(&s.observed,p.file,s.nativeOwner("prepared-readback",windows.Handle(p.file.Fd())),int64(len(p.raw)+1));if err!=nil{s.retainHelperFailure("prepared-readback",heldHandle,err,true);return false};if string(raw)!=string(p.raw){s.unresolved=true;s.observed.CleanupRelease=recordReleaseUnresolved;s.observed.BodyMismatches=append(s.observed.BodyMismatches,recordNativeBodyMismatch{Owner:s.nativeOwner("prepared-readback",heldHandle),Expected:append([]byte(nil),p.raw...),Observed:append([]byte(nil),raw...)});return false};return true
}
func(s *windowsOperationStore)discardPrepared(p *windowsPrepared)error{
 if p==nil{return nil};if p.discardAttempted{if p.discardCompleted{return p.discardResult};return &recordPublicationUnresolved{}};p.discardAttempted=true;var cleanup error;handle:=windows.Handle(p.file.Fd());if s.unresolved||s.observed.PublicationEffect==recordEffectUnresolved{p.retained=true}
 if !p.retained{identity,err:=s.info(handle);if err!=nil||!winSame(identity,p.identity){if err==nil{s.retainIdentityMismatch("prepared-identity",handle,p.identity,identity)};cleanup=recordJoinedErrors(storageError(),err);p.retained=true}else if err:=s.delete(handle);err!=nil{cleanup=s.retainHelperFailure("prepared-delete",handle,err,true);p.retained=true}}
 p.discardResult=recordJoinedErrors(cleanup,s.closeFile("prepared-object",p.file));p.discardCompleted=true;return p.discardResult
}
func(s *windowsOperationStore)journal(kind string,replacement []byte,backup,prepared string)error{
	if s.unresolved{return &recordPublicationUnresolved{}}
	raw,err:=recoveryBytes(kind,s.original,replacement,backup,prepared);if err!=nil{return err}
	name:=s.name+".publication-recovery.json";handle,err:=s.child(s.parent(),name,false,true,windows.GENERIC_READ|windows.GENERIC_WRITE|windows.DELETE,windows.FILE_SHARE_READ);if err!=nil{return recordJoinedErrors(storageError(),s.retainHelperFailure("journal-open",s.parent(),err,false))};file:=os.NewFile(uintptr(handle),name)
	s.journalObject=&windowsPrepared{file:file,name:name,raw:append([]byte(nil),raw...)};s.journalObject.identity,err=s.info(handle);if err!=nil{s.unresolved=true;return recordJoinedErrors(&recordPublicationUnresolved{},err)}
	written,writeErr:=recordWriteFile(&s.observed,file,s.nativeOwner("record-write",windows.Handle(file.Fd())),raw,false,0);s.observeWrite("journal-write",handle,len(raw),written);syncErr:=recordFileSync(&s.observed,file,s.nativeOwner("record-sync",windows.Handle(file.Fd())));if writeErr!=nil||written!=len(raw)||syncErr!=nil||!s.preparedMatches(s.journalObject,name){s.unresolved=true;return recordJoinedErrors(&recordPublicationUnresolved{},s.retainHelperFailure("journal-write",handle,writeErr,true),s.retainHelperFailure("journal-sync",handle,syncErr,true))};return nil
}
func(s *windowsOperationStore)publish(raw []byte)(failure error){
	if s.unresolved||!s.currentTarget(){return storageError()};if err:=s.prepareArbiter(raw);err!=nil{return err};var backup *windowsPrepared;var err error
	if s.original!=nil{backup,err=s.prepare(s.original);if err!=nil{return err};defer func(){failure=recordJoinedErrors(failure,s.discardPrepared(backup))}()}
	prepared,err:=s.prepare(raw);if err!=nil{return err};defer func(){failure=recordJoinedErrors(failure,s.discardPrepared(prepared))}()
	backupName:="";if backup!=nil{backupName=backup.name};if err:=s.journal("replace",raw,backupName,prepared.name);err!=nil{return err}
	if !s.currentTarget()||!s.preparedMatches(prepared,prepared.name)||s.unresolved{return storageError()}
	if err:=s.rename(windows.Handle(prepared.file.Fd()),s.parent(),s.name,s.target!=nil);err!=nil{return recordJoinedErrors(storageError(),s.retainHelperFailure("publication-rename",windows.Handle(prepared.file.Fd()),err,false))}
	s.observed.PublicationEffect=recordEffectUnresolved;s.observed.Replacement=append([]byte(nil),raw...)
	prepared.retained=true
	if !s.preparedMatches(prepared,s.name){
		if backup!=nil{if !s.preparedMatches(backup,backup.name){backup.retained=true;return &recordPublicationUnresolved{}};if err:=s.rename(windows.Handle(backup.file.Fd()),s.parent(),s.name,true);err!=nil{backup.retained=true;return recordJoinedErrors(&recordPublicationUnresolved{},s.retainHelperFailure("rollback-rename",windows.Handle(backup.file.Fd()),err,true))};backup.retained=true}else if err:=s.delete(windows.Handle(prepared.file.Fd()));err!=nil{return recordJoinedErrors(&recordPublicationUnresolved{},s.retainHelperFailure("rollback-delete",windows.Handle(prepared.file.Fd()),err,true))}
		return storageError()
	};s.observed.PublicationEffect=recordReplacementApplied;return nil
}
func(s *windowsOperationStore)remove()error{
 if s.unresolved||!s.currentTarget(){return storageError()};if s.target==nil{return nil}
 if err:=s.prepareArbiter(nil);err!=nil{return err};if err:=s.journal("clear",nil,"","");err!=nil{return err}
 if s.unresolved||!s.currentTarget(){return storageError()}
 if err:=s.delete(windows.Handle(s.target.Fd()));err!=nil{return recordJoinedErrors(storageError(),s.retainHelperFailure("clear-delete",windows.Handle(s.target.Fd()),err,false))}
 s.observed.PublicationEffect=recordEffectUnresolved
 // A successful disposition request does not prove namespace removal. Retain
 // the original target while a distinct native relative open observes absence
 // under the SAME held exclusive parent; no pathname-based replacement owner.
 handle,err:=s.child(s.parent(),s.name,false,false,windows.FILE_READ_ATTRIBUTES,windows.FILE_SHARE_READ|windows.FILE_SHARE_WRITE|windows.FILE_SHARE_DELETE)
 owner:=s.nativeOwner("clear-absence-parent",s.parent())
 if err==windows.ERROR_FILE_NOT_FOUND||err==windows.STATUS_OBJECT_NAME_NOT_FOUND{
  if s.unresolved||!owner.IdentityKnown{return &recordPublicationUnresolved{}}
  s.observed.Presence=append(s.observed.Presence,recordNativePresenceObservation{Owner:owner,ExpectedPresent:false,ObservedPresent:false})
  s.observed.PublicationEffect=recordClearApplied
  return nil
 }
 if err!=nil{return recordJoinedErrors(&recordPublicationUnresolved{},s.retainHelperFailure("clear-absence",s.parent(),err,true))}
 observed:=s.nativeOwner("clear-unexpected-target",handle)
 s.observed.Presence=append(s.observed.Presence,recordNativePresenceObservation{Owner:owner,Observed:observed,ExpectedPresent:false,ObservedPresent:true})
 s.unresolved=true
 return recordJoinedErrors(&recordPublicationUnresolved{},s.closeHandle("clear-comparison",handle))
}
func(s *windowsOperationStore)retainOwnership(){s.unresolved=true}
func(s *windowsOperationStore)close()error{
 if s.closeAttempted{if s.closeCompleted{return s.closeResult};return &recordPublicationUnresolved{}};s.closeAttempted=true
	var failure error;recordFailure:=func(err error){if err!=nil{failure=recordJoinedErrors(failure,err,&recordPublicationUnresolved{});s.unresolved=true;s.observed.CleanupRelease=recordReleaseUnresolved;s.observed.HelperFailures=append(s.observed.HelperFailures,recordNativeHelperFailure{Owner:recordNativeIdentity{Platform:"windows",Kind:"store-finalizer"},Cause:err})}}
	if s.target!=nil{recordFailure(s.closeFile("record",s.target));s.target=nil}
	if s.lock!=0{
		if !s.unresolved{
			if s.journalObject!=nil&&!s.preparedMatches(s.journalObject,s.journalObject.name){recordFailure(storageError())
			}else if err:=s.delete(s.lock);err!=nil{recordFailure(err)
			}else if s.journalObject!=nil{if !s.preparedMatches(s.journalObject,s.journalObject.name){recordFailure(storageError())}else{recordFailure(s.delete(windows.Handle(s.journalObject.file.Fd())))}}
		};recordFailure(s.closeHandle("publication-lock",s.lock))
	}
	if s.journalObject!=nil{recordFailure(s.closeFile("recovery-journal",s.journalObject.file))}
	for i:=len(s.parents)-1;i>=0;i--{if s.parents[i]!=s.exclusiveParent{recordFailure(s.closeHandle("parent-directory",s.parents[i]))}};recordFailure(s.finishArbiter());recordFailure(s.finishExclusiveParent());s.closeResult=failure;s.closeCompleted=true;return failure
}
