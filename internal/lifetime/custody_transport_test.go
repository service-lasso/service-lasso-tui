package lifetime

import("bytes";"encoding/binary";"errors";"io";"testing")

type transferEndpoint struct{written bytes.Buffer;reply *bytes.Reader;failure error;writes int;before func()}
func(e *transferEndpoint)Write(p []byte)(int,error){if e.before!=nil{e.before()};e.writes++;if e.failure!=nil{return 1,e.failure};return e.written.Write(p)}
func(e *transferEndpoint)Read(p []byte)(int,error){if e.before!=nil{e.before()};return e.reply.Read(p)}

func TestLocalTransferRetainsActualEndpointAndPartialFailure(t *testing.T){
 cause:=errors.New("private transport failure");receiver:=NewReceiver();body:=[]byte("private body")
 endpoint:=&transferEndpoint{failure:cause};transfer:=&LocalTransfer{Endpoint:endpoint,Body:body}
 endpoint.before=func(){if transfer.Original.owner!=receiver||transfer.Endpoint!=endpoint||&transfer.Body[0]!=&body[0]{t.Fatal("source was not preowned")}}
 if err:=receiver.PrepareLocalRepresentation(transfer);err!=nil{t.Fatal(err)};receiver.Stop();receiver.Join();err:=receiver.ExchangeLocalRepresentation(transfer)
 if err!=cause||transfer.HeaderWrite.Attempts[0].Failure!=cause||transfer.HeaderWrite.Attempts[0].Count!=1||endpoint.writes!=1||!transfer.Returned{t.Fatal("partial original was lost or replayed")}
 if len(transfer.BodyWrite.Attempts)!=0||len(transfer.ReplyRead.Attempts)!=0{t.Fatal("uncertain transfer continued")}
}
func TestLocalTransferExactLengthObservationIsNotDisposition(t *testing.T){
 receiver:=NewReceiver();body:=[]byte("private body");var reply [8]byte;binary.LittleEndian.PutUint64(reply[:],uint64(len(body)))
 endpoint:=&transferEndpoint{reply:bytes.NewReader(reply[:])};transfer:=&LocalTransfer{Endpoint:endpoint,Body:body}
 if err:=receiver.PrepareLocalRepresentation(transfer);err!=nil{t.Fatal(err)};receiver.Stop();receiver.Join();if err:=receiver.ExchangeLocalRepresentation(transfer);err!=nil{t.Fatal(err)}
 if !bytes.Equal(endpoint.written.Bytes()[8:],body)||transfer.ReplyRead.Count!=8{t.Fatal("incomplete exact transfer")}
 receiver.Join();found:=false;if err:=receiver.Inspect(func(original *Original){if original.Value==transfer{found=true}});err!=nil||!found{t.Fatal("wire observation discarded actual original")}
}
type invalidTransferEndpoint struct{cause error}
func(e *invalidTransferEndpoint)Write(p []byte)(int,error){return len(p)+1,e.cause}
func(e *invalidTransferEndpoint)Read([]byte)(int,error){return 0,io.EOF}
func TestLocalTransferInvalidExtentRetainsRawCountAndCause(t *testing.T){
 cause:=errors.New("raw extent cause");transfer:=&LocalTransfer{Endpoint:&invalidTransferEndpoint{cause:cause}}
 receiver:=NewReceiver();if err:=receiver.PrepareLocalRepresentation(transfer);err!=nil{t.Fatal(err)};receiver.Join();err:=receiver.ExchangeLocalRepresentation(transfer)
 if !errors.Is(err,cause)||transfer.HeaderWrite.Attempts[0].Failure!=cause||transfer.HeaderWrite.Attempts[0].Count!=9{t.Fatal("invalid raw extent was projected away")}
}
func TestLocalTransferRejectsLateNewAdmissionAndRepeat(t *testing.T){
 receiver:=NewReceiver();var reply [8]byte
 endpoint:=&transferEndpoint{reply:bytes.NewReader(reply[:])};prepared:=&LocalTransfer{Endpoint:endpoint}
 if err:=receiver.PrepareLocalRepresentation(prepared);err!=nil{t.Fatal(err)}
 if err:=receiver.ExchangeLocalRepresentation(prepared);err==nil||prepared.Attempted{t.Fatal("exchange preceded actual producer join")}
 receiver.Stop();receiver.Join()
 late:=&LocalTransfer{Endpoint:endpoint};if err:=receiver.PrepareLocalRepresentation(late);err==nil{t.Fatal("late transfer was admitted")}
 if err:=receiver.ExchangeLocalRepresentation(late);err==nil||late.Attempted{t.Fatal("unowned late transfer entered IO")}
 if err:=receiver.ExchangeLocalRepresentation(prepared);err!=nil{t.Fatal(err)}
 writes:=endpoint.writes;if err:=receiver.ExchangeLocalRepresentation(prepared);err==nil||endpoint.writes!=writes{t.Fatal("once exchange was replayed")}
}
func TestLocalTransferUsesSamePreparedBackingAndEndpoint(t *testing.T){
 receiver:=NewReceiver();body:=[]byte("actual prepared representation");var reply [8]byte;binary.LittleEndian.PutUint64(reply[:],uint64(len(body)))
 endpoint:=&transferEndpoint{reply:bytes.NewReader(reply[:])};transfer:=&LocalTransfer{Endpoint:endpoint,Body:body}
 if err:=receiver.PrepareLocalRepresentation(transfer);err!=nil{t.Fatal(err)}
 transfer.Endpoint=&invalidTransferEndpoint{};transfer.Body=[]byte("replacement")
 receiver.Join();if err:=receiver.ExchangeLocalRepresentation(transfer);err!=nil{t.Fatal(err)}
 if !bytes.Equal(endpoint.written.Bytes()[8:],body)||&transfer.BodyWrite.Buffer[0]!=&body[0]{t.Fatal("prepared source replaced")}
}
