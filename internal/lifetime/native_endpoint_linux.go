//go:build linux

package lifetime

import (
 "errors"
 "os"
 "runtime"
 "runtime/debug"
 "syscall"
 "unsafe"
)

// EndpointSocketObservation retains the local syscall originals. Credentials
// describe socket creation, and Security describes the peer SOCKET label.
// Neither supplies native birth, current process-domain or disposition authority.
type EndpointSocketObservation struct {
 Original Original
 Endpoint *os.File
 Native *NativeEndpoint
 Security []byte
 endpoint *os.File
 native *NativeEndpoint
 locatorOnly bool
 locator int
 security []byte
 Descriptor uintptr
 Credentials syscall.Ucred
 creatorCredentials syscall.Ucred
 CredentialLength,SecurityLength uint32
 CredentialReturn,CredentialSecondary uintptr
 CredentialErrno syscall.Errno
 SecurityReturn,SecuritySecondary uintptr
 SecurityErrno syscall.Errno
 CredentialEntered,CredentialReturned,SecurityEntered,SecurityReturned bool
 Attempted,Returned,Interrupted bool
 Failure error
 Panic any
 Stack []byte
}
var errEndpointObservation=errors.New("native endpoint socket observation invalid")

// ObserveEndpointSocket must precede endpoint IO. A supplied finite label buffer
// is frozen before effects; an insufficient buffer retains ERANGE and its actual
// required length without allocating a replacement or retrying the same stream.
func(r *Receiver)ObserveEndpointSocket(o *EndpointSocketObservation)error {return r.observeEndpointSocket(o,true,false,0)}
// ObserveEndpointCreator uses only embedded fixed credentials before any label allocation.
func(r *Receiver)ObserveEndpointCreator(o *EndpointSocketObservation)error {return r.observeEndpointSocket(o,false,false,0)}
func(r *Receiver)ObserveEndpointCreatorDescriptor(o *EndpointSocketObservation,descriptor int)error {
 return r.observeEndpointSocket(o,false,true,descriptor)
}
func(r *Receiver)observeEndpointSocket(o *EndpointSocketObservation,includeSecurity,locatorOnly bool,descriptor int)error {
 interrupted:=false
 failure:=func()error{
 if r==nil||o==nil{return errOriginalEnrollment}
 if err:=r.Enroll(&o.Original,"native-endpoint.socket-observation",o);err!=nil{return err}
 r.mutex.Lock()
 if r.stopped||r.joined{r.mutex.Unlock();o.Failure=errDispatchStopped;return o.Failure}
 r.workers.Add(1);r.mutex.Unlock()
 defer r.workers.Done()
 defer func(){if !o.Returned{interrupted=true;o.Interrupted=true;o.Panic=recover();o.Stack=debug.Stack()}}()
 if o.Attempted{return errOriginalEnrollment};o.Attempted=true;o.locatorOnly=locatorOnly;o.locator=descriptor
 o.endpoint=o.Endpoint;o.native=o.Native;o.security=o.Security
 if ((!o.locatorOnly&&(o.endpoint==nil)==(o.native==nil))||(o.locatorOnly&&(includeSecurity||o.locator<3||o.endpoint!=nil||o.native!=nil)))||(includeSecurity&&(len(o.security)==0||uint64(len(o.security))>uint64(^uint32(0)))){o.Failure=errEndpointObservation;o.Returned=true;return o.Failure}
 if o.locatorOnly{o.Descriptor=uintptr(o.locator)}else if o.native!=nil{
  if o.native.Original.owner!=r||!o.native.prepared{o.Failure=errEndpointObservation;o.Returned=true;return o.Failure}
  o.Descriptor=uintptr(o.native.descriptor)
 }else{o.Descriptor=o.endpoint.Fd()}
 o.CredentialLength=uint32(unsafe.Sizeof(o.Credentials))
 o.CredentialEntered=true
 o.CredentialReturn,o.CredentialSecondary,o.CredentialErrno=syscall.Syscall6(syscall.SYS_GETSOCKOPT,o.Descriptor,syscall.SOL_SOCKET,syscall.SO_PEERCRED,uintptr(unsafe.Pointer(&o.Credentials)),uintptr(unsafe.Pointer(&o.CredentialLength)),0)
 o.CredentialReturned=true
 o.creatorCredentials=o.Credentials
 runtime.KeepAlive(o.endpoint);runtime.KeepAlive(o)
 if o.CredentialErrno!=0{o.Failure=o.CredentialErrno}else if o.CredentialReturn!=0||o.CredentialLength!=uint32(unsafe.Sizeof(o.Credentials)){o.Failure=errEndpointObservation}
 if o.Failure!=nil{o.Returned=true;return o.Failure}
 if !includeSecurity{o.Returned=true;return nil}
 o.SecurityLength=uint32(len(o.security));o.SecurityEntered=true
 // Linux SOL_SOCKET/SO_PEERSEC is 31. The output may include or omit a NUL;
 // preserve the exact bytes and returned extent rather than trimming a string.
 o.SecurityReturn,o.SecuritySecondary,o.SecurityErrno=syscall.Syscall6(syscall.SYS_GETSOCKOPT,o.Descriptor,syscall.SOL_SOCKET,31,uintptr(unsafe.Pointer(&o.security[0])),uintptr(unsafe.Pointer(&o.SecurityLength)),0)
 o.SecurityReturned=true
 runtime.KeepAlive(o.security);runtime.KeepAlive(o.endpoint);runtime.KeepAlive(o)
 if o.SecurityErrno!=0{o.Failure=o.SecurityErrno}else if o.SecurityReturn!=0||o.SecurityLength==0||uint64(o.SecurityLength)>uint64(len(o.security)){o.Failure=errEndpointObservation}
 o.Returned=true;return o.Failure
 }()
 if interrupted{panic(o.Panic)}
 return failure
}
