package lifetime

import (
 "encoding/binary"
 "errors"
 "io"
 "runtime/debug"
 "sync"
)

var errTerminalControlStopped=errors.New("native terminal control admission stopped")
var errTerminalControlCapacity=errors.New("native terminal control backing exhausted")
var errTerminalControlAbnormal=errors.New("native terminal exchange did not return normally")
var errTerminalControlResponse=errors.New("native terminal control response invalid")

// TerminalControl is the SAME local endpoint's serialized terminal producer.
// It carries observations only. Preparation, operation tags and byte equality
// do not authenticate native birth/source or authorize original disposition.
type TerminalControl struct {
 Original Original
 Transfer *LocalTransfer
 Calls []TerminalCall
 receiver *Receiver
 transfer *LocalTransfer
 endpoint io.ReadWriter
 birth any
 calls []TerminalCall
 preparedAttempts *terminalAttemptBacking
 mutex sync.Mutex
 count int
 stopped bool
 Failure error
}
type TerminalCall struct {
 Original Original
 Borrow Borrow
 Operation uint64
 Header [8]byte
 Response [112]byte
 Write,Read TransferIO
 Observation TerminalObservation
 NativeFailure TerminalObservationError
 Failure error
 Panic any
 Stack []byte
 Attempted,Returned,Interrupted bool
}
type TerminalObservation struct {
 Version,Operation,Entered uint64
 InputQuery,OutputQuery,FallbackOpen,FallbackQuery,WindowQuery,WindowCopy,Result int64
 RawQuery,RawChange,Restore int64
 Rows,Columns,XPixels,YPixels uint16
}
// This is a Go observation error, not the outer native original or an errno
// reconstructed and presented as if that actual native object crossed IPC.
type TerminalObservationError struct { Call *TerminalCall }
func(*TerminalObservationError)Error()string{return "native terminal operation reported failure"}

