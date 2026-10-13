package lifetime

import (
 "encoding/binary"
 "errors"
 "io"
 "runtime/debug"
)

// LocalTransfer is the actual local representation transport occurrence. The
// endpoint must later be linked to its genuine before-birth native occurrence;
// this transport neither authenticates that link nor authorizes disposition.
// The caller retains Body unchanged for the entire transfer/collector borrow.
type LocalTransfer struct {
 Original Original
 Endpoint io.ReadWriter
 Birth any
 Body []byte
 Header [8]byte
 Reply [8]byte
 HeaderWrite,BodyWrite,ReplyRead TransferIO
 Panic any
 Stack []byte
 Returned,Interrupted bool
 Failure error
 Borrow Borrow
 Attempted bool
 exchanging bool
 preparedEndpoint io.ReadWriter
 preparedBirth any
 preparedBody []byte
 preparedAttempts *transferAttemptBacking
 // Guarded by the owning Receiver mutex; retains the actual ambiguous call.
 terminalControl *TerminalControl
 streamFailure error
 streamFailureOriginal *TerminalCall
}
type TransferIO struct {
 Buffer []byte
 Count int
 Failure error
 Attempts []TransferAttempt
}
type TransferAttempt struct { Buffer []byte;Count int;Failure error }
var errTransferExtent=errors.New("local representation transfer returned invalid extent")
var errTransferReply=errors.New("local representation transfer reply does not match")

