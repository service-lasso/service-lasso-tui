//go:build linux

package lifetime

import (
 "encoding/binary"
 "io"
 "os"
 "syscall"
 "testing"
)
// UNRUN genuine socket fixture: requires a future bounded supervised admission.
func TestNativeDescriptorPreownedExchangeAfterJoin(t *testing.T){
 pair,err:=syscall.Socketpair(syscall.AF_UNIX,syscall.SOCK_STREAM|syscall.SOCK_CLOEXEC,0);if err!=nil{t.Fatal(err)}
 peer:=os.NewFile(uintptr(pair[1]),"native-peer-fixture");defer peer.Close();defer syscall.Close(pair[0])
 receiver:=NewReceiver();calls:=make([]NativeEndpointIO,7);endpoint:=&NativeEndpoint{Descriptor:pair[0],Calls:calls}
 if err:=receiver.PrepareNativeEndpoint(endpoint);err!=nil{t.Fatal(err)}
 body:=[]byte("body");transfer:=&LocalTransfer{Endpoint:endpoint,Body:body}
 if err:=receiver.PrepareLocalRepresentation(transfer);err!=nil{t.Fatal(err)}
 completed:=make(chan error,1)
 go func(){var header [8]byte;var received [4]byte
  _,err:=io.ReadFull(peer,header[:]);if err==nil{_,err=io.ReadFull(peer,received[:])};if err==nil&&(binary.LittleEndian.Uint64(header[:])!=4||string(received[:])!="body"){err=errTransferExtent};if err==nil{_,err=peer.Write(header[:])};completed<-err
 }()
 receiver.Stop();receiver.Join()
 if err:=receiver.ExchangeLocalRepresentation(transfer);err!=nil{t.Fatal(err)}
 if err:=<-completed;err!=nil{t.Fatal(err)}
 if endpoint.count!=3||!calls[0].Returned||calls[0].Operation!=syscall.SYS_SENDTO||calls[0].Flags!=syscall.MSG_NOSIGNAL||calls[1].Buffer[0]!=body[0]||!calls[2].Returned{t.Fatal("actual native exchange cells missing")}
 if &calls[1].Buffer[0]!=&body[0]{t.Fatal("body alias replaced")}
 if _,err:=endpoint.Read(make([]byte,1));err==nil||calls[3].Entered{t.Fatal("generic postjoin reader admitted")}
 if _,err:=endpoint.Write([]byte("x"));err==nil||calls[4].Entered{t.Fatal("generic postjoin writer admitted")}
 if _,err:=transfer.preparedEndpoint.Read(make([]byte,1));err==nil||calls[5].Entered{t.Fatal("retired transfer admitted")}
 wrong:=&LocalTransfer{}
 if _,err:=endpoint.invoke(syscall.SYS_READ,make([]byte,1),wrong);err==nil||calls[6].Entered||calls[6].Transfer!=wrong{t.Fatal("wrong transfer admitted or lost")}
 if receiver.ExchangeLocalRepresentation(transfer)==nil{t.Fatal("same exchange repeated")}
}
func TestNativeDescriptorCompetingTransferAndExhaustion(t *testing.T){
 pair,err:=syscall.Socketpair(syscall.AF_UNIX,syscall.SOCK_STREAM|syscall.SOCK_CLOEXEC,0);if err!=nil{t.Fatal(err)}
 defer syscall.Close(pair[0]);defer syscall.Close(pair[1])
 receiver:=NewReceiver();calls:=make([]NativeEndpointIO,1);endpoint:=&NativeEndpoint{Descriptor:pair[0],Calls:calls}
 if err:=receiver.PrepareNativeEndpoint(endpoint);err!=nil{t.Fatal(err)}
 first:=&LocalTransfer{Endpoint:endpoint};second:=&LocalTransfer{Endpoint:endpoint}
 if receiver.PrepareLocalRepresentation(first)!=nil||receiver.PrepareLocalRepresentation(second)==nil||second.Original.Value!=second{t.Fatal("competing transfer not retained and denied")}
 buffer:=[]byte("x");if _,err:=endpoint.Write(buffer);err!=nil{t.Fatal(err)}
 if _,err:=endpoint.Write(buffer);err!=errNativeEndpointCapacity||endpoint.Exhaustion.Buffer==nil||endpoint.Exhaustion.Entered{t.Fatal("first exhaustion original missing")}
 if &endpoint.Exhaustion.Buffer[0]!=&buffer[0]||endpoint.count!=1{t.Fatal("exhaustion backing changed")}
 receiver.Stop();receiver.Join()
}
func TestNativeDescriptorPartialAndErrorKeepSameBuffer(t *testing.T){
 pair,err:=syscall.Socketpair(syscall.AF_UNIX,syscall.SOCK_STREAM|syscall.SOCK_CLOEXEC|syscall.SOCK_NONBLOCK,0);if err!=nil{t.Fatal(err)}
 defer syscall.Close(pair[0]);defer syscall.Close(pair[1])
 if err:=syscall.SetsockoptInt(pair[0],syscall.SOL_SOCKET,syscall.SO_SNDBUF,4096);err!=nil{t.Fatal(err)}
 receiver:=NewReceiver();calls:=make([]NativeEndpointIO,2);endpoint:=&NativeEndpoint{Descriptor:pair[0],Calls:calls}
 if err:=receiver.PrepareNativeEndpoint(endpoint);err!=nil{t.Fatal(err)}
 body:=make([]byte,1024*1024)
 count,err:=endpoint.Write(body)
 if err!=nil||count<=0||count>=len(body)||calls[0].RawReturn!=uintptr(count)||&calls[0].Buffer[0]!=&body[0]{t.Fatal("actual partial send not retained")}
 remaining:=body[count:];count,err=endpoint.Write(remaining)
 if count!=0||err!=syscall.EAGAIN||calls[1].Errno!=syscall.EAGAIN||!calls[1].Returned||calls[1].Flags!=syscall.MSG_NOSIGNAL||&calls[1].Buffer[0]!=&remaining[0]{t.Fatal("actual nonblocking failure not retained")}
 receiver.Stop();receiver.Join()
}