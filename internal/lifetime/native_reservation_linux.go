//go:build linux

package lifetime

import ("errors";"unsafe")

var errNativeReservation = errors.New("native candidate reservation geometry invalid")

// nativeStartupReservation owns source-prepared distinct backing. Its quantities
// are candidate construction inputs, never private argv. This is custody and
// geometry only; neither its presence nor successful preparation authenticates
// selected code, creator, policy, endpoint, or a native grant.
type nativeReservationRange struct{Start,Extent uintptr}
type nativeStartupReservation struct {
 original Original
 transferBacking transferAttemptBacking
 terminalBacking terminalAttemptBacking
 candidateQuantities *NativeStartupQuantities
 sourceEndpointCalls []NativeEndpointIO
 sourceSecurity []byte
 sourceBody []byte
 sourceTerminalCalls []TerminalCall
 sourceHeaderAttempts,sourceBodyAttempts,sourceReplyAttempts []TransferAttempt
 sourceReadback []byte
 sourceTerminalWriteAttempts,sourceTerminalReadAttempts []TransferAttempt
 observedCapacities [10]int
 ranges [12]nativeReservationRange
 actualCandidate *NativeStartupQuantities
 actualQuantities NativeStartupQuantities
 endpointCalls []NativeEndpointIO
 security,body,readback []byte
 terminalCalls []TerminalCall
 headerAttempts,bodyAttempts,replyAttempts []TransferAttempt
 terminalWriteAttempts,terminalReadAttempts []TransferAttempt
 Attempted,Returned bool
 Failure error
}

