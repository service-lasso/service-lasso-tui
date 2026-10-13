//go:build linux

package lifetime

import("runtime";"runtime/debug";"syscall";"unsafe";"golang.org/x/sys/unix")

type NativePidfdLiveness struct {Call NativeParentCall;FD unix.PollFd;Timeout unix.Timespec}
func nativePidfdLive(o *NativePidfdLiveness,fd int)error {
 if o.Call.Entered||o.Call.Returned||fd<3{return errNativeParentObservation}
 o.FD=unix.PollFd{Fd:int32(fd),Events:unix.POLLIN};o.Call.Entered=true
 o.Call.Result,o.Call.Secondary,o.Call.Errno=syscall.Syscall6(unix.SYS_PPOLL,uintptr(unsafe.Pointer(&o.FD)),1,uintptr(unsafe.Pointer(&o.Timeout)),0,0,0)
 o.Call.Returned=true;runtime.KeepAlive(o)
 if o.Call.Errno!=0{return o.Call.Errno};if o.Call.Result!=0||o.FD.Revents!=0{return errNativeParentObservation};return nil
}
// Held creator image metadata is an actual observation, not authenticated bytes
// or a loaded-image/policy/controller grant. The open view survives all borrowers.
type NativeCreatorImage struct {
 Original Original
 Proc *NativeParentProc
 proc *NativeParentProc
 View NativeBootstrapView
 Before,After NativePidfdLiveness
 ParentAfter NativeParentCall
 Metadata NativeParentCall
 Statx unix.Statx_t
 EmptyPath [1]byte
 Path [15]byte
 Digits [11]byte
 ActualRoot,ActualPID int
 Attempted,Returned,AbnormalTermination bool
 Failure error
 Panic any
 Stack []byte
}
func(r *Receiver)ObserveNativeCreatorImage(o *NativeCreatorImage)error {
 interrupted:=false
 failure:=func()error{
 if r==nil||o==nil{return errOriginalEnrollment}
 if err:=r.Enroll(&o.Original,"native-creator.held-image",o);err!=nil{return err}
 r.mutex.Lock();if r.stopped||r.joined{r.mutex.Unlock();o.Failure=errDispatchStopped;return o.Failure};r.workers.Add(1);r.mutex.Unlock();defer r.workers.Done()
 defer func(){if !o.Returned{interrupted=true;o.AbnormalTermination=true;o.Panic=recover();o.Stack=debug.Stack()}}()
 if o.Attempted{return errOriginalEnrollment};o.Attempted=true;o.proc=o.Proc;o.View.RawFD=-1;o.View.HeldFD=-1
 finish:=func(err error)error{o.Failure=err;o.Returned=true;return err}
 p:=o.proc
 if p==nil||p.Original.owner!=r||!p.Returned||p.Failure!=nil||!p.EOF||p.Exhausted||p.PID<=0||p.Root.HeldFD<3||p.parent==nil{return finish(errNativeParentObservation)}
 o.ActualRoot=p.Root.HeldFD;o.ActualPID=p.PID
 if err:=nativePidfdLive(&o.Before,p.parent.HeldDescriptor);err!=nil{return finish(err)}
 value:=uint32(o.ActualPID);at:=len(o.Digits)-1
 for {at--;o.Digits[at]=byte('0'+value%10);value/=10;if value==0{break}}
 out:=copy(o.Path[:],o.Digits[at:len(o.Digits)-1]);out+=copy(o.Path[out:],"/exe");o.Path[out]=0
 o.View.Open.Entered=true
 o.View.Open.Result,o.View.Open.Secondary,o.View.Open.Errno=syscall.Syscall(syscall.SYS_OPENAT,uintptr(o.ActualRoot),uintptr(unsafe.Pointer(&o.Path[0])),unix.O_RDONLY|unix.O_NONBLOCK|unix.O_CLOEXEC)
 o.View.Open.Returned=true;runtime.KeepAlive(o)
 if err:=nativeBootstrapLift(&o.View);err!=nil{return finish(err)}
 const mask=unix.STATX_TYPE|unix.STATX_INO|unix.STATX_SIZE|unix.STATX_MNT_ID
 o.Metadata.Entered=true
 o.Metadata.Result,o.Metadata.Secondary,o.Metadata.Errno=syscall.Syscall6(unix.SYS_STATX,uintptr(o.View.HeldFD),uintptr(unsafe.Pointer(&o.EmptyPath[0])),unix.AT_EMPTY_PATH,mask,uintptr(unsafe.Pointer(&o.Statx)),0)
 o.Metadata.Returned=true;runtime.KeepAlive(o)
 if o.Metadata.Errno!=0{return finish(o.Metadata.Errno)}
 if o.Metadata.Result!=0||o.Statx.Mask&mask!=mask||o.Statx.Mode&unix.S_IFMT!=unix.S_IFREG||o.Statx.Size==0{return finish(errNativeParentObservation)}
 if err:=nativePidfdLive(&o.After,p.parent.HeldDescriptor);err!=nil{return finish(err)}
 nativeParentCall(&o.ParentAfter,syscall.SYS_GETPPID,0,0,0)
 if o.ParentAfter.Errno!=0{return finish(o.ParentAfter.Errno)}
 if o.ParentAfter.Result!=p.AfterParent.Result{return finish(errNativeParentObservation)}
 return finish(nil)
 }()
 if interrupted{panic(o.Panic)}
 return failure
}
