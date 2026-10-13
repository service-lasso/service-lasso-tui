//go:build linux

package lifetime

import "testing"

// UNRUN: actual rejection bodies, no native/SELinux authority claim.
func TestProcessContextAndImageLabelRequireTheirOwnOriginalsBeforeIO(t *testing.T){
 receiver:=NewReceiver()
 for _,parent:=range []bool{false,true}{
  context:=&NativeProcessContext{Parent:parent}
  if receiver.ObserveNativeProcessContext(context)!=errNativeParentObservation||context.View.Open.Entered||context.PIDBefore.Entered||context.ReadCount!=0{t.Fatal("missing process relation reached native read")}
  if context.Original.Value!=context||!context.Returned||context.View.RawFD!=-1||context.View.HeldFD!=-1{t.Fatal("actual process context root lost")}
 }
 label:=&NativeImageLabel{}
 if receiver.ObserveNativeImageLabel(label)!=errNativeParentObservation||label.Query.Entered||label.Length!=0||label.Original.Value!=label||!label.Returned{t.Fatal("missing held image reached label query")}
 receiver.Stop();receiver.Join()
}