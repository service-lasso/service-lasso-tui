//go:build linux
package lifetime
import("encoding/binary";"testing")
// UNRUN source cases. Fixture encoding supplies no native or author authority.
func candidateContextTestRecords(rows ...[][]byte)[]byte{
 b:=make([]byte,8);binary.LittleEndian.PutUint64(b,uint64(len(rows)))
 for _,row:=range rows{lengths:=make([]byte,len(row)*8);for i,f:=range row{binary.LittleEndian.PutUint64(lengths[i*8:],uint64(len(f)))};b=append(b,lengths...);for _,f:=range row{b=append(b,f...)}}
 return b
}
func candidateContextTestProfile(contexts ...string)[]byte{return candidateContextTestProfileMeaning("service-lasso.payload-context.v1",contexts...)}
func candidateContextTestProfileMeaning(meaning string,contexts ...string)[]byte{
 sections:=[9][]byte{make([]byte,32),make([]byte,32),make([]byte,72),make([]byte,24)}
 sections[4]=candidateContextTestRecords([][]byte{[]byte("source"),[]byte("author original"),[]byte("subject"),[]byte("scope"),nil,[]byte("meaning"),[]byte("statement")})
 rows:=make([][][]byte,len(contexts));for i,c:=range contexts{link:=make([]byte,24);binary.LittleEndian.PutUint64(link[16:],9);rows[i]=[][]byte{link,[]byte(meaning),[]byte(c)}}
 sections[6]=candidateContextTestRecords(rows...);sections[5]=candidateContextTestRecords();sections[7]=candidateContextTestRecords();sections[8]=candidateContextTestRecords()
 b:=make([]byte,32);copy(b,"SLCPROF1");binary.LittleEndian.PutUint32(b[8:],1);binary.LittleEndian.PutUint32(b[12:],9)
 for i,s:=range sections{head:=make([]byte,16);binary.LittleEndian.PutUint32(head,uint32(i+1));binary.LittleEndian.PutUint64(head[8:],uint64(len(s)));b=append(b,head...);b=append(b,s...)};binary.LittleEndian.PutUint64(b[16:],uint64(len(b)));return b
}
func TestCandidatePostexecContextOriginals(t *testing.T){
 raw:=candidateContextTestProfile("u:r:t:s0","u:r:t:s0");current:=NativeSELinuxCurrent{Returned:true,FramingComplete:true};copy(current.Body[32:],"u:r:t:s0");current.Context=current.Body[32:40:40]
 o:=nativeCandidateContext{source:raw,current:&current};if err:=o.interpret();err!=nil{t.Fatal(err)}
 if o.declarations!=2||len(o.statement)!=9||&o.actual[0]!=&current.Body[32]||&o.expected[0]!=&o.declaration.fields[2][0]||&o.statement[0]!=&o.evidence.fields[6][0]{t.Fatal("original source spans lost")}
 if err:=o.interpret();err!=errOriginalEnrollment||o.Failure!=nil||o.declarations!=2{t.Fatal("repeat changed completed original")}
}
func TestCandidatePostexecContextDenials(t *testing.T){
 for _,tc:=range []struct{name string;contexts []string;actual string}{
  {"mismatch",[]string{"u:r:t:s0"},"u:r:x:s0"},
  {"conflicting originals",[]string{"u:r:t:s0","u:r:x:s0"},"u:r:t:s0"},
  {"absent declaration",nil,"u:r:t:s0"},
 }{t.Run(tc.name,func(t *testing.T){current:=NativeSELinuxCurrent{Returned:true,FramingComplete:true,Context:[]byte(tc.actual)};o:=nativeCandidateContext{source:candidateContextTestProfile(tc.contexts...),current:&current};if o.interpret()!=errNativeCandidateContext||!o.Returned||o.Failure!=errNativeCandidateContext{t.Fatal("missing actual denial")}})}
}
func TestCandidatePostexecEvidenceExtentAndTruncation(t *testing.T){
 for mode:=0;mode<2;mode++{raw:=candidateContextTestProfile("u:r:t:s0");current:=NativeSELinuxCurrent{Returned:true,FramingComplete:true,Context:[]byte("u:r:t:s0")}
  if mode==0{raw=raw[:len(raw)-1]}else{at:=32;for i:=0;i<6;i++{at+=16+int(binary.LittleEndian.Uint64(raw[at+8:]))};at+=16+8+24;binary.LittleEndian.PutUint64(raw[at:],1)}
  o:=nativeCandidateContext{source:raw,current:&current};if o.interpret()!=errNativeCandidateContext{t.Fatal("accepted truncated or unmatched evidence")}
 }
}
// The fixture models ONLY comparison under an externally qualified hook-order
// premise; it does not establish that premise or actual creator authentication.
func TestCandidateOwnerContextConditionalOriginals(t *testing.T){
 raw:=candidateContextTestProfileMeaning("service-lasso.owner-context.v1","u:r:owner:s0")
 parent:=NativeProcessContext{Returned:true,EOF:true,parent:true,Length:13};copy(parent.Body[:],"u:r:owner:s0\x00")
 o:=nativeCandidateContext{source:raw,parent:&parent};if err:=o.interpret();err!=nil{t.Fatal(err)}
 if &o.actual[0]!=&parent.Body[0]||len(o.actual)!=12||cap(o.actual)!=12||&o.kernelInterpretation[0]!=&o.sections[2][40]||o.meaning!="service-lasso.owner-context.v1"{t.Fatal("lost parent/kernel source aliases")}
}
func TestCandidateOwnerContextMalformedOrOtherMeaning(t *testing.T){
 for mode:=0;mode<4;mode++{raw:=candidateContextTestProfileMeaning("service-lasso.owner-context.v1","u:r:owner:s0");parent:=NativeProcessContext{Returned:true,EOF:true,parent:true,Length:13};copy(parent.Body[:],"u:r:owner:s0\x00")
  if mode==0{parent.Body[12]='x'};if mode==1{parent.Body[2]=0};if mode==2{parent.Exhausted=true};if mode==3{raw=candidateContextTestProfile("u:r:owner:s0")}
  o:=nativeCandidateContext{source:raw,parent:&parent};if o.interpret()!=errNativeCandidateContext{t.Fatal("accepted unavailable/framing/wrong meaning")}
 }
}
func TestCandidatePeerSocketContextKeepsRawExtent(t *testing.T){
 for mode:=0;mode<2;mode++{raw:=candidateContextTestProfileMeaning("service-lasso.owner-endpoint-context.v1","u:r:socket:s0");security:=[]byte("u:r:socket:s0");if mode==1{security=append(security,0)}
  peer:=EndpointSocketObservation{Returned:true,SecurityReturned:true,SecurityLength:uint32(len(security)),security:security};o:=nativeCandidateContext{source:raw,socket:&peer}
  if o.interpret()!=nil||len(o.raw)!=len(security)||&o.raw[0]!=&security[0]||&o.actual[0]!=&security[0]||len(o.actual)!=13||cap(o.actual)!=13{t.Fatal("lost exact peer socket original")}
 }
}
func TestCandidatePeerSocketContextDenials(t *testing.T){
 for mode:=0;mode<4;mode++{raw:=candidateContextTestProfileMeaning("service-lasso.owner-endpoint-context.v1","u:r:socket:s0");security:=[]byte("u:r:socket:s0");peer:=EndpointSocketObservation{Returned:true,SecurityReturned:true,SecurityLength:uint32(len(security)),security:security}
  if mode==0{security[2]=0};if mode==1{peer.SecurityLength++};if mode==2{raw=candidateContextTestProfileMeaning("service-lasso.owner-context.v1","u:r:socket:s0")};if mode==3{security[4]='x'}
  o:=nativeCandidateContext{source:raw,socket:&peer};if o.interpret()!=errNativeCandidateContext{t.Fatal("accepted wrong socket context/extent/meaning")}
 }
}