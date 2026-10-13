package lifetime

import ("bytes";"encoding/binary";"testing";"io")

// Source regressions only; UNRUN, no authentic native endpoint/admission claim.
func TestPreownedTransportConsumesSameNestedBacking(t *testing.T){
 receiver:=NewReceiver()
 response:=terminalWireResponse(1)
 var reply [8]byte;binary.LittleEndian.PutUint64(reply[:],1)
 stream:=&terminalWireFixture{input:bytes.NewReader(append(response,reply[:]...))}
 transfer:=&LocalTransfer{Endpoint:stream,Body:[]byte{9}}
 var header [8]TransferAttempt;var body [1]TransferAttempt;var acknowledgment [8]TransferAttempt
 backing:=transferAttemptBacking{header[:],body[:],acknowledgment[:]}
 if err:=receiver.prepareLocalRepresentation(transfer,&backing);err!=nil{t.Fatal(err)}
 var calls [1]TerminalCall;var writes [8]TransferAttempt;var reads [112]TransferAttempt
 control:=&TerminalControl{Transfer:transfer,Calls:calls[:]}
 nested:=terminalAttemptBacking{writes[:],reads[:]}
 if err:=receiver.prepareTerminalControl(control,&nested);err!=nil{t.Fatal(err)}
 call,err:=control.Fallback();if err!=nil{t.Fatal(err)}
 if &call.Write.Attempts[0]!=&writes[0]||&call.Read.Attempts[0]!=&reads[0]||cap(call.Write.Attempts)!=8||cap(call.Read.Attempts)!=112{t.Fatal("terminal backing replaced")}
 receiver.Stop();receiver.Join()
 if err:=receiver.ExchangeLocalRepresentation(transfer);err!=nil{t.Fatal(err)}
 if &transfer.HeaderWrite.Attempts[0]!=&header[0]||&transfer.BodyWrite.Attempts[0]!=&body[0]||&transfer.ReplyRead.Attempts[0]!=&acknowledgment[0]{t.Fatal("transfer backing replaced")}
}
func TestPreownedTerminalRejectsShortNestedBackingBeforeIO(t *testing.T){
 receiver:=NewReceiver();stream:=&terminalWireFixture{input:bytes.NewReader(nil)}
 transfer:=&LocalTransfer{Endpoint:stream,Body:[]byte{1}}
 if err:=receiver.PrepareLocalRepresentation(transfer);err!=nil{t.Fatal(err)}
 var calls [1]TerminalCall;var writes [7]TransferAttempt;var reads [112]TransferAttempt
 control:=&TerminalControl{Transfer:transfer,Calls:calls[:]}
 if err:=receiver.prepareTerminalControl(control,&terminalAttemptBacking{writes[:],reads[:]});err==nil{t.Fatal("unfunded frame accepted")}
 if stream.writes!=0||stream.reads!=0||transfer.terminalControl!=nil{t.Fatal("invalid geometry bound endpoint or performed IO")}
}
type preownedBinderFixture struct {terminalWireFixture;bindings int}
func(f *preownedBinderFixture)bindLocalTransfer(_ *Receiver,_ *LocalTransfer)(io.ReadWriter,error){f.bindings++;return f,nil}
func TestInvalidPreownedTransferCannotBindOrPrepareTerminal(t *testing.T){
 receiver:=NewReceiver();stream:=&preownedBinderFixture{}
 transfer:=&LocalTransfer{Endpoint:stream,Body:[]byte{1}}
 var header [7]TransferAttempt;var body [1]TransferAttempt;var reply [8]TransferAttempt
 if err:=receiver.prepareLocalRepresentation(transfer,&transferAttemptBacking{header[:],body[:],reply[:]});err==nil{t.Fatal("short header accepted")}
 var calls [1]TerminalCall
 control:=&TerminalControl{Transfer:transfer,Calls:calls[:]}
 if err:=receiver.PrepareTerminalControl(control);err==nil{t.Fatal("incomplete transfer admitted terminal")}
 if stream.bindings!=0||stream.writes!=0||stream.reads!=0||transfer.Borrow.Original.owner!=nil||transfer.terminalControl!=nil{t.Fatal("failed preparation admitted endpoint consumer")}
}