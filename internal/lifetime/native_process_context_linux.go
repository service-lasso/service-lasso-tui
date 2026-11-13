//go:build linux

package lifetime

import("runtime";"runtime/debug";"syscall";"unsafe";"golang.org/x/sys/unix")

// Separate raw proc attr/current originals; none is the peer SOCKET or image label.
// Actual deployed LSM dispatch must bind this attribute to SELinux before policy
// interpretation. A successful legacy attr read does not establish that binding.
type NativeProcessContext struct {
 Original Original
 Proc *NativeParentProc
 proc *NativeParentProc
 Parent bool
 parent bool
 View NativeBootstrapView
 Before,After NativePidfdLiveness
 PIDBefore,PIDAfter NativeParentCall
 Path [24]byte
 Digits [11]byte
 Body [nativeBootstrapFDInfoBytes]byte
 Reads [nativeBootstrapFDInfoReads]NativeParentCall
 Offsets,Extents [nativeBootstrapFDInfoReads]int
 Length,ReadCount int
 Attempted,Returned,EOF,Exhausted,AbnormalTermination bool
 Failure error
 Panic any
 Stack []byte
}
func(r *Receiver)ObserveNativeProcessContext(o *NativeProcessContext)error {
 interrupted:=false
 failure:=func()error{
 if r==nil||o==nil{return errOriginalEnrollment}
 if err:=r.Enroll(&o.Original,"native-creator.process-context",o);err!=nil{return err}
 r.mutex.Lock();if r.stopped||r.joined{r.mutex.Unlock();o.Failure=errDispatchStopped;return o.Failure};r.workers.Add(1);r.mutex.Unlock();defer r.workers.Done()
 defer func(){if !o.Returned{interrupted=true;o.AbnormalTermination=true;o.Panic=recover();o.Stack=debug.Stack()}}()
 if o.Attempted{return errOriginalEnrollment};o.Attempted=true;o.proc=o.Proc;o.parent=o.Parent;o.View.RawFD=-1;o.View.HeldFD=-1
 finish:=func(err error)error{o.Failure=err;o.Returned=true;return err}
 p:=o.proc
 if p==nil||p.Original.owner!=r||!p.Returned||p.Failure!=nil||!p.EOF||p.Exhausted||p.PID<=0||p.Root.HeldFD<3||p.parent==nil{return finish(errNativeParentObservation)}
 if o.parent{
  if err:=nativePidfdLive(&o.Before,p.parent.HeldDescriptor);err!=nil{return finish(err)}
  nativeParentCall(&o.PIDBefore,syscall.SYS_GETPPID,0,0,0)
  if o.PIDBefore.Errno!=0{return finish(o.PIDBefore.Errno)}
  if o.PIDBefore.Result!=p.AfterParent.Result{return finish(errNativeParentObservation)}
  value:=uint32(p.PID);at:=len(o.Digits)-1
  for{at--;o.Digits[at]=byte('0'+value%10);value/=10;if value==0{break}}
  out:=copy(o.Path[:],o.Digits[at:len(o.Digits)-1]);out+=copy(o.Path[out:],"/attr/current");o.Path[out]=0
 }else{
  nativeParentCall(&o.PIDBefore,syscall.SYS_GETPID,0,0,0)
  if o.PIDBefore.Errno!=0{return finish(o.PIDBefore.Errno)}
  if o.PIDBefore.Result==0{return finish(errNativeParentObservation)}
  copy(o.Path[:],"self/attr/current\x00")
 }
 o.View.Open.Entered=true
 o.View.Open.Result,o.View.Open.Secondary,o.View.Open.Errno=syscall.Syscall(syscall.SYS_OPENAT,uintptr(p.Root.HeldFD),uintptr(unsafe.Pointer(&o.Path[0])),unix.O_RDONLY|unix.O_NONBLOCK|unix.O_CLOEXEC|unix.O_NOFOLLOW)
 o.View.Open.Returned=true;runtime.KeepAlive(o)
 if err:=nativeBootstrapLift(&o.View);err!=nil{return finish(err)}
 if err:=nativeBootstrapProcfs(&o.View);err!=nil{return finish(err)}
 for !o.EOF{
  if o.ReadCount==len(o.Reads)||o.Length==len(o.Body){o.Exhausted=true;return finish(errNativeParentObservation)}
  index:=o.ReadCount;call:=&o.Reads[index];o.ReadCount++;remaining:=len(o.Body)-o.Length;o.Offsets[index]=o.Length;o.Extents[index]=remaining;call.Entered=true
  call.Result,call.Secondary,call.Errno=syscall.Syscall(syscall.SYS_READ,uintptr(o.View.HeldFD),uintptr(unsafe.Pointer(&o.Body[o.Length])),uintptr(remaining))
  call.Returned=true;runtime.KeepAlive(o)
  if call.Errno!=0{return finish(call.Errno)}
  if call.Result>uintptr(remaining){return finish(errNativeParentObservation)}
  if call.Result==0{o.EOF=true}else{o.Length+=int(call.Result)}
 }
 if o.Length==0{return finish(errNativeParentObservation)}
 if o.parent{
  if err:=nativePidfdLive(&o.After,p.parent.HeldDescriptor);err!=nil{return finish(err)}
  nativeParentCall(&o.PIDAfter,syscall.SYS_GETPPID,0,0,0)
 }else{nativeParentCall(&o.PIDAfter,syscall.SYS_GETPID,0,0,0)}
 if o.PIDAfter.Errno!=0{return finish(o.PIDAfter.Errno)}
 if o.PIDAfter.Result!=o.PIDBefore.Result{return finish(errNativeParentObservation)}
 return finish(nil)
 // Context syntax/policy/epoch/actor/exec interpretation is a separate actual
 // selected native policy source relation, not a string comparison here.
 }()
 if interrupted{panic(o.Panic)}
 return failure
}

