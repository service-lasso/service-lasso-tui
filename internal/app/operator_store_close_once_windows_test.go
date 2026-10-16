//go:build windows

package app

import("bytes";"os";"path/filepath";"testing";"errors";"unsafe";"golang.org/x/sys/windows")

func recordCloseSourceFixture()(func(*os.File)error,func()recordStorageOutcome){
 store:=&windowsOperationStore{observed:recordStorageOutcome{PublicationEffect:recordUnchanged,CleanupRelease:recordReleased}}
 return func(file *os.File)error{return store.closeFile("source-fixture-file",file)},store.nativeCleanupOutcome
}

func recordFinalizerSourceFixture(file *os.File)(func()error,func()recordStorageOutcome){
 store:=&windowsOperationStore{target:file,observed:recordStorageOutcome{PublicationEffect:recordUnchanged,CleanupRelease:recordReleased}}
 return store.close,store.nativeCleanupOutcome
}

func recordDiscardSourceFixture(file *os.File)(func()error,func()recordStorageOutcome){
 store:=&windowsOperationStore{observed:recordStorageOutcome{PublicationEffect:recordUnchanged,CleanupRelease:recordReleased}}
 prepared:=&windowsPrepared{file:file,retained:true}
 return func()error{return store.discardPrepared(prepared)},store.nativeCleanupOutcome
}
// Actual isolated native failure source case, UNRUN. It supplies no installed
// native factory, ABI/finite profile, durable release or positive qualification.
func TestWindowsChildFailureKeepsOriginalNTCallObjects(t *testing.T){
 parent,err:=os.Open(t.TempDir());if err!=nil{t.Fatal(err)};defer parent.Close()
 owner:=&windowsOperationStore{observed:recordStorageOutcome{PublicationEffect:recordUnchanged,CleanupRelease:recordReleased}}
 handle,cause:=owner.child(windows.Handle(parent.Fd()),"missing-child",false,false,windows.FILE_READ_ATTRIBUTES,windows.FILE_SHARE_READ)
 if cause==nil||handle!=0{t.Fatal("missing child unexpectedly admitted")}
 if len(owner.observed.NativeCalls)!=1{t.Fatal("original native invocation not owned")}
 original,ok:=owner.observed.NativeCalls[0].(*windowsChildObservation)
 if !ok||original.Cause!=cause||!original.Attempted||!original.Completed||original.Unicode==nil||original.Attributes.ObjectName!=original.Unicode||original.Parent!=windows.Handle(parent.Fd()){t.Fatal("actual NT argument/result objects replaced or dropped")}
 observed:=owner.nativeCleanupOutcome()
 if observed.NativeCalls[0]!=original{t.Fatal("outcome snapshot copied original native call object")}
 failure:=&recordStorageOutcomeError{Outcome:observed}
 if !errors.Is(failure,cause){t.Fatal("original NT failure graph lost")}
}
func windowsRenameFixture(t *testing.T)(*windowsOperationStore,*windowsPrepared){
 t.Helper();held,err:=acquireOperationStore(filepath.Join(t.TempDir(),"target"));if err!=nil{logRecordFailure(t,err);t.Fatal("native rename fixture acquisition failed")};owner:=held.(*windowsOperationStore)
 if _,err:=owner.read();err!=nil{t.Fatal("native rename fixture original read failed")}
 prepared,err:=owner.prepare([]byte("native rename fixture"));if err!=nil{t.Fatal("native rename fixture preparation failed")}
 t.Cleanup(func(){if err:=owner.discardPrepared(prepared);err!=nil{t.Error("native rename fixture source release failed")};if err:=owner.close();err!=nil{t.Error("native rename fixture owner release failed")}})
 return owner,prepared
}
func TestWindowsRenameRetainsOriginalBufferAndHeaderAlias(t *testing.T){
 owner,prepared:=windowsRenameFixture(t);first:=len(owner.observed.NativeCalls)
 cause:=owner.rename(prepared,owner.parent(),"renamed-source",false)
 var original *windowsSetInformationObservation
 for _,call:=range owner.observed.NativeCalls[first:]{if observation,ok:=call.(*windowsSetInformationObservation);ok{original=observation;break}}
 if original==nil||!original.Attempted||!original.Completed||original.Cause!=cause||original.Rename==nil||len(original.Buffer)==0||len(original.Name16)==0{t.Fatal("actual native rename buffer/result not retained")}
 if original.API!="NtSetInformationFile"||original.Class!=65||original.RequestedBytes!=uint32(unsafe.Offsetof(windowsRename{}.FileName))+uint32((len(original.Name16)-1)*2)||original.Rename.RootDirectory!=0||original.Parent!=owner.parent()||original.PreparedOriginal!=prepared{t.Fatal("native same-directory rename lost exact source/API/class/parent/extent")}
 if original.Rename!=(*windowsRename)(unsafe.Pointer(&original.Buffer[0])){t.Fatal("native rename header no longer aliases original backing")}
 if owner.nativeCleanupOutcome().NativeCalls[first]!=original{t.Fatal("rename observation identity replaced")}
 if cause!=nil{logRecordFailure(t,&recordStorageOutcomeError{Outcome:owner.nativeCleanupOutcome(),Primary:cause});t.Fatal("native same-directory rename failed")}
 if !owner.preparedMatches(prepared,"renamed-source"){t.Fatal("renamed source lost SAME held parent/object/bytes")}
 // This actual isolated namespace result is not complete durable publication.
}
func TestWindowsSameDirectoryRenameRefusesEscapeForeignParentAndChangedCarrier(t *testing.T){
 owner,prepared:=windowsRenameFixture(t);foreignOwner,foreignPrepared:=windowsRenameFixture(t)
 for _,leaf:=range []string{"",".","..","../escape","nested/escape","nested\\escape","C:escape","leaf\x00tail"}{
  first:=len(owner.observed.NativeCalls);if owner.rename(prepared,owner.parent(),leaf,false)==nil{t.Fatal("escape leaf admitted")};row:=owner.observed.NativeCalls[first].(*windowsSetInformationObservation);if row.Attempted{t.Fatal("escape issued native rename")}
 }
 for _,source:=range []*windowsPrepared{foreignPrepared,func()*windowsPrepared{copy:=*prepared;copy.parentIdentity.FileIndexLow^=1;return &copy}()}{
  first:=len(owner.observed.NativeCalls);if owner.rename(source,owner.parent(),"refused-target",false)==nil{t.Fatal("different original source carrier admitted")};row:=owner.observed.NativeCalls[first].(*windowsSetInformationObservation);if row.Attempted{t.Fatal("foreign/changed carrier issued native rename")}
 }
 if !owner.preparedMatches(prepared,prepared.name)||!foreignOwner.preparedMatches(foreignPrepared,foreignPrepared.name){t.Fatal("refusal changed actual prepared source/parent/bytes")}
}
func TestWindowsClearKeepsIndependentOriginalAndOnceClosedDeleteOwner(t *testing.T){
 path:=filepath.Join(t.TempDir(),"record.json")
 value:=persistedOperation{Version:2,OperationID:"operation-A",ConnectionName:"profile",Binding:selectedOperatorTestContext("a").LegacyDigest()}
 store:=fileOperationStore{path:path};if err:=store.Save(value);err!=nil{logRecordFailure(t,err);t.Fatal("clear fixture Save failed")}
 held,err:=acquireOperationStore(path);if err!=nil{logRecordFailure(t,err);t.Fatal("clear fixture acquisition failed")};owner:=held.(*windowsOperationStore);defer owner.close()
 raw,err:=owner.read();if err!=nil{t.Fatal("clear fixture read failed")};deleteOwner:=owner.target;deleteHandle:=windows.Handle(deleteOwner.Fd())
 if err:=owner.remove();err!=nil{logRecordFailure(t,&recordStorageOutcomeError{Outcome:owner.nativeCleanupOutcome(),Primary:err});t.Fatal("actual clear did not settle")}
 original:=owner.clearOriginal
 if original==nil||original.DeleteOwner!=deleteOwner||original.Reader==nil||original.Reader==deleteOwner||original.Acquire==nil||original.Acquire.Handle==deleteHandle||original.Acquire.Handle!=windows.Handle(original.Reader.Fd())||original.Acquire.Access&(windows.GENERIC_WRITE|windows.DELETE)!=0{t.Fatal("clear did not retain a distinct native read-only original")}
 if original.Delete==nil||!original.Delete.Attempted||!original.Delete.Completed||original.Delete.Cause!=nil||original.Delete.Handle!=deleteHandle||original.DeleteClose==nil||original.DeleteClose.File!=deleteOwner||!original.DeleteClose.Completed||original.DeleteClose.Cause!=nil{t.Fatal("actual delete/once-close originals lost")}
 if original.Absence==nil||!original.Absence.Completed||original.Absence.Parent!=owner.parent()||(original.Absence.Cause!=windows.ERROR_FILE_NOT_FOUND&&original.Absence.Cause!=windows.STATUS_OBJECT_NAME_NOT_FOUND)||original.After==nil||original.After.Cause!=nil||!bytes.Equal(original.Before.Raw,raw)||!bytes.Equal(original.After.Raw,raw){t.Fatal("clear did not join absence with exact surviving original bytes")}
 closeIndex:=-1;absenceIndex:=-1
 for index,call:=range owner.observed.NativeCalls{
  if info,ok:=call.(*windowsInformationObservation);ok&&info.Handle==deleteHandle&&info.Owner.Kind=="clear-delete-owner"{closeIndex=index}
  if call==original.Absence{absenceIndex=index}
 }
 if closeIndex<0||absenceIndex<=closeIndex{t.Fatal("absence did not follow actual delete-owner close boundary")}
 for _,call:=range owner.observed.NativeCalls[closeIndex+1:]{if info,ok:=call.(*windowsInformationObservation);ok&&info.Handle==deleteHandle{t.Fatal("closed delete handle was probed again")}}
 // Rejoining a completed close is safe; it must issue no new native query or
 // close, even while the independent retained reader is still alive.
 calls:=len(owner.observed.NativeCalls);closes:=len(owner.observed.FileCloses)
 if err:=owner.closeFile("duplicate-delete-close",deleteOwner);err!=nil||len(owner.observed.NativeCalls)!=calls||len(owner.observed.FileCloses)!=closes{t.Fatal("cached delete close retried a stale handle")}
 if owner.remove()==nil||len(owner.observed.NativeCalls)!=calls||len(owner.observed.FileCloses)!=closes||owner.clearOriginal!=original||original.Reader.Fd()==^uintptr(0){t.Fatal("repeated clear reused original authority or lost retained reader")}
 if owner.observed.PublicationEffect!=recordClearApplied||owner.observed.CleanupRelease!=recordReleased{t.Fatal("clear effect/release were conflated")}
 if err:=owner.close();err!=nil{logRecordFailure(t,&recordStorageOutcomeError{Outcome:owner.nativeCleanupOutcome(),Cleanup:err});t.Fatal("clear owner release failed")}
 countDelete,countReader:=0,0;for _,close:=range owner.observed.FileCloses{if close.File==deleteOwner{countDelete++};if close.File==original.Reader{countReader++}}
 if countDelete!=1||countReader!=1{t.Fatal("clear originals were double-closed or lost")}
 calls=len(owner.observed.NativeCalls);if err:=owner.close();err!=nil||len(owner.observed.NativeCalls)!=calls{t.Fatal("finalizer retried a settled close")}
}
