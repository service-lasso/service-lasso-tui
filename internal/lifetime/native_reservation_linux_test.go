//go:build linux
package lifetime
import "testing"
// UNRUN. Fixture quantities fund only these test objects, not product defaults.
func TestNativeReservationRetainsCandidateBacking(t *testing.T){
 q:=NativeStartupQuantities{1,1,1};count,err:=nativeStartupGeometry(q);if err!=nil{t.Fatal(err)}
 reservation:=&nativeStartupReservation{candidateQuantities:&q,sourceEndpointCalls:make([]NativeEndpointIO,count),sourceSecurity:make([]byte,1),sourceBody:make([]byte,1),sourceTerminalCalls:make([]TerminalCall,1),sourceHeaderAttempts:make([]TransferAttempt,8),sourceBodyAttempts:make([]TransferAttempt,1),sourceReplyAttempts:make([]TransferAttempt,8),sourceReadback:make([]byte,1),sourceTerminalWriteAttempts:make([]TransferAttempt,8),sourceTerminalReadAttempts:make([]TransferAttempt,112)}
 receiver:=NewReceiver();if err:=receiver.prepareNativeStartupReservation(reservation);err!=nil{t.Fatal(err)}
 original:=&reservation.body[0];reservation.sourceBody=[]byte{9};q.RepresentationBytes=2
 if &reservation.body[0]!=original||reservation.actualQuantities.RepresentationBytes!=1||reservation.actualCandidate!=&q{t.Fatal("prepared source originals replaced")}
 receiver.Stop();receiver.Join();if err:=receiver.prepareNativeStartupReservation(reservation);err==nil{t.Fatal("joined preparation admitted")}
}
func TestNativeReservationRejectsUnfundedRepresentableQuantity(t *testing.T){
 q:=NativeStartupQuantities{1,1,1};if _,err:=nativeStartupGeometry(q);err!=nil{t.Fatal(err)};reservation:=&nativeStartupReservation{candidateQuantities:&q};receiver:=NewReceiver()
 if err:=receiver.prepareNativeStartupReservation(reservation);err==nil||!reservation.Returned||reservation.Failure==nil||reservation.actualCandidate!=&q{t.Fatal("invalid geometry not retained")}
 receiver.Stop();receiver.Join()
}
func nativeReservationFixture(t *testing.T)*nativeStartupReservation{
 t.Helper();q:=&NativeStartupQuantities{1,1,1};count,err:=nativeStartupGeometry(*q);if err!=nil{t.Fatal(err)}
 return &nativeStartupReservation{candidateQuantities:q,sourceEndpointCalls:make([]NativeEndpointIO,count),sourceSecurity:make([]byte,1),sourceBody:make([]byte,1),sourceTerminalCalls:make([]TerminalCall,1),sourceHeaderAttempts:make([]TransferAttempt,8),sourceBodyAttempts:make([]TransferAttempt,1),sourceReplyAttempts:make([]TransferAttempt,8),sourceReadback:make([]byte,1),sourceTerminalWriteAttempts:make([]TransferAttempt,8),sourceTerminalReadAttempts:make([]TransferAttempt,112)}
}
func TestNativeReservationRejectsSpareCapacityAndAdjacentBacking(t *testing.T){
 for _,adjacent:=range []bool{false,true}{
  reservation:=nativeReservationFixture(t);shared:=make([]byte,2);reservation.sourceSecurity=shared[:1]
  if adjacent{reservation.sourceBody=shared[1:2]}
  receiver:=NewReceiver();if err:=receiver.prepareNativeStartupReservation(reservation);err==nil||!reservation.Returned||reservation.observedCapacities[1]!=2{t.Fatal("spare allocation extent admitted")}
  if cap(reservation.security)!=2||&reservation.security[0]!=&shared[0]{t.Fatal("failed original backing/capacity lost")};if adjacent && &reservation.body[0]!=&shared[1]{t.Fatal("adjacent original lost")}
  receiver.Stop();receiver.Join()
 }
}