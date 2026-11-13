//go:build linux

package lifetime

import (
 "errors"
 "runtime"
 "runtime/debug"
 "syscall"
 "unsafe"
 "golang.org/x/sys/unix"
)

var errNativeParentObservation=errors.New("native creator parent observation invalid")
type NativeParentCall struct {Result,Secondary uintptr; Errno syscall.Errno; Entered,Returned bool}
// NativeParentObservation uses embedded fixed cells before quantity-sized backing.
// Parent equality and an unsignaled pidfd are observations, never image/policy authority.
type NativeParentObservation struct {
 Original Original
 Socket *EndpointSocketObservation
 socket *EndpointSocketObservation
 Before,Open,Lift,CloseLow,Poll,After NativeParentCall
 PollFD unix.PollFd
 PollTimeout unix.Timespec
 RawDescriptor,HeldDescriptor int
 Attempted,Returned bool
 AbnormalTermination bool
 Failure error
 Panic any
 Stack []byte
}
func nativeParentCall(c *NativeParentCall,number,a,b,d uintptr){
 c.Entered=true;c.Result,c.Secondary,c.Errno=syscall.Syscall(number,a,b,d);c.Returned=true
}
func(r *Receiver)ObserveNativeParent(o *NativeParentObservation)error{
 interrupted:=false
 failure:=func()error{
 if r==nil||o==nil{return errOriginalEnrollment}
 if err:=r.Enroll(&o.Original,"native-creator.parent",o);err!=nil{return err}
 r.mutex.Lock();if r.stopped||r.joined{r.mutex.Unlock();o.Failure=errDispatchStopped;return o.Failure};r.workers.Add(1);r.mutex.Unlock()
 defer r.workers.Done()
 defer func(){if !o.Returned{interrupted=true;o.AbnormalTermination=true;o.Panic=recover();o.Stack=debug.Stack()}}()
 if o.Attempted{return errOriginalEnrollment};o.Attempted=true;o.socket=o.Socket;o.RawDescriptor=-1;o.HeldDescriptor=-1
 finish:=func(err error)error{o.Failure=err;o.Returned=true;return err}
 s:=o.socket
 if s==nil||s.Original.owner!=r||!s.CredentialReturned||s.CredentialErrno!=0||s.CredentialReturn!=0||s.CredentialLength!=uint32(unsafe.Sizeof(s.Credentials))||s.creatorCredentials.Pid<=0{return finish(errNativeParentObservation)}
 nativeParentCall(&o.Before,syscall.SYS_GETPPID,0,0,0)
 if o.Before.Errno!=0{return finish(o.Before.Errno)}
 if o.Before.Result!=uintptr(s.creatorCredentials.Pid){return finish(errNativeParentObservation)}
 nativeParentCall(&o.Open,unix.SYS_PIDFD_OPEN,uintptr(s.creatorCredentials.Pid),0,0)
 if o.Open.Errno!=0{return finish(o.Open.Errno)}
 if o.Open.Result>uintptr(^uint32(0)>>1){return finish(errNativeParentObservation)}
 o.RawDescriptor=int(o.Open.Result);o.HeldDescriptor=o.RawDescriptor
 if o.RawDescriptor<3{
  nativeParentCall(&o.Lift,syscall.SYS_FCNTL,uintptr(o.RawDescriptor),syscall.F_DUPFD_CLOEXEC,3)
  if o.Lift.Errno!=0{return finish(o.Lift.Errno)}
  if o.Lift.Result<3||o.Lift.Result>uintptr(^uint32(0)>>1){return finish(errNativeParentObservation)}
  o.HeldDescriptor=int(o.Lift.Result)
  // Never retry an uncertain close; both originals stay reachable.
  nativeParentCall(&o.CloseLow,syscall.SYS_CLOSE,uintptr(o.RawDescriptor),0,0)
  if o.CloseLow.Errno!=0{return finish(o.CloseLow.Errno)}
  if o.CloseLow.Result!=0{return finish(errNativeParentObservation)}
 }
 o.PollFD=unix.PollFd{Fd:int32(o.HeldDescriptor),Events:unix.POLLIN}
 o.Poll.Entered=true
 o.Poll.Result,o.Poll.Secondary,o.Poll.Errno=syscall.Syscall6(unix.SYS_PPOLL,uintptr(unsafe.Pointer(&o.PollFD)),1,uintptr(unsafe.Pointer(&o.PollTimeout)),0,0,0)
 o.Poll.Returned=true
 runtime.KeepAlive(o)
 if o.Poll.Errno!=0{return finish(o.Poll.Errno)}
 if o.Poll.Result!=0||o.PollFD.Revents!=0{return finish(errNativeParentObservation)}
 nativeParentCall(&o.After,syscall.SYS_GETPPID,0,0,0)
 if o.After.Errno!=0{return finish(o.After.Errno)}
 if o.After.Result!=o.Before.Result{return finish(errNativeParentObservation)}
 return finish(nil)
 }()
 if interrupted{panic(o.Panic)}
 return failure
}