// ExchangeLocalRepresentation sends one complete length-delimited private body
// and receives its exact length observation. This is a wire observation ONLY:
// the independent collector's durable exact readback and authentic birth-bound
// custody relation are separate required source operations. No retry follows an
// uncertain operation. The cells retain raw native/transport causes privately.
// Prepared attempt backing is supplied only by the source-owned native root.
// Compatibility preparation deliberately remains a separate allocation path.
type transferAttemptBacking struct { header,body,reply []TransferAttempt }
func(r *Receiver)PrepareLocalRepresentation(transfer *LocalTransfer)error {
 return r.prepareLocalRepresentation(transfer,nil)
}
func(r *Receiver)prepareLocalRepresentation(transfer *LocalTransfer,backing *transferAttemptBacking)error {
 if r==nil||transfer==nil||transfer.Endpoint==nil{return errOriginalEnrollment}
 if err:=r.Enroll(&transfer.Original,"local-representation.exchange",transfer);err!=nil{return err}
 transfer.preparedAttempts=backing
 transfer.preparedEndpoint=transfer.Endpoint;transfer.preparedBirth=transfer.Birth;transfer.preparedBody=transfer.Body

 if backing==nil {
  transfer.HeaderWrite.Attempts=make([]TransferAttempt,0,len(transfer.Header));transfer.BodyWrite.Attempts=make([]TransferAttempt,0,len(transfer.Body));transfer.ReplyRead.Attempts=make([]TransferAttempt,0,len(transfer.Reply))
 } else {
  if len(backing.header)!=len(transfer.Header)||cap(backing.header)!=len(backing.header)||len(backing.body)!=len(transfer.preparedBody)||cap(backing.body)!=len(backing.body)||len(backing.reply)!=len(transfer.Reply)||cap(backing.reply)!=len(backing.reply){return errOriginalEnrollment}
  if transfer.HeaderWrite.Attempts!=nil||transfer.BodyWrite.Attempts!=nil||transfer.ReplyRead.Attempts!=nil{return errOriginalEnrollment}
  transfer.HeaderWrite.Attempts=backing.header[:0:len(backing.header)]
  transfer.BodyWrite.Attempts=backing.body[:0:len(backing.body)]
  transfer.ReplyRead.Attempts=backing.reply[:0:len(backing.reply)]
 }
 if binder,ok:=transfer.Endpoint.(interface{bindLocalTransfer(*Receiver,*LocalTransfer)(io.ReadWriter,error)});ok{bound,err:=binder.bindLocalTransfer(r,transfer);if err!=nil{return err};transfer.preparedEndpoint=bound}
 return r.Borrow(&transfer.Borrow,&transfer.Original,"local-representation.exchange.io",transfer)
}
func(r *Receiver)ExchangeLocalRepresentation(transfer *LocalTransfer)error {
 if r==nil||transfer==nil{return errOriginalEnrollment}
 r.mutex.Lock()
 if !r.joined||r.inspections!=0||transfer.Original.owner!=r||transfer.Borrow.Original.owner!=r||transfer.Attempted{r.mutex.Unlock();return errOriginalEnrollment}
 transfer.Attempted=true;transfer.exchanging=true;r.terminalIO++;streamFailure:=transfer.streamFailure;r.mutex.Unlock()
 defer func(){r.mutex.Lock();transfer.exchanging=false;r.terminalIO--;r.mutex.Unlock()}()
 if streamFailure!=nil{transfer.Failure=streamFailure;transfer.Returned=true;r.ReturnBorrow(&transfer.Borrow);return streamFailure}
 interrupted:=false;var err error
 func(){
 returned:=false
 defer func(){if !returned{interrupted=true;transfer.Interrupted=true;transfer.Panic=recover();transfer.Stack=debug.Stack()}}()
 binary.LittleEndian.PutUint64(transfer.Header[:],uint64(len(transfer.preparedBody)))
 transfer.HeaderWrite.Buffer=transfer.Header[:]
 transfer.BodyWrite.Buffer=transfer.preparedBody
 transfer.ReplyRead.Buffer=transfer.Reply[:]
 err=writeTransfer(transfer.preparedEndpoint,&transfer.HeaderWrite)
 if err==nil{err=writeTransfer(transfer.preparedEndpoint,&transfer.BodyWrite)}
 if err==nil{err=readTransfer(transfer.preparedEndpoint,&transfer.ReplyRead)}
 if err==nil&&binary.LittleEndian.Uint64(transfer.Reply[:])!=uint64(len(transfer.preparedBody)){err=errTransferReply}
 transfer.Failure=err;transfer.Returned=true;returned=true
 r.ReturnBorrow(&transfer.Borrow)
 }()
 // Goexit continues unwinding the outer call. A recovered nil panic returns
 // here, where it must be propagated even though its original value is nil.
 if interrupted{panic(transfer.Panic)}
 return err
}
func writeTransfer(writer io.Writer,operation *TransferIO)error {
 for operation.Count<len(operation.Buffer){
  operation.Attempts=append(operation.Attempts,TransferAttempt{Buffer:operation.Buffer[operation.Count:]})
  attempt:=&operation.Attempts[len(operation.Attempts)-1]
  attempt.Count,attempt.Failure=writer.Write(attempt.Buffer)
  if attempt.Count<0||attempt.Count>len(attempt.Buffer){operation.Failure=errors.Join(errTransferExtent,attempt.Failure);return operation.Failure}
  operation.Count+=attempt.Count
  if attempt.Failure!=nil{operation.Failure=attempt.Failure;return attempt.Failure}
  if attempt.Count==0{operation.Failure=io.ErrNoProgress;return operation.Failure}
 }
 return nil
}
func readTransfer(reader io.Reader,operation *TransferIO)error {
 for operation.Count<len(operation.Buffer){
  operation.Attempts=append(operation.Attempts,TransferAttempt{Buffer:operation.Buffer[operation.Count:]})
  attempt:=&operation.Attempts[len(operation.Attempts)-1]
  attempt.Count,attempt.Failure=reader.Read(attempt.Buffer)
  if attempt.Count<0||attempt.Count>len(attempt.Buffer){operation.Failure=errors.Join(errTransferExtent,attempt.Failure);return operation.Failure}
  operation.Count+=attempt.Count
  // Even a final n>0+error is retained and fails this transport occurrence.
  if attempt.Failure!=nil{operation.Failure=attempt.Failure;return attempt.Failure}
  if attempt.Count==0{operation.Failure=io.ErrNoProgress;return operation.Failure}
 }
 return nil
}
