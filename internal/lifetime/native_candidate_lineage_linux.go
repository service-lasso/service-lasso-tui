//go:build linux
package lifetime
import("errors";"unsafe")
var errNativeCandidateLineage=errors.New("native candidate creator occurrence correspondence invalid")
// This operation joins actual observed occurrences. It is not a loaded-code,
// policy, actor or endpoint-supplier admission result.
type nativeCandidateLineage struct{
 Original Original
 startup,actualStartup *NativePayloadStartup
 first,second *EndpointSocketObservation
 parent *NativeParentObservation
 proc *NativeParentProc
 image *NativeCreatorImage
 parentContext *NativeProcessContext
 payloadContext *NativeSELinuxCurrent
 imageLabel *NativeImageLabel
 ownerRelation,payloadRelation,socketRelation *nativeCandidateContext
 descriptor uintptr
 Attempted,Returned bool
 Failure error
}
func(r *Receiver)interpretNativeCandidateLineage(o *nativeCandidateLineage)error{
 if r==nil||o==nil{return errOriginalEnrollment}
 if err:=r.Enroll(&o.Original,"native-payload.creator-occurrence-joins",o);err!=nil{return err}
 if o.Attempted{return errOriginalEnrollment};o.Attempted=true;o.actualStartup=o.startup
 finish:=func(err error)error{o.Failure=err;o.Returned=true;return err}
 s:=o.actualStartup
 if s==nil||s.Original.owner!=r||!s.Attempted||s.Returned||s.candidateOriginal==nil||s.reservation==nil||!s.reservation.Returned||s.reservation.Failure!=nil||s.reservation.original.owner!=r||s.reservation.actualQuantities!=s.quantities||!s.Arguments.Returned||s.Arguments.Failure!=nil{return finish(errNativeCandidateLineage)}
 o.first=&s.Creator;o.second=&s.Observation;o.parent=&s.Parent;o.proc=&s.ParentProc;o.image=&s.CreatorImage;o.parentContext=&s.ParentContext;o.payloadContext=&s.Current;o.imageLabel=&s.ImageLabel;o.ownerRelation=&s.ownerContextRelation;o.payloadRelation=&s.contextRelation;o.socketRelation=&s.socketContextRelation;o.descriptor=uintptr(s.Arguments.Descriptor)
 for _,original:=range [...]*Original{&s.Arguments.Original,&o.first.Original,&o.second.Original,&o.parent.Original,&o.proc.Original,&o.image.Original,&o.parentContext.Original,&o.payloadContext.Original,&o.imageLabel.Original,&o.ownerRelation.Original,&o.payloadRelation.Original,&o.socketRelation.Original,&s.Endpoint.Original}{if original.owner!=r{return finish(errNativeCandidateLineage)}}
 if !o.first.Returned||o.first.Failure!=nil||!o.first.locatorOnly||o.first.locator!=s.Arguments.Descriptor||o.first.Descriptor!=o.descriptor||!o.first.CredentialReturned||o.first.CredentialErrno!=0||o.first.CredentialReturn!=0||o.first.CredentialLength!=uint32(unsafe.Sizeof(o.first.Credentials)){return finish(errNativeCandidateLineage)}
 if !o.second.Returned||o.second.Failure!=nil||o.second.native!=&s.Endpoint||o.second.Descriptor!=o.descriptor||o.second.creatorCredentials!=o.first.creatorCredentials||!o.second.CredentialReturned||o.second.CredentialErrno!=0||o.second.CredentialReturn!=0||o.second.CredentialLength!=uint32(unsafe.Sizeof(o.second.Credentials))||!o.second.SecurityReturned||o.second.SecurityErrno!=0||o.second.SecurityReturn!=0||o.second.SecurityLength==0||uint64(o.second.SecurityLength)>uint64(len(o.second.security)){return finish(errNativeCandidateLineage)}
 if !s.Endpoint.prepared||s.Endpoint.receiver!=r||s.Endpoint.descriptor!=s.Arguments.Descriptor||o.second.security==nil||len(o.second.security)!=len(s.reservation.security)||&o.second.security[0]!=&s.reservation.security[0]{return finish(errNativeCandidateLineage)}
 if !o.parent.Returned||o.parent.Failure!=nil||o.parent.socket!=o.first||o.parent.HeldDescriptor<3||o.parent.Before.Result!=uintptr(o.first.creatorCredentials.Pid)||o.parent.After.Result!=o.parent.Before.Result{return finish(errNativeCandidateLineage)}
 if !o.proc.Returned||o.proc.Failure!=nil||!o.proc.EOF||o.proc.Exhausted||o.proc.parent!=o.parent||o.proc.Root.HeldFD<3||o.proc.PID<=0{return finish(errNativeCandidateLineage)}
 if !o.image.Returned||o.image.Failure!=nil||o.image.proc!=o.proc||o.image.ActualRoot!=o.proc.Root.HeldFD||o.image.ActualPID!=o.proc.PID||o.image.View.HeldFD<3||!o.image.Metadata.Returned||o.image.Metadata.Errno!=0{return finish(errNativeCandidateLineage)}
 if !o.parentContext.Returned||o.parentContext.Failure!=nil||o.parentContext.proc!=o.proc||!o.parentContext.parent||!o.parentContext.EOF||o.parentContext.Exhausted||!o.payloadContext.Returned||o.payloadContext.Failure!=nil||!o.payloadContext.FramingComplete||!o.imageLabel.Returned||o.imageLabel.Failure!=nil||o.imageLabel.image!=o.image{return finish(errNativeCandidateLineage)}
 if !o.ownerRelation.Returned||o.ownerRelation.Failure!=nil||o.ownerRelation.parent!=o.parentContext||!o.payloadRelation.Returned||o.payloadRelation.Failure!=nil||o.payloadRelation.current!=o.payloadContext||len(o.ownerRelation.source)!=len(s.candidateProfile)||len(o.payloadRelation.source)!=len(s.candidateProfile)||len(s.candidateProfile)==0||&o.ownerRelation.source[0]!=&s.candidateProfile[0]||&o.payloadRelation.source[0]!=&s.candidateProfile[0]{return finish(errNativeCandidateLineage)}
 if !o.socketRelation.Returned||o.socketRelation.Failure!=nil||o.socketRelation.socket!=o.second||len(o.socketRelation.source)!=len(s.candidateProfile)||&o.socketRelation.source[0]!=&s.candidateProfile[0]{return finish(errNativeCandidateLineage)}
 return finish(nil)
}