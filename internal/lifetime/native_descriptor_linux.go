//go:build linux

package lifetime

import (
 "errors"
 "io"
 "runtime"
 "runtime/debug"
 "sync"
 "syscall"
 "unsafe"
)

var errNativeEndpointCapacity=errors.New("native endpoint IO backing exhausted")
var errNativeEndpointExtent=errors.New("native endpoint raw IO extent invalid")

// NativeEndpoint is the actual inherited descriptor owner. Its closed native
// inheritance/creator/profile relation is an independent caller obligation.
// The fixed syscall roster is prepared before terminal/custody producers start.
type NativeEndpoint struct {
 Original Original
 Descriptor int
 Calls []NativeEndpointIO
 Exhaustion NativeEndpointIO
 mutex sync.Mutex
 receiver *Receiver
 descriptor int
 calls []NativeEndpointIO
 count int
 prepared,exhausted bool
 binding *nativeTransferEndpoint
 Failure error
}
type NativeEndpointIO struct {
 Original Original
 Endpoint *NativeEndpoint
 Transfer *LocalTransfer
 Operation,Flags uintptr
 Buffer []byte
 RawReturn,Secondary uintptr
 Errno syscall.Errno
 Count int
 Failure error
 Panic any
 Stack []byte
 Entered,SyscallReturned,Returned,Interrupted bool
}
func(r *Receiver)PrepareNativeEndpoint(endpoint *NativeEndpoint)error {
 if r==nil||endpoint==nil{return errOriginalEnrollment}
 if err:=r.Enroll(&endpoint.Original,"native-endpoint.descriptor",endpoint);err!=nil{return err}
 endpoint.receiver=r;endpoint.descriptor=endpoint.Descriptor;endpoint.calls=endpoint.Calls
 if endpoint.descriptor<3||len(endpoint.calls)==0{return errOriginalEnrollment}
 for index:=range endpoint.calls{
  call:=&endpoint.calls[index]
  if call.Endpoint!=nil||call.Entered||call.Returned||call.Buffer!=nil{return errOriginalEnrollment}
  call.Endpoint=endpoint
  if err:=r.Enroll(&call.Original,"native-endpoint.preowned-io",call);err!=nil{return err}
 }
 endpoint.Exhaustion.Endpoint=endpoint
 if err:=r.Enroll(&endpoint.Exhaustion.Original,"native-endpoint.first-exhaustion",&endpoint.Exhaustion);err!=nil{return err}
 endpoint.prepared=true;return nil
}
func(endpoint *NativeEndpoint)Read(buffer []byte)(int,error){return endpoint.invoke(syscall.SYS_READ,buffer,nil)}
func(endpoint *NativeEndpoint)Write(buffer []byte)(int,error){return endpoint.invoke(syscall.SYS_SENDTO,buffer,nil)}
func(endpoint *NativeEndpoint)invoke(operation uintptr,buffer []byte,transfer *LocalTransfer)(count int,failure error){
 if endpoint==nil{return 0,errOriginalEnrollment}
 endpoint.mutex.Lock();defer endpoint.mutex.Unlock()
 if !endpoint.prepared{return 0,errOriginalEnrollment}
 if endpoint.exhausted{return 0,endpoint.Failure}
 if endpoint.count==len(endpoint.calls){
  endpoint.exhausted=true;endpoint.Failure=errNativeEndpointCapacity
  endpoint.Exhaustion.Operation=operation;endpoint.Exhaustion.Buffer=buffer;endpoint.Exhaustion.Transfer=transfer;endpoint.Exhaustion.Failure=endpoint.Failure;endpoint.Exhaustion.Returned=true
  return 0,endpoint.Failure
 }
 call:=&endpoint.calls[endpoint.count];endpoint.count++
 call.Operation=operation;call.Buffer=buffer;call.Transfer=transfer
 receiver:=endpoint.receiver;late:=false
 receiver.mutex.Lock()
 if receiver.joined{
  if receiver.inspections!=0||transfer==nil||transfer.Original.owner!=receiver||!transfer.exchanging||endpoint.binding==nil||endpoint.binding.transfer!=transfer||transfer.preparedEndpoint!=endpoint.binding{receiver.mutex.Unlock();call.Failure=errOriginalEnrollment;call.Returned=true;return 0,call.Failure}
  receiver.terminalIO++;late=true
 }else{
  if receiver.stopped{receiver.mutex.Unlock();call.Failure=errDispatchStopped;call.Returned=true;return 0,call.Failure}
  receiver.workers.Add(1)
 }
 receiver.mutex.Unlock()
 defer func(){if late{receiver.mutex.Lock();receiver.terminalIO--;receiver.mutex.Unlock()}else{receiver.workers.Done()}}()
 if operation==syscall.SYS_SENDTO{call.Flags=syscall.MSG_NOSIGNAL}
 interrupted:=false
 func(){
 defer func(){if !call.Returned{interrupted=true;call.Interrupted=true;call.Panic=recover();call.Stack=debug.Stack()}}()
 var pointer unsafe.Pointer;if len(buffer)>0{pointer=unsafe.Pointer(&buffer[0])}
 call.Entered=true
 call.RawReturn,call.Secondary,call.Errno=syscall.Syscall6(operation,uintptr(endpoint.descriptor),uintptr(pointer),uintptr(len(buffer)),call.Flags,0,0)
 call.SyscallReturned=true
 // Raw errno/count is retained before conversion, EOF or invalid-extent projection.
 runtime.KeepAlive(pointer);runtime.KeepAlive(buffer);runtime.KeepAlive(endpoint)
 if call.Errno!=0{call.Failure=call.Errno}else if call.RawReturn>uintptr(len(buffer)){call.Failure=errNativeEndpointExtent}else{
  call.Count=int(call.RawReturn)
  if operation==syscall.SYS_READ&&len(buffer)>0&&call.Count==0{call.Failure=io.EOF}
 }
 call.Returned=true;count=call.Count;failure=call.Failure
 }()
 if interrupted{panic(call.Panic)}
 return count,failure
}
// This SAME preowned wrapper links the selected late exchange to its raw owner.
// Generic endpoint Read/Write cannot admit a new producer after Receiver.Join.
type nativeTransferEndpoint struct{Original Original;endpoint *NativeEndpoint;transfer *LocalTransfer}
func(endpoint *NativeEndpoint)bindLocalTransfer(receiver *Receiver,transfer *LocalTransfer)(io.ReadWriter,error){
 if endpoint==nil||receiver==nil||endpoint.Original.owner!=receiver||!endpoint.prepared{return nil,errOriginalEnrollment}
 bound:=&nativeTransferEndpoint{endpoint:endpoint,transfer:transfer}
 if err:=receiver.Enroll(&bound.Original,"native-endpoint.local-transfer-binding",bound);err!=nil{return nil,err}
 endpoint.mutex.Lock();defer endpoint.mutex.Unlock()
 if endpoint.binding!=nil{return nil,errOriginalEnrollment};endpoint.binding=bound
 return bound,nil
}
func(bound *nativeTransferEndpoint)Read(buffer []byte)(int,error){return bound.endpoint.invoke(syscall.SYS_READ,buffer,bound.transfer)}
func(bound *nativeTransferEndpoint)Write(buffer []byte)(int,error){return bound.endpoint.invoke(syscall.SYS_SENDTO,buffer,bound.transfer)}
