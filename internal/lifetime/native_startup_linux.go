//go:build linux

package lifetime

import ("errors";"unsafe";"runtime/debug")

var errNativeStartupGeometry=errors.New("native payload startup geometry invalid")

type NativeStartupQuantities struct{SecurityLabelBytes,TerminalCalls,RepresentationBytes int}
// NativePayloadStartup is one actual local root. Its construction preserves
// observations and backing; creator/policy/native birth association remains a
// required independent source relation before any terminal/custody stream IO.
type NativePayloadStartup struct {
 Original Original
 candidateOriginal any
 receiverOriginal *Receiver
 candidateProfile []byte
 contextRelation,ownerContextRelation,socketContextRelation nativeCandidateContext
 lineage nativeCandidateLineage
 reservation *nativeStartupReservation
 RawArguments []string
 ObservedQuantities NativeStartupQuantities
 quantities NativeStartupQuantities
 Arguments NativeArguments
 Endpoint NativeEndpoint
 Creator EndpointSocketObservation
 Parent NativeParentObservation
 ParentProc NativeParentProc
 CreatorImage NativeCreatorImage
 ParentContext,PayloadContext NativeProcessContext
 ImageLabel NativeImageLabel
 Current NativeSELinuxCurrent
 Observation EndpointSocketObservation
 Transfer LocalTransfer
 Control TerminalControl
 RawIOCapacity int
 Attempted,Returned bool
 Failure error
 Panic any
 Stack []byte
 AbnormalTermination bool
}
func nativeStartupGeometry(q NativeStartupQuantities)(int,error){
 maximum:=int(^uint(0)>>1)
 if q.SecurityLabelBytes<=0||uint64(q.SecurityLabelBytes)>uint64(^uint32(0))||q.TerminalCalls<=0||q.RepresentationBytes<=0||q.RepresentationBytes>maximum-16{return 0,errNativeStartupGeometry}
 if uint64(q.TerminalCalls)>uint64(maximum)/uint64(unsafe.Sizeof(TerminalCall{}))||uint64(q.RepresentationBytes)>uint64(maximum)/uint64(unsafe.Sizeof(TransferAttempt{})){return 0,errNativeStartupGeometry}
 rest:=q.RepresentationBytes+16
 if q.TerminalCalls>(maximum-rest)/120{return 0,errNativeStartupGeometry}
 count:=q.TerminalCalls*120+rest
 if uint64(count)>uint64(maximum)/uint64(unsafe.Sizeof(NativeEndpointIO{})){return 0,errNativeStartupGeometry}
 return count,nil
}
func(r *Receiver)PrepareNativePayloadStartup(startup *NativePayloadStartup)error {
 interrupted:=false
 failure:=func()error{
 if r==nil||startup==nil{return errOriginalEnrollment}
 if err:=r.Enroll(&startup.Original,"native-payload.startup",startup);err!=nil{return err}
 r.mutex.Lock();if r.stopped||r.joined{r.mutex.Unlock();startup.Failure=errDispatchStopped;return startup.Failure};r.workers.Add(1);r.mutex.Unlock()
 defer r.workers.Done()
 defer func(){if !startup.Returned{interrupted=true;startup.AbnormalTermination=true;startup.Panic=recover();startup.Stack=debug.Stack()}}()
 if startup.Attempted{return errOriginalEnrollment};startup.Attempted=true
 finish:=func(err error)error{startup.Failure=err;startup.Returned=true;return err}
 startup.Arguments.Raw=startup.RawArguments
 if err:=r.observeNativeArguments(&startup.Arguments,false);err!=nil{return finish(err)}
 startup.quantities=NativeStartupQuantities{startup.Arguments.SecurityLabelBytes,startup.Arguments.TerminalCalls,startup.Arguments.RepresentationBytes}
 startup.ObservedQuantities=startup.quantities
 count,err:=nativeStartupGeometry(startup.quantities);if err!=nil{return finish(err)};startup.RawIOCapacity=count
 // Locator quantities never select backing. Only the private candidate factory
 // can supply this reservation; its construction is not native authentication.
 if startup.candidateOriginal==nil||startup.reservation==nil{return finish(errNativeReservation)}
 reservation:=startup.reservation
 if err:=r.prepareNativeStartupReservation(reservation);err!=nil{return finish(err)}
 if startup.quantities!=reservation.actualQuantities{return finish(errNativeReservation)}
 // Embedded-only native observations precede binding the prepared consumers.
 // They do not yet authenticate the controller profile or its quantity originals.
 if err:=r.ObserveEndpointCreatorDescriptor(&startup.Creator,startup.Arguments.Descriptor);err!=nil{return finish(err)}
 startup.Parent.Socket=&startup.Creator
 if err:=r.ObserveNativeParent(&startup.Parent);err!=nil{return finish(err)}
 startup.ParentProc.Parent=&startup.Parent
 if err:=r.ObserveNativeParentProc(&startup.ParentProc);err!=nil{return finish(err)}
 startup.CreatorImage.Proc=&startup.ParentProc
 if err:=r.ObserveNativeCreatorImage(&startup.CreatorImage);err!=nil{return finish(err)}
 startup.ParentContext.Proc=&startup.ParentProc;startup.ParentContext.Parent=true
 if err:=r.ObserveNativeProcessContext(&startup.ParentContext);err!=nil{return finish(err)}
 startup.PayloadContext.Proc=&startup.ParentProc
 if err:=r.ObserveNativeProcessContext(&startup.PayloadContext);err!=nil{return finish(err)}
 startup.ImageLabel.Image=&startup.CreatorImage
 if err:=r.ObserveNativeImageLabel(&startup.ImageLabel);err!=nil{return finish(err)}
 if err:=r.ObserveNativeSELinuxCurrent(&startup.Current);err!=nil{return finish(err)}
 startup.contextRelation.source=startup.candidateProfile;startup.contextRelation.current=&startup.Current
 if err:=r.Enroll(&startup.contextRelation.Original,"native-payload.candidate-context",&startup.contextRelation);err!=nil{return finish(err)}
 if err:=startup.contextRelation.interpret();err!=nil{return finish(err)}
 startup.ownerContextRelation.source=startup.candidateProfile;startup.ownerContextRelation.parent=&startup.ParentContext
 if err:=r.Enroll(&startup.ownerContextRelation.Original,"native-payload.candidate-owner-context",&startup.ownerContextRelation);err!=nil{return finish(err)}
 if err:=startup.ownerContextRelation.interpret();err!=nil{return finish(err)}

 // SAME source-prepared backing remains reachable after every partial failure.
 startup.Endpoint.Descriptor=startup.Arguments.Descriptor;startup.Endpoint.Calls=reservation.endpointCalls
 startup.Observation.Native=&startup.Endpoint;startup.Observation.Security=reservation.security
 startup.Transfer.Endpoint=&startup.Endpoint;startup.Transfer.Birth=startup;startup.Transfer.Body=reservation.body
 startup.Control.Transfer=&startup.Transfer;startup.Control.Calls=reservation.terminalCalls
 reservation.transferBacking=transferAttemptBacking{reservation.headerAttempts,reservation.bodyAttempts,reservation.replyAttempts}
 reservation.terminalBacking=terminalAttemptBacking{reservation.terminalWriteAttempts,reservation.terminalReadAttempts}
 if err:=r.PrepareNativeEndpoint(&startup.Endpoint);err!=nil{return finish(err)}
 if err:=r.ObserveEndpointSocket(&startup.Observation);err!=nil{return finish(err)}
 startup.socketContextRelation.source=startup.candidateProfile;startup.socketContextRelation.socket=&startup.Observation
 if err:=r.Enroll(&startup.socketContextRelation.Original,"native-payload.candidate-socket-context",&startup.socketContextRelation);err!=nil{return finish(err)}
 if err:=startup.socketContextRelation.interpret();err!=nil{return finish(err)}
 startup.lineage.startup=startup
 if err:=r.interpretNativeCandidateLineage(&startup.lineage);err!=nil{return finish(err)}
 if err:=r.prepareLocalRepresentation(&startup.Transfer,&reservation.transferBacking);err!=nil{return finish(err)}
 if err:=r.prepareTerminalControl(&startup.Control,&reservation.terminalBacking);err!=nil{return finish(err)}
 return finish(nil)
 }()
 if interrupted{panic(startup.Panic)}
 return failure
}