type terminalAttemptBacking struct { write,read []TransferAttempt }
func(r *Receiver)PrepareTerminalControl(control *TerminalControl)error {
 return r.prepareTerminalControl(control,nil)
}
func(r *Receiver)prepareTerminalControl(control *TerminalControl,backing *terminalAttemptBacking)error {
 if r==nil||control==nil||control.Transfer==nil{return errOriginalEnrollment}
 if err:=r.Enroll(&control.Original,"terminal-control.endpoint",control);err!=nil{return err}
 control.preparedAttempts=backing
 if backing!=nil {
  maximum:=int(^uint(0)>>1)
  if len(control.Calls)>maximum/112||len(backing.write)!=len(control.Calls)*8||cap(backing.write)!=len(backing.write)||len(backing.read)!=len(control.Calls)*112||cap(backing.read)!=len(backing.read){return errOriginalEnrollment}
 }
 transfer:=control.Transfer
 if transfer.Original.owner!=r||transfer.Borrow.Original.owner!=r||transfer.Borrow.Root!=&transfer.Original||transfer.Borrow.Consumer!=transfer||transfer.Borrow.Returned||transfer.preparedEndpoint==nil||len(control.Calls)==0{return errOriginalEnrollment}
 r.mutex.Lock()
 if r.stopped||r.joined||transfer.terminalControl!=nil{r.mutex.Unlock();return errOriginalEnrollment}
 transfer.terminalControl=control;r.mutex.Unlock()
 control.receiver=r;control.transfer=transfer;control.endpoint=transfer.preparedEndpoint;control.birth=transfer.preparedBirth;control.calls=control.Calls
 for index:=range control.calls {
  call:=&control.calls[index]
  call.NativeFailure.Call=call
  if err:=r.Enroll(&call.Original,"terminal-control.preowned-call",call);err!=nil{return err}
  // Geometries cover actual worst one-byte IO progress for these fixed frames;
  // they are not a whole-session history limit or a native fit grant.
  if backing==nil {
   call.Write.Attempts=make([]TransferAttempt,0,len(call.Header))
   call.Read.Attempts=make([]TransferAttempt,0,len(call.Response))
  } else {
   if call.Write.Attempts!=nil||call.Read.Attempts!=nil{return errOriginalEnrollment}
   writeStart:=index*8;readStart:=index*112
   call.Write.Attempts=backing.write[writeStart:writeStart:writeStart+8]
   call.Read.Attempts=backing.read[readStart:readStart:readStart+112]
  }
 }
 return nil
}
func(control *TerminalControl)call(operation uint64)(call *TerminalCall,failure error) {
 if control==nil||control.receiver==nil||control.endpoint==nil{return nil,errOriginalEnrollment}
 control.mutex.Lock();defer control.mutex.Unlock()
 control.receiver.mutex.Lock()
 if control.receiver.stopped||control.receiver.joined{control.receiver.mutex.Unlock();return nil,errDispatchStopped}
 if control.Original.owner!=control.receiver{control.receiver.mutex.Unlock();return nil,errOriginalEnrollment}
 if control.transfer.streamFailure!=nil{failure:=control.transfer.streamFailure;control.receiver.mutex.Unlock();return nil,failure}
 // Register this actual synchronous producer under the SAME Stop/Add lock.
 // Join waits its real return even when the caller is not a Dispatch worker.
 control.receiver.workers.Add(1);control.receiver.mutex.Unlock()
 defer control.receiver.workers.Done()
 if control.stopped{return nil,errTerminalControlStopped}
 if control.count>=len(control.calls){control.stopped=true;control.Failure=errTerminalControlCapacity;return nil,control.Failure}
 call=&control.calls[control.count];control.count++
 call.Operation=operation;call.Attempted=true
 if err:=control.receiver.Borrow(&call.Borrow,&call.Original,"terminal-control.exchange",control);err!=nil{call.Failure=err;call.Returned=true;if call.Borrow.Original.owner==control.receiver{control.receiver.ReturnBorrow(&call.Borrow)};return call,err}
 interrupted:=false
 func(){
 returned:=false
 defer func(){
  if !returned{interrupted=true;call.Interrupted=true;call.Panic=recover();call.Stack=debug.Stack();call.Failure=errTerminalControlAbnormal;control.poison(call,errTerminalControlAbnormal)}
 }()
 binary.LittleEndian.PutUint64(call.Header[:],uint64(1)<<63|operation)
 call.Write.Buffer=call.Header[:];call.Read.Buffer=call.Response[:]
 failure=writeTransfer(control.endpoint,&call.Write)
 if failure==nil{failure=readTransfer(control.endpoint,&call.Read)}
 if failure==nil {
  bytes:=call.Response[:];observation:=&call.Observation
  observation.Version=binary.LittleEndian.Uint64(bytes)
  observation.Operation=binary.LittleEndian.Uint64(bytes[8:])
  observation.Entered=binary.LittleEndian.Uint64(bytes[16:])
  observation.InputQuery=int64(binary.LittleEndian.Uint64(bytes[24:]))
  observation.OutputQuery=int64(binary.LittleEndian.Uint64(bytes[32:]))
  observation.FallbackOpen=int64(binary.LittleEndian.Uint64(bytes[40:]))
  observation.FallbackQuery=int64(binary.LittleEndian.Uint64(bytes[48:]))
  observation.WindowQuery=int64(binary.LittleEndian.Uint64(bytes[56:]))
  observation.WindowCopy=int64(binary.LittleEndian.Uint64(bytes[64:]))
  observation.Result=int64(binary.LittleEndian.Uint64(bytes[72:]))
  observation.Rows=binary.LittleEndian.Uint16(bytes[80:]);observation.Columns=binary.LittleEndian.Uint16(bytes[82:])
  observation.XPixels=binary.LittleEndian.Uint16(bytes[84:]);observation.YPixels=binary.LittleEndian.Uint16(bytes[86:])
  observation.RawQuery=int64(binary.LittleEndian.Uint64(bytes[88:]))
  observation.RawChange=int64(binary.LittleEndian.Uint64(bytes[96:]))
  observation.Restore=int64(binary.LittleEndian.Uint64(bytes[104:]))
  if observation.Version!=2||observation.Operation!=uint64(1)<<63|operation||observation.Entered&^uint64(511)!=0||observation.Entered&3!=3 {
   failure=errTerminalControlResponse
  } else if observation.Result>0 {failure=errTerminalControlResponse
  } else if observation.Result<0 {failure=&call.NativeFailure
  } else if operation==3&&(observation.OutputQuery!=0||observation.Entered&48!=48||observation.WindowQuery!=0||observation.WindowCopy!=0) {failure=errTerminalControlResponse
  } else if operation==2&&(observation.Entered&192!=192||observation.RawQuery!=0||observation.RawChange!=0){failure=errTerminalControlResponse
  } else if operation==4&&(observation.Entered&256!=256||observation.Restore!=0){failure=errTerminalControlResponse}
 }
 call.Failure=failure;call.Returned=true;returned=true
 control.receiver.ReturnBorrow(&call.Borrow)
 // Uncertain IO/framing never admits another operation on an ambiguous stream.
 // A complete negative native observation remains separately owned and is not
 // automatically a closed expected domain refusal or native recovery.
 if call.Write.Failure!=nil||call.Read.Failure!=nil||failure==errTerminalControlResponse{control.poison(call,failure)}
 }()
 if interrupted{panic(call.Panic)}
 return call,failure
}
func(control *TerminalControl)Fallback()(*TerminalCall,error){return control.call(1)}
func(control *TerminalControl)MakeRaw()(*TerminalCall,error){return control.call(2)}
func(control *TerminalControl)Restore()(*TerminalCall,error){return control.call(4)}
func(control *TerminalControl)Size()(*TerminalCall,error){return control.call(3)}
// Caller invokes this only after stopping/joining actual terminal producers.
// Taking the serial mutex observes completion of their active exchange; it
// does not cancel native IO, join unrelated workers or grant D2 disposition.
func(control *TerminalControl)StopAdmission(){if control==nil{return};control.mutex.Lock();control.stopped=true;control.mutex.Unlock()}

// Poison belongs to the SAME prepared endpoint across terminal and shutdown IO.
// The call graph stays retained; this is no recovery or original disposition.
func(control *TerminalControl)poison(call *TerminalCall,failure error){
 control.stopped=true;control.Failure=failure
 control.receiver.mutex.Lock()
 if control.transfer.streamFailure==nil{control.transfer.streamFailure=failure;control.transfer.streamFailureOriginal=call}
 control.receiver.mutex.Unlock()
}
func(control *TerminalControl)BeforeFallback()error{_,err:=control.Fallback();return err}
func(control *TerminalControl)MakePublicRaw()error{_,err:=control.MakeRaw();return err}
func(control *TerminalControl)RestorePublicInput()error{_,err:=control.Restore();return err}
func(control *TerminalControl)PublicSize()(int,int,error){call,err:=control.Size();if err!=nil{return 0,0,err};return int(call.Observation.Columns),int(call.Observation.Rows),nil}
