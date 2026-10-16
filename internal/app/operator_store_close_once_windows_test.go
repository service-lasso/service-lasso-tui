//go:build windows

package app

import("os";"path/filepath";"testing";"errors";"unsafe";"golang.org/x/sys/windows")

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
