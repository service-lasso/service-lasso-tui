//go:build linux

package app

import (
	"crypto/rand"
	"encoding/hex"

	"os"
	"path/filepath"
	"strings"

	"golang.org/x/sys/unix"
)

type linuxDirectory struct{fd int;name string;identity unix.Stat_t}
type linuxOperationStore struct{chain []linuxDirectory;name string;lock int;lockIdentity unix.Stat_t;target *os.File;targetIdentity unix.Stat_t;original []byte;unresolved,lockReleased bool;journalObject *linuxPrepared;observed recordStorageOutcome;nativeOwners map[int]recordNativeIdentity;arbiter *recordArbiter;arbiterFile *os.File;arbiterExclusive bool;primaryParentFD int;primaryRetained bool;closeAttempted,closeCompleted bool;closeResult error}
func acquireOperationStore(path string)(heldOperationStore,error){
	path,err:=filepath.Abs(path);if err!=nil{return nil,recordJoinedErrors(storageError(),err)}
	parts:=strings.Split(strings.TrimPrefix(filepath.Dir(path),"/"),"/")
	root,err:=unix.Open("/",unix.O_RDONLY|unix.O_DIRECTORY|unix.O_CLOEXEC,0);if err!=nil{return nil,recordJoinedErrors(storageError(),err)}
	s:=&linuxOperationStore{name:filepath.Base(path),lock:-1,primaryParentFD:-1,observed:recordStorageOutcome{PublicationEffect:recordUnchanged,CleanupRelease:recordReleased}};var identity unix.Stat_t
	if err:=unix.Fstat(root,&identity);err!=nil{cleanup:=s.closeDescriptor("unidentified-root",root);return nil,&recordStorageOutcomeError{Outcome:s.storageOutcome(),Primary:recordJoinedErrors(storageError(),err),Cleanup:cleanup}};s.rememberNativeOwner(root,identity);s.chain=append(s.chain,linuxDirectory{root,"",identity})
	fail:=func(causes ...error)(heldOperationStore,error){cleanup:=s.close();return nil,&recordStorageOutcomeError{Outcome:s.storageOutcome(),Primary:recordJoinedErrors(append([]error{storageError()},causes...)...),Cleanup:cleanup}}
	for index,part:=range parts{
		if part==""||part=="."||part==".."{return fail()};parent:=s.chain[len(s.chain)-1].fd
		fd,err:=unix.Openat(parent,part,unix.O_RDONLY|unix.O_DIRECTORY|unix.O_NOFOLLOW|unix.O_CLOEXEC,0)
		if err==unix.ENOENT&&index==len(parts)-1{if cause:=unix.Mkdirat(parent,part,0700);cause!=nil{return fail(s.retainHelperFailure("acquire-parent-create",parent,cause,false))};fd,err=unix.Openat(parent,part,unix.O_RDONLY|unix.O_DIRECTORY|unix.O_NOFOLLOW|unix.O_CLOEXEC,0)}
		if err!=nil{return fail(s.retainHelperFailure("acquire-parent-open",parent,err,false))};if identityErr:=unix.Fstat(fd,&identity);identityErr!=nil{cleanup:=recordJoinedErrors(s.closeDescriptor("unidentified-parent",fd),s.close());return nil,&recordStorageOutcomeError{Outcome:s.storageOutcome(),Primary:recordJoinedErrors(storageError(),identityErr),Cleanup:cleanup}};s.rememberNativeOwner(fd,identity);s.chain=append(s.chain,linuxDirectory{fd,part,identity})
	}
	parent:=s.parent();if !s.validChain(){return fail()};if err:=s.acquireArbiter();err!=nil{return fail(err)};if err:=unix.Mkdirat(parent,s.name+".publication",0700);err!=nil{return fail(s.retainHelperFailure("acquire-lock-create",parent,err,false))}
	s.lock,err=unix.Openat(parent,s.name+".publication",unix.O_RDONLY|unix.O_DIRECTORY|unix.O_NOFOLLOW|unix.O_CLOEXEC,0)
	if err!=nil{return fail(s.retainHelperFailure("acquire-lock-open",parent,err,false))};if err:=unix.Fstat(s.lock,&s.lockIdentity);err!=nil{return fail(s.retainHelperFailure("acquire-lock-identity",s.lock,err,false))}
	s.rememberNativeOwner(s.lock,s.lockIdentity)
	var recovery unix.Stat_t;if err:=unix.Fstatat(parent,s.name+".publication-recovery.json",&recovery,unix.AT_SYMLINK_NOFOLLOW);err!=unix.ENOENT{return fail(s.retainHelperFailure("acquire-recovery-check",parent,err,false))};return s,nil
}
func(s *linuxOperationStore)parent()int{return s.chain[len(s.chain)-1].fd}
func sameLinuxObject(a,b unix.Stat_t)bool{return a.Dev==b.Dev&&a.Ino==b.Ino&&a.Mode==b.Mode}
func(s *linuxOperationStore)validChain()bool{
	for i:=1;i<len(s.chain);i++{var current unix.Stat_t;if err:=unix.Fstatat(s.chain[i-1].fd,s.chain[i].name,&current,unix.AT_SYMLINK_NOFOLLOW);err!=nil{s.retainHelperFailure("parent-chain-identity",s.chain[i-1].fd,err,true);return false};if !sameLinuxObject(current,s.chain[i].identity){s.retainIdentityMismatch("parent-chain-identity",s.chain[i].fd,s.chain[i].identity,current);return false}}
	if s.lock>=0&&!s.lockReleased{var current unix.Stat_t;if err:=unix.Fstatat(s.parent(),s.name+".publication",&current,unix.AT_SYMLINK_NOFOLLOW);err!=nil{s.retainHelperFailure("publication-lock-identity",s.parent(),err,true);return false};if !sameLinuxObject(current,s.lockIdentity){s.retainIdentityMismatch("publication-lock-identity",s.lock,s.lockIdentity,current);return false}};return true
}
func(s *linuxOperationStore)read()([]byte,error){
	if !s.validChain(){return nil,storageError()};if s.target!=nil{prior:=s.target;s.target=nil;if err:=s.closeFile("prior-record",prior);err!=nil{return nil,err}}
	fd,err:=unix.Openat(s.parent(),s.name,unix.O_RDONLY|unix.O_NOFOLLOW|unix.O_CLOEXEC,0);if err==unix.ENOENT{if err:=s.validateArbiterCurrent(nil);err!=nil{return nil,err};s.original=nil;s.observed.OriginalKnown=true;s.observed.Original=nil;return nil,nil};if err!=nil{return nil,recordJoinedErrors(storageError(),s.retainHelperFailure("record-open",s.parent(),err,false))}
	file:=os.NewFile(uintptr(fd),s.name);var before,after unix.Stat_t
	if err:=unix.Fstat(fd,&before);err!=nil{return nil,recordJoinedErrors(storageError(),s.retainHelperFailure("record-initial-identity",fd,err,false),s.closeFile("rejected-record",file))};s.rememberNativeOwner(fd,before);if before.Mode&unix.S_IFMT!=unix.S_IFREG||before.Size<=0||before.Size>4096{return nil,recordJoinedErrors(storageError(),s.closeFile("rejected-record",file))}
	raw,readErr:=recordReadFile(&s.observed,file,s.nativeOwner("record-read",int(file.Fd())),4097);statErr:=unix.Fstat(fd,&after);if readErr!=nil||statErr!=nil||len(raw)>4096{return nil,recordJoinedErrors(storageError(),s.retainHelperFailure("record-read",fd,readErr,false),s.retainHelperFailure("record-final-identity",fd,statErr,false),s.closeFile("rejected-record",file))};if !sameLinuxObject(before,after)||before.Size!=after.Size||before.Mtim!=after.Mtim||before.Ctim!=after.Ctim{s.retainIdentityMismatch("record-readback",fd,before,after);return nil,recordJoinedErrors(storageError(),s.closeFile("rejected-record",file))}
	var named unix.Stat_t;if !s.validChain(){return nil,recordJoinedErrors(storageError(),s.closeFile("rejected-record",file))};if err:=unix.Fstatat(s.parent(),s.name,&named,unix.AT_SYMLINK_NOFOLLOW);err!=nil{return nil,recordJoinedErrors(storageError(),s.retainHelperFailure("record-named-identity",s.parent(),err,false),s.closeFile("rejected-record",file))};if !sameLinuxObject(named,after){s.retainIdentityMismatch("record-named-identity",fd,after,named);return nil,recordJoinedErrors(storageError(),s.closeFile("rejected-record",file))}
	s.target=file;s.targetIdentity=after;s.rememberNativeOwner(fd,after);if err:=s.validateArbiterCurrent(raw);err!=nil{return nil,err};s.original=append([]byte(nil),raw...);s.observed.OriginalKnown=true;s.observed.Original=append([]byte(nil),raw...);return raw,nil
}
func(s *linuxOperationStore)currentTarget()bool{
	var named unix.Stat_t;err:=unix.Fstatat(s.parent(),s.name,&named,unix.AT_SYMLINK_NOFOLLOW)
	if s.target==nil{if err==unix.ENOENT{return true};if err!=nil{s.retainHelperFailure("target-absence",s.parent(),err,true)}else{s.unresolved=true;s.observed.CleanupRelease=recordReleaseUnresolved;s.observed.IdentityMismatches=append(s.observed.IdentityMismatches,recordNativeIdentityMismatch{Owner:recordNativeIdentity{Platform:"linux",Kind:"expected-target-absence",Handle:uint64(s.parent())},Observed:linuxNativeIdentity("unexpected-target",s.parent(),named)})};return false};if err!=nil{s.retainHelperFailure("target-identity",s.parent(),err,true);return false};if !sameLinuxObject(named,s.targetIdentity)||named.Size!=s.targetIdentity.Size||named.Mtim!=s.targetIdentity.Mtim||named.Ctim!=s.targetIdentity.Ctim{s.retainIdentityMismatch("target-identity",int(s.target.Fd()),s.targetIdentity,named);return false};return !s.unresolved
}
type linuxPrepared struct{name string;file *os.File;identity unix.Stat_t;raw []byte;retained bool;discardAttempted,discardCompleted bool;discardResult error}
func(s *linuxOperationStore)temporary(raw []byte)(*linuxPrepared,error){
	var nonce [16]byte;if _,err:=rand.Read(nonce[:]);err!=nil{return nil,storageErrorCause(err)};name:=".operation-reconciliation-"+hex.EncodeToString(nonce[:])
	fd,err:=unix.Openat(s.parent(),name,unix.O_RDWR|unix.O_CREAT|unix.O_EXCL|unix.O_NOFOLLOW|unix.O_CLOEXEC,0600);if err!=nil{return nil,storageErrorCause(err)};file:=os.NewFile(uintptr(fd),name)
	written,writeErr:=recordWriteFile(&s.observed,file,s.nativeOwner("record-write",int(file.Fd())),raw,false,0);s.observeWrite("record-write",int(file.Fd()),len(raw),written);syncErr:=recordFileSync(&s.observed,file,s.nativeOwner("record-sync",int(file.Fd())));duplicate,dupErr:=unix.FcntlInt(uintptr(fd),unix.F_DUPFD_CLOEXEC,0)
	if dupErr!=nil{prepared:=&linuxPrepared{name:name,file:file,raw:append([]byte(nil),raw...)};identityErr:=unix.Fstat(fd,&prepared.identity);return nil,recordJoinedErrors(storageError(),writeErr,syncErr,dupErr,identityErr,s.discardPrepared(prepared))}
	prepared:=&linuxPrepared{name:name,file:os.NewFile(uintptr(duplicate),name),raw:append([]byte(nil),raw...)};identityErr:=unix.Fstat(duplicate,&prepared.identity);if identityErr==nil{s.rememberNativeOwner(duplicate,prepared.identity);s.rememberNativeOwner(fd,prepared.identity)};closeErr:=s.closeFile("prepared-writer",file)
	if writeErr!=nil||written!=len(raw)||syncErr!=nil||identityErr!=nil||closeErr!=nil||!s.preparedMatches(prepared,name){return nil,recordJoinedErrors(storageError(),writeErr,syncErr,identityErr,closeErr,s.discardPrepared(prepared))};return prepared,nil
}
func(s *linuxOperationStore)preparedMatches(p *linuxPrepared,name string)bool{
	var named,held unix.Stat_t;if !s.validChain(){return false};fd:=int(p.file.Fd());if err:=unix.Fstatat(s.parent(),name,&named,unix.AT_SYMLINK_NOFOLLOW);err!=nil{s.retainHelperFailure("prepared-named-identity",s.parent(),err,true);return false};if err:=unix.Fstat(fd,&held);err!=nil{s.retainHelperFailure("prepared-held-identity",fd,err,true);return false};if !sameLinuxObject(named,p.identity){s.retainIdentityMismatch("prepared-named-identity",fd,p.identity,named);return false};if !sameLinuxObject(held,p.identity)||held.Size!=int64(len(p.raw)){s.retainIdentityMismatch("prepared-held-identity",fd,p.identity,held);return false};if named.Size!=held.Size{s.retainIdentityMismatch("prepared-size",fd,held,named);return false}
	if _,err:=recordFileSeek(&s.observed,p.file,s.nativeOwner("prepared-seek",int(p.file.Fd())),0,0);err!=nil{s.retainHelperFailure("prepared-readback-seek",fd,err,true);return false};raw,err:=recordReadFile(&s.observed,p.file,s.nativeOwner("prepared-readback",int(p.file.Fd())),int64(len(p.raw)+1));if err!=nil{s.retainHelperFailure("prepared-readback",fd,err,true);return false};if string(raw)!=string(p.raw){s.unresolved=true;s.observed.CleanupRelease=recordReleaseUnresolved;s.observed.BodyMismatches=append(s.observed.BodyMismatches,recordNativeBodyMismatch{Owner:linuxNativeIdentity("prepared-readback",fd,held),Expected:append([]byte(nil),p.raw...),Observed:append([]byte(nil),raw...)});return false};return true
}
func(s *linuxOperationStore)discardPrepared(p *linuxPrepared)error{
 if p==nil{return nil};if p.discardAttempted{if p.discardCompleted{return p.discardResult};return &recordPublicationUnresolved{}};p.discardAttempted=true;var cleanup error;fd:=int(p.file.Fd());s.nativeOwner("prepared-object",fd);if s.unresolved||s.observed.PublicationEffect==recordEffectUnresolved{p.retained=true}
 if !p.retained{var named unix.Stat_t;err:=unix.Fstatat(s.parent(),p.name,&named,unix.AT_SYMLINK_NOFOLLOW);if err!=unix.ENOENT{if err!=nil{p.retained=true;cleanup=recordJoinedErrors(storageError(),s.retainHelperFailure("discard-named-identity",s.parent(),err,true))}else if !s.validChain(){p.retained=true;cleanup=storageError()}else if !sameLinuxObject(named,p.identity){p.retained=true;s.retainIdentityMismatch("discard-named-identity",fd,p.identity,named);cleanup=storageError()}else if err:=unix.Unlinkat(s.parent(),p.name,0);err!=nil{p.retained=true;cleanup=recordJoinedErrors(storageError(),s.retainHelperFailure("discard-unlink",s.parent(),err,true))}else if err:=unix.Fsync(s.parent());err!=nil{cleanup=recordJoinedErrors(storageError(),s.retainHelperFailure("discard-parent-sync",s.parent(),err,true))}}}
 p.discardResult=recordJoinedErrors(cleanup,s.closeFile("prepared-object",p.file));p.discardCompleted=true;return p.discardResult
}
func(s *linuxOperationStore)preparedMatchesName(p *linuxPrepared)bool{return s.preparedMatchesNameAt(p,p.name)}
func(s *linuxOperationStore)journal(kind string,replacement []byte,backup,prepared string)error{
	raw,err:=recoveryBytes(kind,s.original,replacement,backup,prepared);if err!=nil{return err}
	name:=s.name+".publication-recovery.json";fd,err:=unix.Openat(s.parent(),name,unix.O_RDWR|unix.O_CREAT|unix.O_EXCL|unix.O_NOFOLLOW|unix.O_CLOEXEC,0600);if err!=nil{return recordJoinedErrors(storageError(),s.retainHelperFailure("journal-open",s.parent(),err,false))};file:=os.NewFile(uintptr(fd),name)
	s.journalObject=&linuxPrepared{name:name,file:file,raw:append([]byte(nil),raw...)}
	if err:=unix.Fstat(fd,&s.journalObject.identity);err!=nil{s.unresolved=true;return recordJoinedErrors(&recordPublicationUnresolved{},s.retainHelperFailure("journal-identity",fd,err,true))}
	s.rememberNativeOwner(fd,s.journalObject.identity)
	written,writeErr:=recordWriteFile(&s.observed,file,s.nativeOwner("record-write",int(file.Fd())),raw,false,0);s.observeWrite("record-write",int(file.Fd()),len(raw),written);syncErr:=recordFileSync(&s.observed,file,s.nativeOwner("record-sync",int(file.Fd())));if writeErr!=nil||written!=len(raw)||syncErr!=nil||!s.preparedMatches(s.journalObject,name){s.unresolved=true;return recordJoinedErrors(&recordPublicationUnresolved{},s.retainHelperFailure("journal-write",fd,writeErr,true),s.retainHelperFailure("journal-sync",fd,syncErr,true))};if err:=unix.Fsync(s.parent());err!=nil{return recordJoinedErrors(&recordPublicationUnresolved{},s.retainHelperFailure("journal-parent-sync",s.parent(),err,true))};return nil
}
func(s *linuxOperationStore)publish(raw []byte)(failure error){
	if !s.validChain()||!s.currentTarget(){return storageError()}
	if err:=s.prepareArbiter(raw);err!=nil{return err}
	var backup *linuxPrepared;var err error
	if s.original!=nil{backup,err=s.temporary(s.original);if err!=nil{return err};defer func(){failure=recordJoinedErrors(failure,s.discardPrepared(backup))}()}
	prepared,err:=s.temporary(raw);if err!=nil{return err};defer func(){failure=recordJoinedErrors(failure,s.discardPrepared(prepared))}()
	backupName:="";if backup!=nil{backupName=backup.name};if err:=s.journal("replace",raw,backupName,prepared.name);err!=nil{return err}
	if !s.validChain()||!s.currentTarget()||!s.preparedMatches(prepared,prepared.name){return storageError()}
	if err:=unix.Renameat(s.parent(),prepared.name,s.parent(),s.name);err!=nil{return recordJoinedErrors(storageError(),s.retainHelperFailure("publication-rename",s.parent(),err,false))}
	s.observed.PublicationEffect=recordEffectUnresolved;s.observed.Replacement=append([]byte(nil),raw...)
	matched:=s.preparedMatches(prepared,s.name);var durabilityErr error;if matched{durabilityErr=unix.Fsync(s.parent());s.retainHelperFailure("publication-parent-sync",s.parent(),durabilityErr,true)}
	if !matched||durabilityErr!=nil{
		// Keep the complete old preimage until new identity/readback/durability
		// succeed. Rollback occurs under the same held publication ownership.
		if !s.preparedMatchesNameAt(prepared,s.name){if backup!=nil{backup.retained=true};return &recordPublicationUnresolved{}}
		if backup!=nil{if !s.preparedMatches(backup,backup.name){backup.retained=true;return recordJoinedErrors(&recordPublicationUnresolved{},durabilityErr)};if err:=unix.Renameat(s.parent(),backup.name,s.parent(),s.name);err!=nil{backup.retained=true;return recordJoinedErrors(&recordPublicationUnresolved{},durabilityErr,s.retainHelperFailure("rollback-rename",s.parent(),err,true))}}else if err:=unix.Unlinkat(s.parent(),s.name,0);err!=nil{return recordJoinedErrors(&recordPublicationUnresolved{},durabilityErr,s.retainHelperFailure("rollback-unlink",s.parent(),err,true))}
		if err:=unix.Fsync(s.parent());err!=nil{return recordJoinedErrors(&recordPublicationUnresolved{},durabilityErr,s.retainHelperFailure("rollback-parent-sync",s.parent(),err,true))};if backup!=nil{if !s.preparedMatches(backup,s.name){backup.retained=true;return recordJoinedErrors(&recordPublicationUnresolved{},durabilityErr)}}else{var named unix.Stat_t;if err:=unix.Fstatat(s.parent(),s.name,&named,unix.AT_SYMLINK_NOFOLLOW);err!=unix.ENOENT{return recordJoinedErrors(&recordPublicationUnresolved{},durabilityErr,s.retainHelperFailure("rollback-absence",s.parent(),err,true))}};s.observed.PublicationEffect=recordUnchanged;return recordJoinedErrors(storageError(),durabilityErr)
	};s.observed.PublicationEffect=recordReplacementApplied;return nil
}
func(s *linuxOperationStore)preparedMatchesNameAt(p *linuxPrepared,name string)bool{var named unix.Stat_t;if !s.validChain(){return false};if err:=unix.Fstatat(s.parent(),name,&named,unix.AT_SYMLINK_NOFOLLOW);err!=nil{s.retainHelperFailure("prepared-name-identity",s.parent(),err,true);return false};if !sameLinuxObject(named,p.identity){s.retainIdentityMismatch("prepared-name-identity",int(p.file.Fd()),p.identity,named);return false};return true}
func(s *linuxOperationStore)remove()(failure error){
	if !s.validChain()||!s.currentTarget(){return storageError()};if s.target==nil{return nil}
	if err:=s.prepareArbiter(nil);err!=nil{return err}
	backup,err:=s.temporary(s.original);if err!=nil{return err};defer func(){failure=recordJoinedErrors(failure,s.discardPrepared(backup))}()
	if err:=s.journal("clear",nil,backup.name,"");err!=nil{return err}
	if !s.currentTarget(){return storageError()};if err:=unix.Unlinkat(s.parent(),s.name,0);err!=nil{return recordJoinedErrors(storageError(),s.retainHelperFailure("clear-unlink",s.parent(),err,false))}
	s.observed.PublicationEffect=recordEffectUnresolved
	if err:=unix.Fsync(s.parent());err!=nil{backup.retained=true;return recordJoinedErrors(&recordPublicationUnresolved{},s.retainHelperFailure("clear-parent-sync",s.parent(),err,true))};var named unix.Stat_t;if err:=unix.Fstatat(s.parent(),s.name,&named,unix.AT_SYMLINK_NOFOLLOW);err!=unix.ENOENT{backup.retained=true;return recordJoinedErrors(&recordPublicationUnresolved{},s.retainHelperFailure("clear-absence",s.parent(),err,true))};s.observed.PublicationEffect=recordClearApplied;return nil
}
func(s *linuxOperationStore)retainOwnership(){s.unresolved=true}
// Called only after the original named lock was successfully unlinked. The
// native descriptor remains held and must still be closed by its owner.
func(s *linuxOperationStore)releaseLockSync()error{s.lockReleased=true;return unix.Fsync(s.parent())}
// Re-establish a durable exclusion after a failed final directory sync. Never
// replace an existing named object; any existing marker already excludes new
// publication and belongs to its actual holder, not this cleanup attempt.
func(s *linuxOperationStore)restoreRecoveryMarker()error{
	if s.journalObject==nil||!s.validChain(){return &recordPublicationUnresolved{}}
	name:=s.journalObject.name;raw:=append([]byte(nil),s.journalObject.raw...)
	var named unix.Stat_t;if err:=unix.Fstatat(s.parent(),name,&named,unix.AT_SYMLINK_NOFOLLOW);err==nil{s.unresolved=true;s.observed.CleanupRelease=recordReleaseUnresolved;s.observed.Presence=append(s.observed.Presence,recordNativePresenceObservation{Owner:linuxNativeIdentity("original-recovery-marker",int(s.journalObject.file.Fd()),s.journalObject.identity),Observed:linuxNativeIdentity("recovery-marker-already-present",s.parent(),named),ExpectedPresent:false,ObservedPresent:true});return &recordPublicationUnresolved{}}else if err!=unix.ENOENT{return recordJoinedErrors(&recordPublicationUnresolved{},s.retainHelperFailure("recovery-marker-check",s.parent(),err,true))}
	fd,err:=unix.Openat(s.parent(),name,unix.O_RDWR|unix.O_CREAT|unix.O_EXCL|unix.O_NOFOLLOW|unix.O_CLOEXEC,0600);if err!=nil{return recordJoinedErrors(&recordPublicationUnresolved{},s.retainHelperFailure("recovery-marker-create",s.parent(),err,true))}
	file:=os.NewFile(uintptr(fd),name);replacement:=&linuxPrepared{name:name,file:file,raw:raw};statErr:=unix.Fstat(fd,&replacement.identity);if statErr==nil{s.rememberNativeOwner(fd,replacement.identity)};written,writeErr:=recordWriteFile(&s.observed,file,s.nativeOwner("record-write",int(file.Fd())),raw,false,0);s.observeWrite("recovery-write",fd,len(raw),written);syncErr:=recordFileSync(&s.observed,file,s.nativeOwner("record-sync",int(file.Fd())))
	if statErr!=nil||writeErr!=nil||written!=len(raw)||syncErr!=nil||!s.preparedMatches(replacement,name){return recordJoinedErrors(&recordPublicationUnresolved{},s.retainHelperFailure("recovery-marker-identity",fd,statErr,true),s.retainHelperFailure("recovery-marker-write",fd,writeErr,true),s.retainHelperFailure("recovery-marker-sync",fd,syncErr,true),s.closeFile("failed-recovery-marker",file))};if err:=unix.Fsync(s.parent());err!=nil{return recordJoinedErrors(&recordPublicationUnresolved{},s.retainHelperFailure("recovery-marker-parent-sync",s.parent(),err,true),s.closeFile("unsynced-recovery-marker",file))}
	old:=s.journalObject;s.journalObject=replacement;return recordJoinedErrors(&recordPublicationUnresolved{},s.closeFile("original-recovery-journal",old.file))
}
func(s *linuxOperationStore)close()error{
 if s.closeAttempted{if s.closeCompleted{return s.closeResult};return &recordPublicationUnresolved{}};s.closeAttempted=true
	var failure error
	recordFailure:=func(err error){if err!=nil{failure=recordJoinedErrors(failure,err,&recordPublicationUnresolved{});s.unresolved=true;s.observed.CleanupRelease=recordReleaseUnresolved;s.observed.HelperFailures=append(s.observed.HelperFailures,recordNativeHelperFailure{Owner:recordNativeIdentity{Platform:"linux",Kind:"store-finalizer"},Cause:err})}}
	if s.target!=nil{recordFailure(s.closeFile("record",s.target));s.target=nil}
	if s.lock>=0{
		if !s.unresolved{
			if !s.validChain(){recordFailure(storageError())
			}else if s.journalObject!=nil&&!s.preparedMatches(s.journalObject,s.journalObject.name){recordFailure(storageError())
			}else if err:=unix.Unlinkat(s.parent(),s.name+".publication",unix.AT_REMOVEDIR);err!=nil{recordFailure(err)
			}else if err:=s.releaseLockSync();err!=nil{recordFailure(err)
			}else if s.journalObject!=nil{
				if !s.preparedMatches(s.journalObject,s.journalObject.name){recordFailure(storageError())
				}else if err:=unix.Unlinkat(s.parent(),s.journalObject.name,0);err!=nil{recordFailure(err)
				}else{recordFailure(unix.Fsync(s.parent()))}
			}
		};recordFailure(s.closeDescriptor("publication-lock",s.lock))
	}
	if s.unresolved&&s.journalObject!=nil{failure=recordJoinedErrors(failure,s.restoreRecoveryMarker())}
	if s.journalObject!=nil{recordFailure(s.closeFile("recovery-journal",s.journalObject.file))}
	if s.arbiter!=nil&&!s.namedArbiterMatches(){recordFailure(storageError())};for i:=len(s.chain)-1;i>=0;i--{if s.chain[i].fd!=s.primaryParentFD{recordFailure(s.closeDescriptor("parent-directory",s.chain[i].fd))}};recordFailure(s.finishArbiter());recordFailure(s.finishPrimaryParent());s.closeResult=failure;s.closeCompleted=true;return failure
}
