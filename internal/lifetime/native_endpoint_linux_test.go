//go:build linux

package lifetime

import (
 "os"
 "syscall"
 "testing"
 "unsafe"
)

// UNRUN source regression. This observes a genuine socketpair only when an
// exact supervised command is later admitted. It grants no native birth/profile.
func TestEndpointSocketRetainsActualCreatorAndRawSecurityFailure(t *testing.T){
 pair,err:=syscall.Socketpair(syscall.AF_UNIX,syscall.SOCK_STREAM|syscall.SOCK_CLOEXEC,0)
 if err!=nil{t.Fatal(err)}
 left:=os.NewFile(uintptr(pair[0]),"endpoint-original");right:=os.NewFile(uintptr(pair[1]),"endpoint-peer")
 defer left.Close();defer right.Close()
 receiver:=NewReceiver();security:=make([]byte,1)
 observation:=&EndpointSocketObservation{Endpoint:left,Security:security}
 err=receiver.ObserveEndpointSocket(observation)
 receiver.Stop();receiver.Join()
 if !observation.CredentialEntered||!observation.CredentialReturned||observation.CredentialErrno!=0||observation.CredentialLength!=uint32(unsafe.Sizeof(observation.Credentials)){t.Fatal("raw credentials not retained")}
 if observation.Credentials.Pid!=int32(os.Getpid())||observation.endpoint!=left||&observation.security[0]!=&security[0]{t.Fatal("creator or original endpoint/backing changed")}
 if !observation.SecurityEntered||!observation.SecurityReturned||!observation.Returned{t.Fatal("security query completion missing")}
 if observation.SecurityErrno!=0&&err!=observation.SecurityErrno{t.Fatal("raw security errno replaced")}
 if err==nil&&(observation.SecurityLength==0||observation.SecurityLength>1){t.Fatal("invalid security extent accepted")}
 originalReturn:=observation.SecurityReturn
 if receiver.ObserveEndpointSocket(observation)==nil||observation.SecurityReturn!=originalReturn{t.Fatal("repeat observation overwrote original")}
}

func TestEndpointSocketStoppedBeforeObservationHasNoSyscallEntry(t *testing.T){
 receiver:=NewReceiver();receiver.Stop()
 observation:=&EndpointSocketObservation{Security:make([]byte,1)}
 if receiver.ObserveEndpointSocket(observation)==nil{t.Fatal("stopped source admitted")}
 if observation.CredentialEntered||observation.SecurityEntered{t.Fatal("stopped source performed native query")}
 receiver.Join()
}