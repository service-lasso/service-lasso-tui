//go:build linux
// +build linux

package cancelreader

import("errors";"fmt";"io";"os";"sync";"golang.org/x/sys/unix")

func NewReader(reader io.Reader)(CancelReader,error){return NewReaderOwned(reader,nil)}
func newReaderWithOwnership(reader io.Reader,ownership *Ownership)(CancelReader,error){
 file,ok:=reader.(File);if !ok{return newFallbackCancelReaderOwned(reader,ownership)}
 r:=&epollCancelReader{file:file,epoll:-1,ownership:ownership};ownership.Partial=r
 creating:=ownership.begin("epoll.create",r,0);epoll,err:=unix.EpollCreate1(0);creating.Output=epoll;creating.Failure=err;creating.Returned=true
 if err!=nil{return nil,fmt.Errorf("create epoll: %w",err)};r.epoll=epoll;r.epollAcquired=true
 pipes:=ownership.begin("cancel-pipe.create",r,nil);pipeReader,pipeWriter,pipeError:=os.Pipe();err=pipeError;if pipeReader!=nil{r.cancelSignalReader=pipeReader};if pipeWriter!=nil{r.cancelSignalWriter=pipeWriter};pipes.Output=[]*os.File{pipeReader,pipeWriter};pipes.Failure=err;pipes.Returned=true
 if err!=nil{return nil,errors.Join(err,r.Close())};r.pipeAcquired=true
 first:=&unix.EpollEvent{Events:unix.EPOLLIN,Fd:int32(file.Fd())};registration:=ownership.begin("epoll.add-input",r,first)
 err=unix.EpollCtl(epoll,unix.EPOLL_CTL_ADD,int(file.Fd()),first);registration.Failure=err;registration.Returned=true
 if err!=nil{return nil,errors.Join(fmt.Errorf("add reader to epoll interest list: %w",err),r.Close())}
 second:=&unix.EpollEvent{Events:unix.EPOLLIN,Fd:int32(r.cancelSignalReader.Fd())};registration=ownership.begin("epoll.add-cancel",r,second)
 err=unix.EpollCtl(epoll,unix.EPOLL_CTL_ADD,int(r.cancelSignalReader.Fd()),second);registration.Failure=err;registration.Returned=true
 if err!=nil{return nil,errors.Join(fmt.Errorf("add cancel reader to epoll interest list: %w",err),r.Close())}
 return r,nil
}
type epollCancelReader struct{
 file File
 cancelSignalReader,cancelSignalWriter File
 cancelMixin
 epoll int
 epollAcquired,pipeAcquired bool
 ownership *Ownership
 state sync.Mutex
 readSerial sync.Mutex
 // Private source-observation seams; installed readers leave these nil.
 readAdmitted,waitEntered func()
 readers sync.WaitGroup
 cancelLock sync.Mutex
 closing bool
 closeOnce,cancelOnce sync.Once
 closeAttempted bool
 closeError,cancelError error
}
func(r *epollCancelReader)Read(data []byte)(int,error){
 operation:=r.ownership.begin("input.read",r,data)
 finish:=func(count int,err error)(int,error){operation.Output=count;operation.Failure=err;operation.Returned=true;return count,err}
 r.state.Lock();if r.closing{r.state.Unlock();return finish(0,os.ErrClosed)};r.readers.Add(1);r.state.Unlock();defer r.readers.Done();if r.readAdmitted!=nil{r.readAdmitted()};r.readSerial.Lock();defer r.readSerial.Unlock();if r.isCanceled(){return finish(0,ErrCanceled)}
 err:=r.wait();if err!=nil{if errors.Is(err,ErrCanceled){signal:=make([]byte,1);read:=r.ownership.begin("cancel-pipe.read",r.cancelSignalReader,signal);count,readErr:=r.cancelSignalReader.Read(signal);read.Output=count;read.Failure=readErr;read.Returned=true;if readErr!=nil{return finish(0,fmt.Errorf("reading cancel signal: %w",readErr))}};return finish(0,err)}
 count,readErr:=r.file.Read(data);return finish(count,readErr)
}
func(r *epollCancelReader)Cancel()bool{
 r.cancelLock.Lock();defer r.cancelLock.Unlock();if r.closeAttempted||!r.pipeAcquired{return false};r.setCanceled()
 r.cancelOnce.Do(func(){signal:=[]byte{'c'};operation:=r.ownership.begin("cancel-pipe.write",r.cancelSignalWriter,signal);count,err:=r.cancelSignalWriter.Write(signal);operation.Output=count;operation.Failure=err;operation.Returned=true;r.cancelError=err;if err==nil&&count!=len(signal){r.cancelError=io.ErrShortWrite}})
 return r.cancelError==nil
}
func(r *epollCancelReader)Close()error{
 // Actual read/cancel borrowers must return before independently owned native
 // resources are closed. A stuck producer retains this runtime, not a timeout.
 r.closeOnce.Do(func(){r.state.Lock();r.closing=true;r.state.Unlock();if r.pipeAcquired{r.Cancel()};r.readers.Wait();r.cancelLock.Lock();defer r.cancelLock.Unlock();r.closeAttempted=true;r.closeError=errors.Join(r.closeError,r.cancelError)
  if r.epollAcquired{operation:=r.ownership.begin("epoll.close",r,r.epoll);operation.Failure=unix.Close(r.epoll);operation.Returned=true;r.closeError=errors.Join(r.closeError,operation.Failure)}
  if r.pipeAcquired{for _,file:=range []File{r.cancelSignalWriter,r.cancelSignalReader}{if file==nil{continue};operation:=r.ownership.begin("cancel-pipe.close",file,nil);operation.Failure=file.Close();operation.Returned=true;r.closeError=errors.Join(r.closeError,operation.Failure)}}
 });return r.closeError
}
func(r *epollCancelReader)wait()error{
 events:=make([]unix.EpollEvent,1)
 for{operation:=r.ownership.begin("epoll.wait",r,events);if r.waitEntered!=nil{r.waitEntered()};count,err:=unix.EpollWait(r.epoll,events,-1);operation.Output=count;operation.Failure=err;operation.Returned=true;if errors.Is(err,unix.EINTR){continue};if err!=nil{return fmt.Errorf("epoll wait: %w",err)};break}
 switch events[0].Fd{case int32(r.file.Fd()):return nil;case int32(r.cancelSignalReader.Fd()):return ErrCanceled};return fmt.Errorf("unknown epoll readiness")
}