type NativeImageLabel struct {
 Original Original
 Image *NativeCreatorImage
 image *NativeCreatorImage
 Name [17]byte
 Body [nativeBootstrapFDInfoBytes]byte
 Query NativeParentCall
 Length int
 Attempted,Returned,AbnormalTermination bool
 Failure error
 Panic any
 Stack []byte
}
func(r *Receiver)ObserveNativeImageLabel(o *NativeImageLabel)error {
 interrupted:=false
 failure:=func()error{
 if r==nil||o==nil{return errOriginalEnrollment}
 if err:=r.Enroll(&o.Original,"native-creator.image-label",o);err!=nil{return err}
 r.mutex.Lock();if r.stopped||r.joined{r.mutex.Unlock();o.Failure=errDispatchStopped;return o.Failure};r.workers.Add(1);r.mutex.Unlock();defer r.workers.Done()
 defer func(){if !o.Returned{interrupted=true;o.AbnormalTermination=true;o.Panic=recover();o.Stack=debug.Stack()}}()
 if o.Attempted{return errOriginalEnrollment};o.Attempted=true;o.image=o.Image
 finish:=func(err error)error{o.Failure=err;o.Returned=true;return err}
 i:=o.image
 if i==nil||i.Original.owner!=r||!i.Returned||i.Failure!=nil||i.View.HeldFD<3||!i.Metadata.Returned||i.Metadata.Errno!=0{return finish(errNativeParentObservation)}
 copy(o.Name[:],"security.selinux\x00");o.Query.Entered=true
 o.Query.Result,o.Query.Secondary,o.Query.Errno=syscall.Syscall6(syscall.SYS_FGETXATTR,uintptr(i.View.HeldFD),uintptr(unsafe.Pointer(&o.Name[0])),uintptr(unsafe.Pointer(&o.Body[0])),uintptr(len(o.Body)),0,0)
 o.Query.Returned=true;runtime.KeepAlive(o)
 if o.Query.Errno!=0{return finish(o.Query.Errno)}
 if o.Query.Result==0||o.Query.Result>uintptr(len(o.Body)){return finish(errNativeParentObservation)}
 o.Length=int(o.Query.Result);return finish(nil)
 }()
 if interrupted{panic(o.Panic)}
 return failure
}
