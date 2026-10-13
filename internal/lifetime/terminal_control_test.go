package lifetime

import (
 "bytes"
 "encoding/binary"
 "errors"
 "io"
 "testing"
)

// These are source-level wire/ownership cases only, never native admission,
// actual terminal behavior, native original transfer or D2 eligibility proof.
type terminalWireFixture struct {input *bytes.Reader;written bytes.Buffer;writeFailure error;writes,reads int}
func(f *terminalWireFixture)Write(value []byte)(int,error){f.writes++;if f.writeFailure!=nil{return 1,f.writeFailure};return f.written.Write(value)}
func(f *terminalWireFixture)Read(value []byte)(int,error){f.reads++;return f.input.Read(value)}
func terminalWireResponse(operation uint64)[]byte{
 response:=make([]byte,112)
 binary.LittleEndian.PutUint64(response,2)
 binary.LittleEndian.PutUint64(response[8:],uint64(1)<<63|operation)
 binary.LittleEndian.PutUint64(response[16:],511)
 binary.LittleEndian.PutUint16(response[80:],24);binary.LittleEndian.PutUint16(response[82:],80)
 return response
}
func prepareTerminalWireFixture(t *testing.T,stream io.ReadWriter)(*Receiver,*TerminalControl){
 t.Helper();receiver:=NewReceiver();transfer:=&LocalTransfer{Endpoint:stream,Birth:new(int),Body:[]byte{1}}
 if err:=receiver.PrepareLocalRepresentation(transfer);err!=nil{t.Fatal(err)}
 control:=&TerminalControl{Transfer:transfer,Calls:make([]TerminalCall,2)}
 if err:=receiver.PrepareTerminalControl(control);err!=nil{t.Fatal(err)}
 return receiver,control
}
func TestTerminalControlPreservesPartialWriteOriginalAndStopsAmbiguousStream(t *testing.T){
 original:=errors.New("fixture original short write failure")
 stream:=&terminalWireFixture{input:bytes.NewReader(nil),writeFailure:original}
 _,control:=prepareTerminalWireFixture(t,stream)
 call,err:=control.Fallback()
 if err!=original||call.Failure!=original||len(call.Write.Attempts)!=1||call.Write.Attempts[0].Count!=1||call.Write.Attempts[0].Failure!=original{t.Fatal("actual partial count/error lost")}
 if !call.Returned||!call.Borrow.Returned||stream.reads!=0{t.Fatal("normal failed IO return/borrow not represented")}
 if _,err=control.Size();err!=original||stream.writes!=1{t.Fatal("ambiguous stream reused")}
}
func TestTerminalControlUsesSamePreparedEndpointAndCallBacking(t *testing.T){
 stream:=&terminalWireFixture{input:bytes.NewReader(terminalWireResponse(3))}
 _,control:=prepareTerminalWireFixture(t,stream)
 actual:=control.Calls
 control.Calls=make([]TerminalCall,1)
 control.Transfer.Endpoint=&terminalWireFixture{input:bytes.NewReader(nil)}
 call,err:=control.Size()
 if err!=nil||call!=&actual[0]||call.Observation.Rows!=24||call.Observation.Columns!=80{t.Fatalf("prepared observation lost: %v",err)}
 if call.Read.Buffer[0]!=call.Response[0]||&call.Read.Buffer[0]!=&call.Response[0]||stream.writes!=1||stream.reads!=1{t.Fatal("same source response backing not retained")}
 if binary.LittleEndian.Uint64(stream.written.Bytes())!=uint64(1)<<63|3{t.Fatal("wrong complete request discriminant")}
}
func TestTerminalControlAfterActualStopJoinDoesNotEnterIO(t *testing.T){
 stream:=&terminalWireFixture{input:bytes.NewReader(nil)}
 receiver,control:=prepareTerminalWireFixture(t,stream)
 receiver.Stop();receiver.Join()
 call,err:=control.Fallback()
 if err!=errDispatchStopped||call!=nil||stream.writes!=0||stream.reads!=0{t.Fatal("late terminal producer entered IO")}
}

