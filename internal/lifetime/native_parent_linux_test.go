//go:build linux

package lifetime

import("os";"syscall";"testing";"strconv")

// UNRUN: actual socket creator is this test process, not its native parent.
// No socket label buffer, endpoint IO roster or quantity allocation is needed.
func TestFixedCreatorPreflightRejectsSelfCreatedEndpointBeforePidfd(t *testing.T){
 pair,err:=syscall.Socketpair(syscall.AF_UNIX,syscall.SOCK_STREAM|syscall.SOCK_CLOEXEC,0);if err!=nil{t.Fatal(err)}
 defer syscall.Close(pair[0]);defer syscall.Close(pair[1])
 receiver:=NewReceiver();socket:=&EndpointSocketObservation{}
 if err:=receiver.ObserveEndpointCreatorDescriptor(socket,pair[0]);err!=nil{t.Fatal(err)}
 if !socket.CredentialReturned||socket.creatorCredentials.Pid!=int32(os.Getpid())||socket.SecurityEntered||socket.security!=nil{t.Fatal("fixed creator observation changed")}
 // Exported projection is not the retained creator used by the parent observer.
 socket.Credentials.Pid=int32(os.Getppid())
 parent:=&NativeParentObservation{Socket:socket}
 if receiver.ObserveNativeParent(parent)!=errNativeParentObservation||!parent.Before.Returned||parent.Open.Entered||parent.Poll.Entered{t.Fatal("self-created endpoint gained parent association")}
 if parent.socket!=socket||!parent.Returned||parent.RawDescriptor!=-1||parent.HeldDescriptor!=-1{t.Fatal("actual refusal roots lost")}
 before:=parent.Before
 if receiver.ObserveNativeParent(parent)==nil||parent.Before!=before{t.Fatal("repeat replaced original")}
 receiver.Stop();receiver.Join()
}
func TestFixedParentStoppedBeforeEntryHasNoNativeCall(t *testing.T){
 receiver:=NewReceiver();receiver.Stop();parent:=&NativeParentObservation{}
 if receiver.ObserveNativeParent(parent)==nil||parent.Before.Entered||parent.Open.Entered||parent.Poll.Entered{t.Fatal("stopped parent observer entered native call")}
 receiver.Join()
}
// UNRUN actual constructor path: a real self-created locator cannot supply the
// missing source-owned candidate. Refuse before observing its native creator;
// the independent fixed-observer test above still checks the real parent cut.
func TestStartupSelfCreatedLocatorRejectsBeforeQuantityBacking(t *testing.T){
 pair,err:=syscall.Socketpair(syscall.AF_UNIX,syscall.SOCK_STREAM|syscall.SOCK_CLOEXEC,0);if err!=nil{t.Fatal(err)}
 defer syscall.Close(pair[0]);defer syscall.Close(pair[1])
 receiver:=NewReceiver();startup:=&NativePayloadStartup{RawArguments:[]string{"public",NativeArgumentVersion,strconv.Itoa(pair[0]),"1","256","3","64","--"}}
 if receiver.PrepareNativePayloadStartup(startup)!=errNativeReservation{t.Fatal("self-created locator supplied absent candidate backing")}
 if startup.Creator.CredentialEntered||startup.Creator.CredentialReturned||startup.Parent.Before.Entered||startup.Parent.Open.Entered{t.Fatal("unbound candidate reached native observations")}
 if startup.Endpoint.Calls!=nil||startup.Observation.Security!=nil||startup.Transfer.Body!=nil||startup.Control.Calls!=nil{t.Fatal("unassociated locator allocated quantity backing")}
 if startup.Original.Value!=startup||startup.Arguments.Original.Value!=&startup.Arguments||!startup.Returned{t.Fatal("actual constructor refusal roots lost")}
 receiver.Stop();receiver.Join()
}