// prepareNativeStartupReservation performs no make, endpoint query or stream IO.
// The actual candidate constructor must establish fresh, disjoint backing and
// full aggregate charging before calling; qualification supplies real numbers.
// Immutable completed preparation is retained even on geometry refusal.
func (r *Receiver) prepareNativeStartupReservation(reservation *nativeStartupReservation) error {
 if r==nil || reservation==nil {return errOriginalEnrollment}
 r.mutex.Lock();if r.stopped||r.joined{r.mutex.Unlock();return errDispatchStopped};r.workers.Add(1);r.mutex.Unlock()
 defer r.workers.Done()
 if err:=r.Enroll(&reservation.original,"native-candidate.reservation",reservation);err!=nil{return err}
 if reservation.Attempted{return errOriginalEnrollment};reservation.Attempted=true
 reservation.actualCandidate=reservation.candidateQuantities
 reservation.endpointCalls=reservation.sourceEndpointCalls;reservation.security=reservation.sourceSecurity
 reservation.body=reservation.sourceBody;reservation.readback=reservation.sourceReadback
 reservation.terminalCalls=reservation.sourceTerminalCalls
 reservation.terminalWriteAttempts=reservation.sourceTerminalWriteAttempts;reservation.terminalReadAttempts=reservation.sourceTerminalReadAttempts
 reservation.headerAttempts=reservation.sourceHeaderAttempts;reservation.bodyAttempts=reservation.sourceBodyAttempts;reservation.replyAttempts=reservation.sourceReplyAttempts
 finish:=func(err error)error{reservation.Failure=err;reservation.Returned=true;return err}
 if reservation.actualCandidate==nil{return finish(errNativeReservation)}
 reservation.actualQuantities=*reservation.actualCandidate
 count,err:=nativeStartupGeometry(reservation.actualQuantities);if err!=nil{return finish(err)}
 q:=reservation.actualQuantities
 maximum:=int(^uint(0)>>1);if uint64(count)>uint64(maximum)/uint64(unsafe.Sizeof(TransferAttempt{})){return finish(errNativeReservation)}
 terminalWrites,terminalReads:=q.TerminalCalls*8,q.TerminalCalls*112
 if len(reservation.terminalWriteAttempts)!=terminalWrites||len(reservation.terminalReadAttempts)!=terminalReads{return finish(errNativeReservation)}
 if len(reservation.endpointCalls)!=count || len(reservation.security)!=q.SecurityLabelBytes || len(reservation.body)!=q.RepresentationBytes || len(reservation.readback)!=q.RepresentationBytes || len(reservation.terminalCalls)!=q.TerminalCalls || len(reservation.headerAttempts)!=8 || len(reservation.bodyAttempts)!=q.RepresentationBytes || len(reservation.replyAttempts)!=8 {return finish(errNativeReservation)}
 reservation.observedCapacities=[10]int{cap(reservation.endpointCalls),cap(reservation.security),cap(reservation.body),cap(reservation.terminalCalls),cap(reservation.headerAttempts),cap(reservation.bodyAttempts),cap(reservation.replyAttempts),cap(reservation.readback),cap(reservation.terminalWriteAttempts),cap(reservation.terminalReadAttempts)}
 expected:=[10]int{count,q.SecurityLabelBytes,q.RepresentationBytes,q.TerminalCalls,8,q.RepresentationBytes,8,q.RepresentationBytes,terminalWrites,terminalReads}
 for i,capacity:=range reservation.observedCapacities{if capacity!=expected[i]{return finish(errNativeReservation)}}
 // These views cannot grow into another region. Exact cap does not establish
 // fresh full-allocation ownership or eliminate external aliases: the actual
 // source candidate constructor must establish those before preparation.
 reservation.endpointCalls=reservation.endpointCalls[:count:count]
 reservation.security=reservation.security[:q.SecurityLabelBytes:q.SecurityLabelBytes]
 reservation.body=reservation.body[:q.RepresentationBytes:q.RepresentationBytes]
 reservation.terminalCalls=reservation.terminalCalls[:q.TerminalCalls:q.TerminalCalls]
 reservation.headerAttempts=reservation.headerAttempts[:8:8]
 reservation.bodyAttempts=reservation.bodyAttempts[:q.RepresentationBytes:q.RepresentationBytes]
 reservation.replyAttempts=reservation.replyAttempts[:8:8]
 reservation.readback=reservation.readback[:q.RepresentationBytes:q.RepresentationBytes]
 reservation.terminalWriteAttempts=reservation.terminalWriteAttempts[:terminalWrites:terminalWrites]
 reservation.terminalReadAttempts=reservation.terminalReadAttempts[:terminalReads:terminalReads]
 reservation.ranges=[12]nativeReservationRange{
  {uintptr(unsafe.Pointer(reservation)),unsafe.Sizeof(*reservation)},
  {uintptr(unsafe.Pointer(reservation.actualCandidate)),unsafe.Sizeof(*reservation.actualCandidate)},
  {uintptr(unsafe.Pointer(unsafe.SliceData(reservation.endpointCalls))),uintptr(count)*unsafe.Sizeof(NativeEndpointIO{})},
  {uintptr(unsafe.Pointer(unsafe.SliceData(reservation.security))),uintptr(len(reservation.security))},
  {uintptr(unsafe.Pointer(unsafe.SliceData(reservation.body))),uintptr(len(reservation.body))},
  {uintptr(unsafe.Pointer(unsafe.SliceData(reservation.terminalCalls))),uintptr(len(reservation.terminalCalls))*unsafe.Sizeof(TerminalCall{})},
  {uintptr(unsafe.Pointer(unsafe.SliceData(reservation.headerAttempts))),8*unsafe.Sizeof(TransferAttempt{})},
  {uintptr(unsafe.Pointer(unsafe.SliceData(reservation.bodyAttempts))),uintptr(len(reservation.bodyAttempts))*unsafe.Sizeof(TransferAttempt{})},
  {uintptr(unsafe.Pointer(unsafe.SliceData(reservation.replyAttempts))),8*unsafe.Sizeof(TransferAttempt{})},
  {uintptr(unsafe.Pointer(unsafe.SliceData(reservation.readback))),uintptr(len(reservation.readback))},
  {uintptr(unsafe.Pointer(unsafe.SliceData(reservation.terminalWriteAttempts))),uintptr(terminalWrites)*unsafe.Sizeof(TransferAttempt{})},
  {uintptr(unsafe.Pointer(unsafe.SliceData(reservation.terminalReadAttempts))),uintptr(terminalReads)*unsafe.Sizeof(TransferAttempt{})},
 }
 for i,a:=range reservation.ranges{
  if a.Start==0||a.Extent==0||a.Start>^uintptr(0)-a.Extent{return finish(errNativeReservation)}
  for j:=0;j<i;j++{b:=reservation.ranges[j];if a.Start<b.Start+b.Extent&&b.Start<a.Start+a.Extent{return finish(errNativeReservation)}}
 }
 return finish(nil)
}