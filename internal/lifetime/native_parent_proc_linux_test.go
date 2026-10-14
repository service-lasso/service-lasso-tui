//go:build linux

package lifetime

import "testing"

// UNRUN: byte-exact parsing only, no process/image/profile authority.
func TestNativeFDInfoRequiresCompleteUniquePositiveRealmPID(t *testing.T){
 good:=[]struct{body string;pid int}{{"pos:\t0\nPid:\t17\nNSpid:\t17\n",17},{"Pid: 2147483647\n",2147483647}}
 for _,fixture:=range good{pid,ok:=nativeFDInfoPID([]byte(fixture.body));if !ok||pid!=fixture.pid{t.Fatal("actual positive Pid lost")}}
 bad:=[]string{"","Pid: 17","Pid: 0\n","Pid: -1\n","Pid: +1\n","Pid: 2147483648\n","Pid: 1a\n","Pid: 1\nPid: 1\n","NSpid: 1\n","Pid:\t\n","Pid: 1\ntruncated"}
 for _,body:=range bad{if _,ok:=nativeFDInfoPID([]byte(body));ok{t.Fatal("incomplete/ambiguous fdinfo accepted")}}
}
func TestNativeParentProcMissingAssociationHasNoNativeEntry(t *testing.T){
 receiver:=NewReceiver();observation:=&NativeParentProc{}
 if receiver.ObserveNativeParentProc(observation)!=errNativeParentObservation||observation.Root.Open.Entered||observation.Info.Open.Entered||observation.ReadCount!=0{t.Fatal("missing parent reached procfs")}
 if observation.Original.Value!=observation||!observation.Returned||observation.Root.RawFD!=-1||observation.Info.HeldFD!=-1{t.Fatal("actual failed root lost")}
 receiver.Stop();receiver.Join()
}