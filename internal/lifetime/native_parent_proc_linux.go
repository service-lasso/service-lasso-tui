//go:build linux

package lifetime

import("runtime";"runtime/debug";"syscall";"unsafe";"golang.org/x/sys/unix")

// Internal candidate observation scratch, never a public input allowance or fit.
const nativeBootstrapFDInfoBytes=4096
const nativeBootstrapFDInfoReads=nativeBootstrapFDInfoBytes+1

type NativeBootstrapView struct {
 Open,Lift,CloseLow,Filesystem NativeParentCall
 RawFD,HeldFD int
 Statfs unix.Statfs_t
}
// NativeParentProc retains exact fdinfo bytes and all raw calls. A proc realm PID
// is a locator only; held executable/current policy association remains separate.
type NativeParentProc struct {
 Original Original
 Parent *NativeParentObservation
 parent *NativeParentObservation
 Root,Info NativeBootstrapView
 AfterPoll,AfterParent NativeParentCall
 AfterPollFD unix.PollFd
 AfterTimeout unix.Timespec
 ProcPath [6]byte
 InfoPath [30]byte
 Digits [11]byte
 Body [nativeBootstrapFDInfoBytes]byte
 Reads [nativeBootstrapFDInfoReads]NativeParentCall
 ReadOffsets,ReadExtents [nativeBootstrapFDInfoReads]int
 Length,ReadCount int
 PID int
 Attempted,Returned,EOF,Exhausted,AbnormalTermination bool
 Failure error
 Panic any
 Stack []byte
}
func nativeBootstrapLift(v *NativeBootstrapView)error {
 if v.Open.Errno!=0{return v.Open.Errno}
 if v.Open.Result>uintptr(^uint32(0)>>1){return errNativeParentObservation}
 v.RawFD=int(v.Open.Result);v.HeldFD=v.RawFD
 if v.RawFD>=3{return nil}
 nativeParentCall(&v.Lift,syscall.SYS_FCNTL,uintptr(v.RawFD),syscall.F_DUPFD_CLOEXEC,3)
 if v.Lift.Errno!=0{return v.Lift.Errno}
 if v.Lift.Result<3||v.Lift.Result>uintptr(^uint32(0)>>1){return errNativeParentObservation}
 v.HeldFD=int(v.Lift.Result)
 nativeParentCall(&v.CloseLow,syscall.SYS_CLOSE,uintptr(v.RawFD),0,0)
 if v.CloseLow.Errno!=0{return v.CloseLow.Errno}
 if v.CloseLow.Result!=0{return errNativeParentObservation};return nil
}
func nativeBootstrapProcfs(v *NativeBootstrapView)error {
 v.Filesystem.Entered=true
 v.Filesystem.Result,v.Filesystem.Secondary,v.Filesystem.Errno=syscall.Syscall(syscall.SYS_FSTATFS,uintptr(v.HeldFD),uintptr(unsafe.Pointer(&v.Statfs)),0)
 v.Filesystem.Returned=true;runtime.KeepAlive(v)
 if v.Filesystem.Errno!=0{return v.Filesystem.Errno}
 if v.Filesystem.Result!=0||v.Statfs.Type!=unix.PROC_SUPER_MAGIC{return errNativeParentObservation};return nil
}
func nativeFDInfoPID(body []byte)(int,bool){
 pid:=0;seen:=false
 for at:=0;at<len(body);{
  start:=at;for at<len(body)&&body[at]!='\n'{at++};if at==len(body){return 0,false};end:=at;at++
  if end-start<4||body[start]!='P'||body[start+1]!='i'||body[start+2]!='d'||body[start+3]!=':'{continue}
  if seen{return 0,false};seen=true;i:=start+4
  for i<end&&(body[i]==' '||body[i]=='\t'){i++};first:=i
  for i<end&&body[i]>='0'&&body[i]<='9'{digit:=int(body[i]-'0');if pid>(int(^uint32(0)>>1)-digit)/10{return 0,false};pid=pid*10+digit;i++}
  if first==i{return 0,false};for i<end&&(body[i]==' '||body[i]=='\t'){i++};if i!=end{return 0,false}
 }
 return pid,seen&&pid>0
}
func(r *Receiver)ObserveNativeParentProc(o *NativeParentProc)error {
 interrupted:=false
 failure:=func()error{
 if r==nil||o==nil{return errOriginalEnrollment}
 if err:=r.Enroll(&o.Original,"native-creator.proc-fdinfo",o);err!=nil{return err}
 r.mutex.Lock();if r.stopped||r.joined{r.mutex.Unlock();o.Failure=errDispatchStopped;return o.Failure};r.workers.Add(1);r.mutex.Unlock();defer r.workers.Done()
 defer func(){if !o.Returned{interrupted=true;o.AbnormalTermination=true;o.Panic=recover();o.Stack=debug.Stack()}}()
 if o.Attempted{return errOriginalEnrollment};o.Attempted=true;o.parent=o.Parent
 o.Root.RawFD=-1;o.Root.HeldFD=-1;o.Info.RawFD=-1;o.Info.HeldFD=-1
 finish:=func(err error)error{o.Failure=err;o.Returned=true;return err}
 p:=o.parent
 if p==nil||p.Original.owner!=r||!p.Returned||p.Failure!=nil||!p.Poll.Returned||p.Poll.Errno!=0||p.Poll.Result!=0||p.PollFD.Revents!=0||p.HeldDescriptor<3{return finish(errNativeParentObservation)}
 copy(o.ProcPath[:],"/proc\x00")
 directoryLocator:=unix.AT_FDCWD
 o.Root.Open.Entered=true
 o.Root.Open.Result,o.Root.Open.Secondary,o.Root.Open.Errno=syscall.Syscall(syscall.SYS_OPENAT,uintptr(directoryLocator),uintptr(unsafe.Pointer(&o.ProcPath[0])),unix.O_RDONLY|unix.O_DIRECTORY|unix.O_NONBLOCK|unix.O_CLOEXEC)
 o.Root.Open.Returned=true;runtime.KeepAlive(o)
 if err:=nativeBootstrapLift(&o.Root);err!=nil{return finish(err)}
 if err:=nativeBootstrapProcfs(&o.Root);err!=nil{return finish(err)}
 value:=uint32(p.HeldDescriptor);at:=len(o.Digits)-1
 for {at--;o.Digits[at]=byte('0'+value%10);value/=10;if value==0{break}}
 out:=copy(o.InfoPath[:],"thread-self/fdinfo/");out+=copy(o.InfoPath[out:],o.Digits[at:len(o.Digits)-1]);o.InfoPath[out]=0
 o.Info.Open.Entered=true
 o.Info.Open.Result,o.Info.Open.Secondary,o.Info.Open.Errno=syscall.Syscall(syscall.SYS_OPENAT,uintptr(o.Root.HeldFD),uintptr(unsafe.Pointer(&o.InfoPath[0])),unix.O_RDONLY|unix.O_NONBLOCK|unix.O_CLOEXEC|unix.O_NOFOLLOW)
 o.Info.Open.Returned=true;runtime.KeepAlive(o)
 if err:=nativeBootstrapLift(&o.Info);err!=nil{return finish(err)}
 if err:=nativeBootstrapProcfs(&o.Info);err!=nil{return finish(err)}
 for !o.EOF{
  if o.ReadCount==len(o.Reads)||o.Length==len(o.Body){o.Exhausted=true;return finish(errNativeParentObservation)}
  index:=o.ReadCount;call:=&o.Reads[index];o.ReadCount++;remaining:=len(o.Body)-o.Length;o.ReadOffsets[index]=o.Length;o.ReadExtents[index]=remaining;call.Entered=true
  call.Result,call.Secondary,call.Errno=syscall.Syscall(syscall.SYS_READ,uintptr(o.Info.HeldFD),uintptr(unsafe.Pointer(&o.Body[o.Length])),uintptr(remaining))
  call.Returned=true;runtime.KeepAlive(o)
  if call.Errno!=0{return finish(call.Errno)}
  if call.Result>uintptr(remaining){return finish(errNativeParentObservation)}
  if call.Result==0{o.EOF=true}else{o.Length+=int(call.Result)}
 }
 pid,ok:=nativeFDInfoPID(o.Body[:o.Length]);o.PID=pid;if !ok{return finish(errNativeParentObservation)}
 o.AfterPollFD=unix.PollFd{Fd:int32(p.HeldDescriptor),Events:unix.POLLIN};o.AfterPoll.Entered=true
 o.AfterPoll.Result,o.AfterPoll.Secondary,o.AfterPoll.Errno=syscall.Syscall6(unix.SYS_PPOLL,uintptr(unsafe.Pointer(&o.AfterPollFD)),1,uintptr(unsafe.Pointer(&o.AfterTimeout)),0,0,0)
 o.AfterPoll.Returned=true;runtime.KeepAlive(o)
 if o.AfterPoll.Errno!=0{return finish(o.AfterPoll.Errno)}
 if o.AfterPoll.Result!=0||o.AfterPollFD.Revents!=0{return finish(errNativeParentObservation)}
 nativeParentCall(&o.AfterParent,syscall.SYS_GETPPID,0,0,0)
 if o.AfterParent.Errno!=0{return finish(o.AfterParent.Errno)}
 if o.AfterParent.Result!=p.After.Result{return finish(errNativeParentObservation)}
 return finish(nil)
 // Root/info/raw/lifted views remain retained for actual image/profile borrowers.
 // Neither parse success nor returned observation authorizes native close/disposal.
 }()
 if interrupted{panic(o.Panic)}
 return failure
}