func TestTerminalAmbiguityPreventsJoinedCustodyIO(t *testing.T){
 original:=errors.New("same partial terminal write")
 stream:=&terminalWireFixture{input:bytes.NewReader(nil),writeFailure:original}
 receiver,control:=prepareTerminalWireFixture(t,stream)
 call,err:=control.Fallback()
 if err!=original{t.Fatal("terminal original lost")}
 receiver.Stop();receiver.Join()
 writes,reads:=stream.writes,stream.reads
 transfer:=control.Transfer
 if err=receiver.ExchangeLocalRepresentation(transfer);err!=original{t.Fatal("custody did not retain SAME ambiguity")}
 if stream.writes!=writes||stream.reads!=reads||len(transfer.HeaderWrite.Attempts)!=0{t.Fatal("poisoned endpoint entered custody IO")}
 if transfer.streamFailureOriginal!=call||!transfer.Returned||!transfer.Borrow.Returned||call.Write.Attempts[0].Failure!=original{t.Fatal("original call/borrow relation lost")}
}
func TestInvalidTerminalResponsePreventsJoinedCustodyIO(t *testing.T){
 response:=terminalWireResponse(1);response[0]=3
 stream:=&terminalWireFixture{input:bytes.NewReader(response)}
 receiver,control:=prepareTerminalWireFixture(t,stream)
 call,err:=control.Fallback()
 if err!=errTerminalControlResponse{t.Fatal("invalid response accepted")}
 receiver.Stop();receiver.Join()
 writes,reads:=stream.writes,stream.reads
 if receiver.ExchangeLocalRepresentation(control.Transfer)!=errTerminalControlResponse||stream.writes!=writes||stream.reads!=reads||control.Transfer.streamFailureOriginal!=call{t.Fatal("invalid framing reused for custody")}
}
type terminalPanicFixture struct {terminalWireFixture;original any}
func(f *terminalPanicFixture)Write(value []byte)(int,error){f.writes++;panic(f.original)}
func TestInterruptedTerminalWritePreventsJoinedCustodyIO(t *testing.T){
 original:=new(int)
 stream:=&terminalPanicFixture{terminalWireFixture:terminalWireFixture{input:bytes.NewReader(nil)},original:original}
 receiver,control:=prepareTerminalWireFixture(t,stream)
 func(){defer func(){if recover()!=original{t.Fatal("SAME panic lost")}}();control.Fallback()}()
 receiver.Stop();receiver.Join()
 writes,reads:=stream.writes,stream.reads
 if receiver.ExchangeLocalRepresentation(control.Transfer)!=errTerminalControlAbnormal||stream.writes!=writes||stream.reads!=reads{t.Fatal("interrupted exchange reused endpoint")}
 call:=&control.calls[0]
 if call.Panic!=original||call.Returned||call.Borrow.Returned||control.Transfer.streamFailureOriginal!=call{t.Fatal("interrupted original falsely returned or lost")}
}
func TestSamePreparedTransferBindsOnlyOneTerminalProducer(t *testing.T){
 stream:=&terminalWireFixture{input:bytes.NewReader(nil)}
 receiver,first:=prepareTerminalWireFixture(t,stream)
 second:=&TerminalControl{Transfer:first.Transfer,Calls:make([]TerminalCall,1)}
 if err:=receiver.PrepareTerminalControl(second);err!=errOriginalEnrollment{t.Fatal("competing endpoint producer admitted")}
 if first.Transfer.terminalControl!=first||second.Original.owner!=receiver||second.receiver!=nil||stream.writes!=0||stream.reads!=0{t.Fatal("denied producer original lost or endpoint entered")}
}
func TestRawAndRestoreObserveSeparateOriginalResponses(t *testing.T){
 wire:=append(terminalWireResponse(2),terminalWireResponse(4)...)
 stream:=&terminalWireFixture{input:bytes.NewReader(wire)}
 _,control:=prepareTerminalWireFixture(t,stream)
 raw,err:=control.MakeRaw();if err!=nil{t.Fatal(err)}
 restored,err:=control.Restore();if err!=nil{t.Fatal(err)}
 if raw==restored||raw.Operation!=2||restored.Operation!=4||raw.Observation.RawQuery!=0||restored.Observation.Restore!=0||&raw.Response[0]==&restored.Response[0]{t.Fatal("raw/restore actual responses not independently retained")}
}