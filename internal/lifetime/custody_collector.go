package lifetime

import("bytes";"encoding/binary";"errors";"io";"os";"runtime/debug";"sync")

// LocalCollection is retained by the actual independent owner, not sent to the
// child. Its native endpoint/birth and fresh private storage acquisition must
// precede execution through the selected native owner (still a separate source
// obligation). This IO operation cannot prove those acquisition predicates.
type LocalCollection struct {
 Endpoint io.ReadWriter
 Birth any
 Storage *os.File
 Body,Readback []byte
 Header,Reply [8]byte
 HeaderRead,BodyRead,ReplyWrite TransferIO
 Write,Read TransferAttempt
 SyncFailure error
 Failure error
 Panic any
 Stack []byte
 Returned bool
 PreparationInterrupted,Interrupted bool
 mutex sync.Mutex
 attempted bool
 preparationAttempted,prepared bool
 preparedEndpoint io.ReadWriter
 preparedStorage *os.File
 preparedBirth any
 preparedBody,preparedReadback []byte
}

// PrepareLocalCollection reserves all raw progress cells before the endpoint
// starts carrying producer data. Body and Readback are distinct actual backing
// arrays, each held by this owner; their geometry is not a whole capacity grant.
func PrepareLocalCollection(collection *LocalCollection,size int)error {
 if collection==nil||size<0{return errOriginalEnrollment}
 collection.mutex.Lock();if collection.preparationAttempted{collection.mutex.Unlock();return errOriginalEnrollment};collection.preparationAttempted=true;collection.mutex.Unlock()
 interrupted:=false
 func(){
 returned:=false;defer func(){if !returned{interrupted=true;collection.PreparationInterrupted=true;collection.Panic=recover();collection.Stack=debug.Stack()}}()
 collection.Body=make([]byte,size);collection.Readback=make([]byte,size)
 collection.HeaderRead=TransferIO{Buffer:collection.Header[:],Attempts:make([]TransferAttempt,0,len(collection.Header))}
 collection.BodyRead=TransferIO{Buffer:collection.Body,Attempts:make([]TransferAttempt,0,size)}
 collection.ReplyWrite=TransferIO{Buffer:collection.Reply[:],Attempts:make([]TransferAttempt,0,len(collection.Reply))}
 collection.Write.Buffer=collection.Body;collection.Read.Buffer=collection.Readback
 collection.preparedEndpoint=collection.Endpoint;collection.preparedStorage=collection.Storage;collection.preparedBirth=collection.Birth;collection.preparedBody=collection.Body;collection.preparedReadback=collection.Readback
 collection.mutex.Lock();collection.prepared=true;collection.mutex.Unlock();returned=true
 }()
 if interrupted{panic(collection.Panic)}
 return nil
}

// Collect performs actual complete receive, private storage write/flush and
// separately held full readback before replying. It never closes/retries the
// actual endpoint or storage, erases originals or emits FINAL archive acceptance.
// Native birth binding/authentication, exact source/attempt body interpretation,
// namespace/exclusion and source-specific disposition remain required outside
// this operation. The reply alone grants none of them.
func(collection *LocalCollection)Collect()error {
 if collection==nil{return errOriginalEnrollment}
 collection.mutex.Lock();if !collection.prepared||collection.attempted{collection.mutex.Unlock();return errOriginalEnrollment};collection.attempted=true;collection.mutex.Unlock()
 var err error
 interrupted:=false
 func(){
 returned:=false;defer func(){if !returned{interrupted=true;collection.Interrupted=true;collection.Panic=recover();collection.Stack=debug.Stack()}}()
 if collection.preparedEndpoint==nil||collection.preparedStorage==nil{err=errOriginalEnrollment}else{err=readTransfer(collection.preparedEndpoint,&collection.HeaderRead)}
 if err==nil&&binary.LittleEndian.Uint64(collection.Header[:])!=uint64(len(collection.preparedBody)){err=errTransferExtent}
 if err==nil{err=readTransfer(collection.preparedEndpoint,&collection.BodyRead)}
 if err==nil{
  collection.Write.Count,collection.Write.Failure=collection.preparedStorage.WriteAt(collection.Write.Buffer,0)
  err=collection.Write.Failure
  if collection.Write.Count!=len(collection.Write.Buffer){err=errors.Join(errTransferExtent,err)}
 }
 if err==nil{collection.SyncFailure=collection.preparedStorage.Sync();err=collection.SyncFailure}
 if err==nil{
  collection.Read.Count,collection.Read.Failure=collection.preparedStorage.ReadAt(collection.Read.Buffer,0)
  err=collection.Read.Failure
  if collection.Read.Count!=len(collection.Read.Buffer){err=errors.Join(errTransferExtent,err)}
 }
 if err==nil&&!bytes.Equal(collection.preparedBody,collection.preparedReadback){err=errTransferReply}
 if err==nil{binary.LittleEndian.PutUint64(collection.Reply[:],uint64(len(collection.preparedBody)));err=writeTransfer(collection.preparedEndpoint,&collection.ReplyWrite)}
 collection.Failure=err;collection.Returned=true;returned=true
 }()
 if interrupted{panic(collection.Panic)}
 return err
}
