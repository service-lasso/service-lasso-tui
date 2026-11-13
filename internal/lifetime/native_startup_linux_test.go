//go:build linux

package lifetime

import "testing"

// UNRUN geometry source cases. Values are fixture quantities, not product limits.
func TestNativeStartupGeometryUsesExactWireAndExtent(t *testing.T){
 q:=NativeStartupQuantities{SecurityLabelBytes:256,TerminalCalls:3,RepresentationBytes:64}
 count,err:=nativeStartupGeometry(q);if err!=nil||count!=3*120+64+16{t.Fatal("wire-derived geometry changed")}
 maximum:=int(^uint(0)>>1)
 bad:=[]NativeStartupQuantities{
 {SecurityLabelBytes:0,TerminalCalls:3,RepresentationBytes:64},
 {SecurityLabelBytes:256,TerminalCalls:0,RepresentationBytes:64},
 {SecurityLabelBytes:256,TerminalCalls:3,RepresentationBytes:0},
 {SecurityLabelBytes:256,TerminalCalls:maximum,RepresentationBytes:64},
 {SecurityLabelBytes:256,TerminalCalls:3,RepresentationBytes:maximum},
 }
 for _,q:=range bad{if _,err:=nativeStartupGeometry(q);err==nil{t.Fatal("invalid allocation/wire geometry accepted")}}
}
func TestNativeStartupInvalidFramingHasNoEndpointSyscall(t *testing.T){
 receiver:=NewReceiver();startup:=&NativePayloadStartup{RawArguments:[]string{"private","--help"}}
 if receiver.PrepareNativePayloadStartup(startup)==nil||startup.Endpoint.prepared||startup.Observation.CredentialEntered||startup.Observation.SecurityEntered{t.Fatal("unframed startup reached endpoint")}
 if startup.Arguments.Original.Value!=&startup.Arguments||startup.Original.Value!=startup||!startup.Returned{t.Fatal("actual startup failure roots not retained")}
 receiver.Stop();receiver.Join()
}
func TestNativeStartupGeometryComesFromActualV2Arguments(t *testing.T){
 maximum:=int(^uint(0)>>1)
 receiver:=NewReceiver();startup:=&NativePayloadStartup{RawArguments:[]string{"private",NativeArgumentVersion,"19","1","256","9223372036854775807","64","--"}}
 if uint64(maximum)<uint64(9223372036854775807) {return}
 if receiver.PrepareNativePayloadStartup(startup)==nil||startup.Endpoint.prepared||startup.Observation.CredentialEntered{t.Fatal("overflowing actual argv geometry reached endpoint")}
 if startup.Arguments.TerminalCalls!=maximum||startup.ObservedQuantities.TerminalCalls!=maximum{t.Fatal("actual quantity observation lost")}
 receiver.Stop();receiver.Join()
}
func TestNativeStartupForgedRepresentableQuantityCannotAllocateBacking(t *testing.T){
 receiver:=NewReceiver();startup:=&NativePayloadStartup{RawArguments:[]string{"private",NativeArgumentVersion,"19","1","1","1","1","--"}}
 if err:=receiver.PrepareNativePayloadStartup(startup);err!=errNativeReservation{t.Fatal("locator supplied candidate backing")}
 if startup.Arguments.Public!=nil||startup.Endpoint.Calls!=nil||startup.Observation.Security!=nil||startup.Transfer.Body!=nil||startup.Control.Calls!=nil||startup.Creator.CredentialEntered{t.Fatal("unbound candidate allocated or queried endpoint")}
 if startup.Arguments.PublicZero!="private"||!startup.Returned{t.Fatal("original argv/failure lost")}
}