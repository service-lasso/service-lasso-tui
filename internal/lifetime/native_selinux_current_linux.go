//go:build linux

package lifetime

import("encoding/binary";"errors";"runtime";"runtime/debug";"syscall";"unsafe")
var errNativeSELinuxCurrent=errors.New("native SELinux current observation invalid")
// The fixed original record is independent of untrusted locator quantities.
// Generic proc attr/current observations never substitute for this explicit LSM.
type NativeSELinuxCurrent struct {
 Original Original
 Before,Query,After NativeParentCall
 Number uintptr
 Arguments [6]uintptr
 SuppliedSize,ReportedSize uint32
 Header [4]uint64
 Body [nativeBootstrapFDInfoBytes]byte
 Context []byte
 Examined,PaddingExamined int
 LockEntered,LockReturned,UnlockEntered,UnlockReturned bool
 Attempted,Returned,FramingComplete,Exhausted,AbnormalTermination bool
 Failure error
 Panic any
 Stack []byte
}
func(r *Receiver)ObserveNativeSELinuxCurrent(o *NativeSELinuxCurrent)error{
 interrupted:=false
 failure:=func()error{
 if r==nil||o==nil{return errOriginalEnrollment}
 if err:=r.Enroll(&o.Original,"native-payload.selinux-current",o);err!=nil{return err}
 r.mutex.Lock();if r.stopped||r.joined{r.mutex.Unlock();o.Failure=errDispatchStopped;return o.Failure};r.workers.Add(1);r.mutex.Unlock();defer r.workers.Done()
 completed:=false
 defer func(){if completed&&(!o.LockReturned||o.UnlockReturned){o.Returned=true;return};if !o.Returned{interrupted=true;o.AbnormalTermination=true;o.Panic=recover();o.Stack=debug.Stack()}}()
 if o.Attempted{return errOriginalEnrollment};o.Attempted=true
 finish:=func(err error)error{o.Failure=err;completed=true;return err}
 if runtime.GOARCH!="amd64"{return finish(syscall.ENOSYS)}
 if uintptr(unsafe.Pointer(&o.Body[0]))%8!=0||o.Context!=nil||o.SuppliedSize!=0||o.ReportedSize!=0{return finish(errNativeSELinuxCurrent)}
 for _,value:=range o.Body{if value!=0{return finish(errNativeSELinuxCurrent)}}
 binary.LittleEndian.PutUint64(o.Body[:8],101);o.SuppliedSize=uint32(len(o.Body));o.ReportedSize=o.SuppliedSize
 o.LockEntered=true;runtime.LockOSThread();o.LockReturned=true
 defer func(){o.UnlockEntered=true;runtime.UnlockOSThread();o.UnlockReturned=true}()
 nativeParentCall(&o.Before,syscall.SYS_GETTID,0,0,0)
 if o.Before.Errno!=0{return finish(o.Before.Errno)};if o.Before.Result==0{return finish(errNativeSELinuxCurrent)}
 o.Number=459;o.Arguments=[6]uintptr{100,uintptr(unsafe.Pointer(&o.Body[0])),uintptr(unsafe.Pointer(&o.ReportedSize)),1,0,0}
 o.Query.Entered=true;o.Query.Result,o.Query.Secondary,o.Query.Errno=syscall.Syscall6(o.Number,100,uintptr(unsafe.Pointer(&o.Body[0])),uintptr(unsafe.Pointer(&o.ReportedSize)),1,0,0);o.Query.Returned=true
 nativeParentCall(&o.After,syscall.SYS_GETTID,0,0,0);runtime.KeepAlive(o)
 for i:=range o.Header{o.Header[i]=binary.LittleEndian.Uint64(o.Body[i*8:i*8+8])}
 if o.Query.Errno!=0{if o.Query.Errno==syscall.E2BIG{o.Exhausted=true};return finish(o.Query.Errno)}
 if o.After.Errno!=0{return finish(o.After.Errno)}
 if o.Query.Result!=1||o.After.Result!=o.Before.Result||o.ReportedSize>o.SuppliedSize||o.ReportedSize<34{return finish(errNativeSELinuxCurrent)}
 size:=uint64(o.ReportedSize);ctx:=o.Header[3]
 if o.Header[0]!=101||o.Header[1]!=0||o.Header[2]!=size||size%8!=0||ctx<2||ctx>size-32||size-(32+ctx)>=8{return finish(errNativeSELinuxCurrent)}
 end:=int(32+ctx)
 for i:=32;i<end;i++{o.Examined++;if (i+1==end)!=(o.Body[i]==0){return finish(errNativeSELinuxCurrent)}}
 for i:=end;i<int(size);i++{o.PaddingExamined++;if o.Body[i]!=0{return finish(errNativeSELinuxCurrent)}}
 o.Context=o.Body[32:end-1:end-1];o.FramingComplete=true;return finish(nil)
 // Context/peer/code/policy correspondence is a separate source consumer.
 }()
 if interrupted{panic(o.Panic)}
 return failure
}
