//go:build linux
package lifetime
import("testing";"unsafe";"syscall")
// UNRUN controlled occurrence joins. Manually prepared observations provide no
// native loaded-code, kernel-policy, actor or creator authentication evidence.
func candidateLineageFixture(r *Receiver)*NativePayloadStartup{
 s:=&NativePayloadStartup{Attempted:true,candidateOriginal:new(byte),candidateProfile:[]byte{1}};s.reservation=&nativeStartupReservation{Returned:true,security:[]byte{1}}
 s.Arguments.Returned=true;s.Arguments.Descriptor=7
 s.Creator.Returned=true;s.Creator.locatorOnly=true;s.Creator.locator=7;s.Creator.Descriptor=7;s.Creator.CredentialReturned=true;s.Creator.CredentialLength=uint32(unsafe.Sizeof(syscall.Ucred{}));s.Creator.creatorCredentials=syscall.Ucred{Pid:9,Uid:11,Gid:12}
 s.Observation=s.Creator;s.Observation.locatorOnly=false;s.Observation.native=&s.Endpoint;s.Observation.security=s.reservation.security;s.Observation.SecurityReturned=true;s.Observation.SecurityLength=1
 s.Endpoint.prepared=true;s.Endpoint.receiver=r;s.Endpoint.descriptor=7
 s.Parent.Returned=true;s.Parent.socket=&s.Creator;s.Parent.HeldDescriptor=8;s.Parent.Before.Result=9;s.Parent.After.Result=9
 s.ParentProc.Returned=true;s.ParentProc.EOF=true;s.ParentProc.parent=&s.Parent;s.ParentProc.Root.HeldFD=10;s.ParentProc.PID=19
 s.CreatorImage.Returned=true;s.CreatorImage.proc=&s.ParentProc;s.CreatorImage.ActualRoot=10;s.CreatorImage.ActualPID=19;s.CreatorImage.View.HeldFD=11;s.CreatorImage.Metadata.Returned=true
 s.ParentContext.Returned=true;s.ParentContext.parent=true;s.ParentContext.EOF=true;s.ParentContext.proc=&s.ParentProc
 s.Current.Returned=true;s.Current.FramingComplete=true;s.ImageLabel.Returned=true;s.ImageLabel.image=&s.CreatorImage
 s.ownerContextRelation.Returned=true;s.ownerContextRelation.parent=&s.ParentContext;s.ownerContextRelation.source=s.candidateProfile
 s.socketContextRelation.Returned=true;s.socketContextRelation.socket=&s.Observation;s.socketContextRelation.source=s.candidateProfile
 s.contextRelation.Returned=true;s.contextRelation.current=&s.Current;s.contextRelation.source=s.candidateProfile
 for _,original:=range [...]*Original{&s.Original,&s.reservation.original,&s.Arguments.Original,&s.Creator.Original,&s.Observation.Original,&s.Parent.Original,&s.ParentProc.Original,&s.CreatorImage.Original,&s.ParentContext.Original,&s.Current.Original,&s.ImageLabel.Original,&s.ownerContextRelation.Original,&s.contextRelation.Original,&s.socketContextRelation.Original,&s.Endpoint.Original}{original.owner=r}
 return s
}
func TestCandidateLineageSameOccurrencesAndDenials(t *testing.T){
 for mode:=0;mode<6;mode++{r:=&Receiver{};s:=candidateLineageFixture(r)
  if mode==1{s.Observation.creatorCredentials.Uid++};if mode==2{s.CreatorImage.proc=&NativeParentProc{}};if mode==3{s.ownerContextRelation.source=[]byte{1}};if mode==4{s.Observation.security=[]byte{1}};if mode==5{s.Parent.socket=&EndpointSocketObservation{}}
  o:=nativeCandidateLineage{startup:s};err:=r.interpretNativeCandidateLineage(&o)
  if mode==0{if err!=nil||o.first!=&s.Creator||o.second!=&s.Observation||o.image!=&s.CreatorImage||o.ownerRelation!=&s.ownerContextRelation{t.Fatal("lost SAME occurrence")}}else if err!=errNativeCandidateLineage||o.Failure!=err{t.Fatal("accepted substituted occurrence")}
  if !o.Returned||s.Transfer.Original.owner!=nil||s.Control.Original.owner!=nil{t.Fatal("unexpected transfer effects")}
  if r.interpretNativeCandidateLineage(&o)!=errOriginalEnrollment{t.Fatal("replayed lineage comparison")}
 }
}