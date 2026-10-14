//go:build linux

package lifetime

import "testing"

// UNRUN: source rejection routes, not actual native creator authentication.
func TestCreatorImageMissingOrFailedProcCannotOpen(t *testing.T){
 receiver:=NewReceiver();failed:=&NativeParentProc{}
 if err:=receiver.Enroll(&failed.Original,"fixture.failed-parent-proc",failed);err!=nil{t.Fatal(err)}
 failed.Returned=true;failed.Failure=errNativeParentObservation
 for _,proc:=range []*NativeParentProc{nil,failed}{
  image:=&NativeCreatorImage{Proc:proc}
  if receiver.ObserveNativeCreatorImage(image)!=errNativeParentObservation||image.Before.Call.Entered||image.View.Open.Entered||image.Metadata.Entered||image.After.Call.Entered{t.Fatal("unassociated creator reached native image observation")}
  if image.Original.Value!=image||!image.Returned||image.View.RawFD!=-1||image.View.HeldFD!=-1{t.Fatal("actual image failure root lost")}
 }
 receiver.Stop();receiver.Join()
}