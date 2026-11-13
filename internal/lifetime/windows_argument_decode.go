package lifetime

import ("encoding/binary";"errors";"unsafe")

var errWindowsArgumentDecode=errors.New("Windows original argument interpretation failed")

// These views are bound by the sole candidate constructor, not wire lengths.
// Completed source bytes and successful string views stay immutable through
// all public borrowers. Interpretation grants no endpoint or entry authority.
type windowsArgumentDecode struct {
 Original Original
 Header [48]byte
 Body,UTF8,Parsed []byte
 BodyExtent int
 Public []string
 Locator *WindowsNativeArguments
 utf8Used,parsedUsed,count int
 rawUnits,moduleUnits,bodyBytes,publicCount uint64
 flags uint32
 attempted,returned,exhausted bool
 failure error
}
func windowsDecodeSpan(p unsafe.Pointer,n uintptr,other unsafe.Pointer,m uintptr)bool {
 if n==0||m==0{return false};a,b:=uintptr(p),uintptr(other)
 if a>^uintptr(0)-n||b>^uintptr(0)-m{return true};return a<b+m&&b<a+n
}
func(o *windowsArgumentDecode)geometry()bool {
 if o==nil||o.Locator==nil||o.attempted||o.returned||o.failure!=nil||o.utf8Used!=0||o.parsedUsed!=0||o.count!=0||o.rawUnits!=0||o.moduleUnits!=0||o.bodyBytes!=0||o.publicCount!=0||o.flags!=0||o.exhausted{return false}
 if len(o.Body)==0||len(o.UTF8)==0||len(o.Parsed)==0||len(o.Public)==0||cap(o.Body)!=len(o.Body)||cap(o.UTF8)!=len(o.UTF8)||cap(o.Parsed)!=len(o.Parsed)||cap(o.Public)!=len(o.Public){return false}
 if uintptr(len(o.Public))>^uintptr(0)/unsafe.Sizeof(string("")){return false}
 pointers:=[4]unsafe.Pointer{unsafe.Pointer(unsafe.SliceData(o.Body)),unsafe.Pointer(unsafe.SliceData(o.UTF8)),unsafe.Pointer(unsafe.SliceData(o.Parsed)),unsafe.Pointer(unsafe.SliceData(o.Public))}
 sizes:=[4]uintptr{uintptr(len(o.Body)),uintptr(len(o.UTF8)),uintptr(len(o.Parsed)),uintptr(len(o.Public))*unsafe.Sizeof(string(""))}
 for i:=range pointers{
  if windowsDecodeSpan(pointers[i],sizes[i],unsafe.Pointer(o),unsafe.Sizeof(*o))||windowsDecodeSpan(pointers[i],sizes[i],unsafe.Pointer(o.Locator),unsafe.Sizeof(*o.Locator)){return false}
  for j:=0;j<i;j++{if windowsDecodeSpan(pointers[i],sizes[i],pointers[j],sizes[j]){return false}}
 }
 for i:=range o.Public{if o.Public[i]!=""{return false}}
 return true
}
func(o *windowsArgumentDecode)putUTF8(c uint32)bool {
 var b [4]byte;n:=0
 switch{case c<0x80:b[0]=byte(c);n=1
 case c<0x800:b[0]=0xc0|byte(c>>6);b[1]=0x80|byte(c&63);n=2
 case c<0x10000:b[0]=0xe0|byte(c>>12);b[1]=0x80|byte((c>>6)&63);b[2]=0x80|byte(c&63);n=3
 default:b[0]=0xf0|byte(c>>18);b[1]=0x80|byte((c>>12)&63);b[2]=0x80|byte((c>>6)&63);b[3]=0x80|byte(c&63);n=4}
 if n>len(o.UTF8)-o.utf8Used{o.exhausted=true;return false}
 copy(o.UTF8[o.utf8Used:o.utf8Used+n],b[:n]);o.utf8Used+=n;return true
}
func(o *windowsArgumentDecode)putParsed(c byte)bool {
 if o.parsedUsed==len(o.Parsed){o.exhausted=true;return false};o.Parsed[o.parsedUsed]=c;o.parsedUsed++;return true
}
func(o *windowsArgumentDecode)slashes(n int)bool {for ;n>0;n--{if !o.putParsed('\\'){return false}};return true}
func(o *windowsArgumentDecode)interpret()error {
 fail:=func()error{return errWindowsArgumentDecode}
 h:=o.Header[:]
 for i:=0;i<8;i++{if h[i]!="SLWARGV1"[i]{return fail()}};if binary.LittleEndian.Uint32(h[8:12])!=1{return fail()}
 o.flags=binary.LittleEndian.Uint32(h[12:16]);o.rawUnits=binary.LittleEndian.Uint64(h[16:24]);o.moduleUnits=binary.LittleEndian.Uint64(h[24:32]);o.publicCount=binary.LittleEndian.Uint64(h[32:40]);o.bodyBytes=binary.LittleEndian.Uint64(h[40:48])
 if o.flags>1||o.rawUnits==0||o.rawUnits>^uint64(0)-o.moduleUnits{return fail()};total:=o.rawUnits+o.moduleUnits
 if total>^uint64(0)/2||o.bodyBytes!=total*2||o.bodyBytes>uint64(len(o.Body))||o.BodyExtent<0||o.BodyExtent>len(o.Body)||o.bodyBytes!=uint64(o.BodyExtent){return fail()}
 if o.Locator.Failure!=nil||!o.Locator.Returned||o.Locator.PublicCount<0||uint64(o.Locator.PublicCount)!=o.publicCount{return fail()}
 // Body capacity may exceed the completed transfer extent; only its exact
 // original completed prefix is interpreted. The caller owns IO lineage.
 unit:=func(i uint64)uint16{return binary.LittleEndian.Uint16(o.Body[int(i*2):int(i*2)+2])}
 if unit(o.rawUnits-1)!=0{return fail()};for i:=uint64(0);i+1<o.rawUnits;i++{if unit(i)==0{return fail()}}
 start,units:=uint64(0),o.rawUnits-1
 if o.flags==1{
  if o.rawUnits!=1||o.moduleUnits==0||o.publicCount!=1||unit(total-1)!=0{return fail()}
  for i:=o.rawUnits;i+1<total;i++{if unit(i)==0{return fail()}};start=o.rawUnits;units=o.moduleUnits-1
 }else if o.rawUnits<=1||o.moduleUnits!=0{return fail()}
 for i:=uint64(0);i<units;i++{
  c:=uint32(unit(start+i))
  if c>=0xd800&&c<=0xdbff&&i+1<units{next:=uint32(unit(start+i+1));if next>=0xdc00&&next<=0xdfff{c=0x10000+(c-0xd800)*0x400+next-0xdc00;i++}}
  if !o.putUTF8(c){return fail()}
 }
 if o.flags==1{
  if o.utf8Used>len(o.Parsed){o.exhausted=true;return fail()};copy(o.Parsed,o.UTF8[:o.utf8Used]);o.parsedUsed=o.utf8Used
  o.Public[0]=unsafe.String(unsafe.SliceData(o.Parsed),o.parsedUsed);o.count=1
 }else{
  for pos:=0;pos<o.utf8Used;{
   if o.UTF8[pos]==' '||o.UTF8[pos]=='\t'{pos++;continue}
   if o.count==len(o.Public){o.exhausted=true;return fail()};begin:=o.parsedUsed;quoted:=false;slashes:=0
   for pos<o.utf8Used{
    c:=o.UTF8[pos];pos++
    if (c==' '||c=='\t')&&!quoted{break}
    if c=='"'{if !o.slashes(slashes/2){return fail()};if slashes%2==0{if quoted&&pos<o.utf8Used&&o.UTF8[pos]=='"'{if !o.putParsed(c){return fail()};pos++};quoted=!quoted}else if !o.putParsed(c){return fail()};slashes=0;continue}
    if c=='\\'{slashes++;continue}
    if !o.slashes(slashes)||!o.putParsed(c){return fail()};slashes=0
   }
   if !o.slashes(slashes){return fail()};o.Public[o.count]=unsafe.String(unsafe.SliceData(o.Parsed[begin:]),o.parsedUsed-begin);o.count++
  }
 }
 if uint64(o.count)!=o.publicCount{return fail()};return nil
}
func(r *Receiver)interpretWindowsArguments(o *windowsArgumentDecode)error {
 if r==nil||!o.geometry(){return errOriginalEnrollment}
 if o.Locator.Original.owner!=r||o.Locator.Original.Value!=o.Locator||!o.Locator.Attempted||!o.Locator.Returned||o.Locator.Failure!=nil{return errOriginalEnrollment}
 if err:=r.Enroll(&o.Original,"native-payload.windows-original-argument-interpretation",o);err!=nil{return err}
 o.attempted=true;o.failure=o.interpret();o.returned=true;return o.failure
}
