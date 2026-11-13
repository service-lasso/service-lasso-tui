//go:build linux

package lifetime

import("encoding/binary";"errors")
var errNativeCandidateContext=errors.New("native candidate postexec context correspondence invalid")
type nativeCandidateRecord struct{original []byte;fields [8][]byte;count int}
// These are source aliases and comparison originals, never an authorization token.
type nativeCandidateContext struct{
 Original Original
 source []byte
 current *NativeSELinuxCurrent
 parent *NativeProcessContext
 socket *EndpointSocketObservation
 raw []byte
 kernelInterpretation []byte
 meaning string
 sections [9][]byte
 declaration,evidence nativeCandidateRecord
 statement,expected,actual []byte
 cursor,sectionIndex,recordsExamined,evidenceExamined,bytesCompared int
 declarations uint64
 evidenceIndex,statementOffset,statementExtent uint64
 Attempted,Returned bool
 Failure error
}
func nativeCandidateMeaning(a []byte,meaning string)bool{if len(a)!=len(meaning){return false};for i,v:=range a{if v!=meaning[i]{return false}};return true}
func nativeCandidateEqual(a,b []byte)bool{if len(a)!=len(b){return false};for i,v:=range a{if v!=b[i]{return false}};return true}
// Advance keeps each field within SAME original section; no record-sized allocation.
func nativeCandidateNext(section []byte,at *int,fields int,out *nativeCandidateRecord)bool{
 if fields<1||fields>8||*at<0||*at>len(section)||len(section)-*at<fields*8{return false}
 begin:=*at;var lengths [8]uint64
 for i:=0;i<fields;i++{lengths[i]=binary.LittleEndian.Uint64(section[*at+i*8:*at+i*8+8])};*at+=fields*8
 *out=nativeCandidateRecord{count:fields}
 for i:=0;i<fields;i++{n:=lengths[i];if n>uint64(len(section)-*at){return false};end:=*at+int(n);out.fields[i]=section[*at:end:end];*at=end}
 out.original=section[begin:*at:*at];return true
}
func(o *nativeCandidateContext)interpret()error{
 if o.Attempted{return errOriginalEnrollment};o.Attempted=true
 finish:=func(err error)error{o.Failure=err;o.Returned=true;return err}
 b:=o.source;c:=o.current
 if len(b)<32{return finish(errNativeCandidateContext)}
 if o.socket!=nil{
  p:=o.socket
  if c!=nil||o.parent!=nil||!p.Returned||p.Failure!=nil||!p.SecurityReturned||p.SecurityErrno!=0||p.SecurityReturn!=0||p.SecurityLength==0||uint64(p.SecurityLength)>uint64(len(p.security)){return finish(errNativeCandidateContext)}
  n:=int(p.SecurityLength);o.raw=p.security[:n:n];if o.raw[n-1]==0{n--};if n==0{return finish(errNativeCandidateContext)}
  for i:=0;i<n;i++{if o.raw[i]==0{return finish(errNativeCandidateContext)}}
  o.actual=o.raw[:n:n];o.meaning="service-lasso.owner-endpoint-context.v1"
 }else if o.parent!=nil{
  // Conditional interpretation under independently admitted exact kernel/LSM
  // hook-order premise. This raw comparison cannot establish that premise.
  p:=o.parent
  if c!=nil||!p.Returned||p.Failure!=nil||!p.parent||!p.EOF||p.Exhausted||p.Length<2||p.Length>len(p.Body)||p.Body[p.Length-1]!=0{return finish(errNativeCandidateContext)}
  for i:=0;i<p.Length-1;i++{if p.Body[i]==0{return finish(errNativeCandidateContext)}}
  o.raw=p.Body[:p.Length:p.Length];o.actual=p.Body[:p.Length-1:p.Length-1];o.meaning="service-lasso.owner-context.v1"
 }else{
  if c==nil||!c.Returned||c.Failure!=nil||!c.FramingComplete||len(c.Context)==0{return finish(errNativeCandidateContext)}
  o.raw=c.Context;o.actual=c.Context;o.meaning="service-lasso.payload-context.v1"
 }
 magic:=[8]byte{0x53,0x4c,0x43,0x50,0x52,0x4f,0x46,0x31}
 for i,v:=range magic{if b[i]!=v{return finish(errNativeCandidateContext)}}
 if binary.LittleEndian.Uint32(b[8:12])!=1||binary.LittleEndian.Uint32(b[12:16])!=9||binary.LittleEndian.Uint64(b[16:24])!=uint64(len(b))||binary.LittleEndian.Uint64(b[24:32])!=0{return finish(errNativeCandidateContext)}
 o.cursor=32
 for i:=range o.sections{o.sectionIndex=i;if len(b)-o.cursor<16{return finish(errNativeCandidateContext)};at:=o.cursor
  if binary.LittleEndian.Uint32(b[at:at+4])!=uint32(i+1)||binary.LittleEndian.Uint32(b[at+4:at+8])!=0{return finish(errNativeCandidateContext)}
  n:=binary.LittleEndian.Uint64(b[at+8:at+16]);o.cursor+=16;if n>uint64(len(b)-o.cursor){return finish(errNativeCandidateContext)};end:=o.cursor+int(n);o.sections[i]=b[o.cursor:end:end];o.cursor=end
 }
 if o.cursor!=len(b)||len(o.sections[0])!=32||len(o.sections[1])!=32||len(o.sections[2])!=72||len(o.sections[3])!=24{return finish(errNativeCandidateContext)}
 o.kernelInterpretation=o.sections[2][40:72:72]
 entries:=o.sections[6];evidence:=o.sections[4]
 if len(entries)<8||len(evidence)<8{return finish(errNativeCandidateContext)}
 count:=binary.LittleEndian.Uint64(entries[:8]);ecount:=binary.LittleEndian.Uint64(evidence[:8])
 if count>uint64((len(entries)-8)/24)||ecount>uint64((len(evidence)-8)/56){return finish(errNativeCandidateContext)}
 at:=8
 for i:=uint64(0);i<count;i++{var row nativeCandidateRecord;o.recordsExamined++
  if !nativeCandidateNext(entries,&at,3,&row){return finish(errNativeCandidateContext)}
  if !nativeCandidateMeaning(row.fields[1],o.meaning){continue}
  o.declaration=row;o.declarations++;link:=row.fields[0];proposed:=row.fields[2]
  if len(link)!=24||len(proposed)==0{return finish(errNativeCandidateContext)}
  o.evidenceIndex=binary.LittleEndian.Uint64(link[:8]);o.statementOffset=binary.LittleEndian.Uint64(link[8:16]);o.statementExtent=binary.LittleEndian.Uint64(link[16:24])
  if o.evidenceIndex>=ecount||o.statementExtent==0{return finish(errNativeCandidateContext)}
  eAt:=8
  for j:=uint64(0);j<ecount;j++{var original nativeCandidateRecord;o.evidenceExamined++
   if !nativeCandidateNext(evidence,&eAt,7,&original){return finish(errNativeCandidateContext)}
   for k:=0;k<7;k++{if k!=4&&len(original.fields[k])==0{return finish(errNativeCandidateContext)}}
   if j==o.evidenceIndex{o.evidence=original}
  }
  if eAt!=len(evidence){return finish(errNativeCandidateContext)}
  body:=o.evidence.fields[6]
  if o.statementOffset>uint64(len(body))||o.statementExtent>uint64(len(body))-o.statementOffset{return finish(errNativeCandidateContext)}
  start:=int(o.statementOffset);end:=start+int(o.statementExtent);o.statement=body[start:end:end]
  if o.expected!=nil&&!nativeCandidateEqual(o.expected,proposed){return finish(errNativeCandidateContext)};o.expected=proposed
  if len(proposed)!=len(o.actual){return finish(errNativeCandidateContext)}
  for j,value:=range proposed{o.bytesCompared++;if value!=o.actual[j]{return finish(errNativeCandidateContext)}}
 }
 if at!=len(entries)||o.declarations==0{return finish(errNativeCandidateContext)}
 return finish(nil)
 // Exact source context comparison only. Creator, policy, entry, supplier and
 // maintenance episode consumers remain separate; no stream IO occurs here.
}