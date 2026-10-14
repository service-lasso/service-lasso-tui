//go:build windows

package app

import("os";"testing";"errors";"unsafe";"golang.org/x/sys/windows")

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
func TestWindowsRenameRetainsOriginalBufferAndHeaderAlias(t *testing.T){
 directory:=t.TempDir();parent,err:=os.Open(directory);if err!=nil{t.Fatal(err)};defer parent.Close()
 file,err:=os.CreateTemp(directory,"rename-source-");if err!=nil{t.Fatal(err)};defer file.Close()
 owner:=&windowsOperationStore{observed:recordStorageOutcome{PublicationEffect:recordUnchanged,CleanupRelease:recordReleased}}
 cause:=owner.rename(windows.Handle(file.Fd()),windows.Handle(parent.Fd()),"renamed-source",false)
 var original *windowsSetInformationObservation
 for _,call:=range owner.observed.NativeCalls{if observation,ok:=call.(*windowsSetInformationObservation);ok{original=observation;break}}
 if original==nil||!original.Attempted||!original.Completed||original.Cause!=cause||original.Rename==nil||len(original.Buffer)==0||original.RequestedBytes!=uint32(len(original.Buffer))||len(original.Name16)==0{t.Fatal("actual native rename buffer/result not retained")}
 if original.Rename!=(*windowsRename)(unsafe.Pointer(&original.Buffer[0])){t.Fatal("native rename header no longer aliases original backing")}
 if owner.nativeCleanupOutcome().NativeCalls[0]!=original{t.Fatal("rename observation identity replaced")}
 // Whether the native platform permits this isolated rename is observed here;
 // neither nil nor nonnil result proves the required durable publication route.
}