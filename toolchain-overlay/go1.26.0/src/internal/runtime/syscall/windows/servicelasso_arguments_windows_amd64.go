//go:build windows && amd64

package windows

import "unsafe"

// Fixed originals only. The complete early acquisition caller binds actual
// authenticated endpoint reads, native completions and immutable prefixes
// before invoking this post-runtime representation decoder. Its Boolean
// result never authenticates entry or grants any native operation.
var serviceLassoArgumentHeaderOriginal [48]byte
type serviceLassoArgumentDecodeStorage struct {
 BodyExtent uint64
 ExpectedCount uint64
 UTF8Used uint64
 ParsedUsed uint64
 Count uint64
 Public []string
 Failure uint64
 Attempted bool
 Returned bool
}
var serviceLassoArgumentDecodeOriginal serviceLassoArgumentDecodeStorage

// Typed, ordinary Go borrower context. Production binds the fixed generated
// originals; tests bind a distinct fixture occurrence to the SAME methods.
// No context pointer or slice header is written by pre-runtime assembly.
type serviceLassoArgumentDecodeContext struct {
 Original *serviceLassoArgumentDecodeStorage
 Header *[48]byte
 Body []byte
 UTF8 []byte
 Parsed []byte
 Public []string
 Fallback *[1]string
}
func serviceLassoDecodePublicArguments() bool {
 c:=serviceLassoArgumentDecodeContext{
  Original:&serviceLassoArgumentDecodeOriginal,
  Header:&serviceLassoArgumentHeaderOriginal,
  Body:serviceLassoArgumentBodyOriginal[:],
  UTF8:serviceLassoArgumentUTF8Original[:],
  Parsed:serviceLassoArgumentParsedOriginal[:],
  Public:serviceLassoArgumentPublicOriginal[:],
  Fallback:&serviceLassoArgumentFallbackOriginal,
 }
 return c.decode()
}

func (c *serviceLassoArgumentDecodeContext) u64(at int) uint64 {
 h:=c.Header
 return uint64(h[at])|uint64(h[at+1])<<8|uint64(h[at+2])<<16|uint64(h[at+3])<<24|uint64(h[at+4])<<32|uint64(h[at+5])<<40|uint64(h[at+6])<<48|uint64(h[at+7])<<56
}
func (c *serviceLassoArgumentDecodeContext) putByte(value byte) bool {
 o:=c.Original
 if o.ParsedUsed>=uint64(len(c.Parsed)){return false}
 c.Parsed[o.ParsedUsed]=value;o.ParsedUsed++;return true
}
func (c *serviceLassoArgumentDecodeContext) slashes(count uint64) bool {
 for ;count>0;count-- {if !c.putByte('\\'){return false}}
 return true
}
func (c *serviceLassoArgumentDecodeContext) putRune(value uint32) bool {
 o:=c.Original
 var bytes [4]byte;var count uint64
 switch {
 case value<0x80:bytes[0]=byte(value);count=1
 case value<0x800:bytes[0]=0xc0|byte(value>>6);bytes[1]=0x80|byte(value&63);count=2
 case value<0x10000:bytes[0]=0xe0|byte(value>>12);bytes[1]=0x80|byte((value>>6)&63);bytes[2]=0x80|byte(value&63);count=3
 default:bytes[0]=0xf0|byte(value>>18);bytes[1]=0x80|byte((value>>12)&63);bytes[2]=0x80|byte((value>>6)&63);bytes[3]=0x80|byte(value&63);count=4
 }
 if o.UTF8Used>uint64(len(c.UTF8))||count>uint64(len(c.UTF8))-o.UTF8Used{return false}
 for i:=uint64(0);i<count;i++ {c.UTF8[o.UTF8Used+i]=bytes[i]};o.UTF8Used+=count;return true
}
func (c *serviceLassoArgumentDecodeContext) unit(index uint64) uint16 {
 at:=index*2;return uint16(c.Body[at])|uint16(c.Body[at+1])<<8
}

// Matches the original os constructor grammar: WTF8 conversion precedes its
// pre-2008 quoting rule, including double-quote toggling. The actual PRIVATE
// os.init constructor remains a distinct retained occurrence; this decoder
// publishes public views into fixed Parsed storage without append or string
// allocation. Partial outputs survive refusal and cannot be retried/reset.
func (c *serviceLassoArgumentDecodeContext) decode() bool {
 o:=c.Original
 if o.Attempted||o.Returned||o.Failure!=0||o.UTF8Used!=0||o.ParsedUsed!=0||o.Count!=0||o.Public!=nil{return false}
 o.Attempted=true
 if !c.decodeBody(){o.Failure=1;o.Returned=true;return false}
 o.Returned=true;return true
}
func (c *serviceLassoArgumentDecodeContext) decodeBody() bool {
 o:=c.Original;h:=c.Header
 for i:=0;i<8;i++ {if h[i]!="SLWARGV1"[i]{return false}}
 if h[8]!=1||h[9]!=0||h[10]!=0||h[11]!=0||h[13]!=0||h[14]!=0||h[15]!=0||h[12]>1{return false}
 flags:=h[12];raw:=c.u64(16);module:=c.u64(24)
 count:=c.u64(32);body:=c.u64(40)
 if raw==0||raw>32767||module>serviceLassoModuleUnitsIncludingNUL||raw>^uint64(0)-module{return false}
 units:=raw+module
 if units>^uint64(0)/2||body!=units*2||body>uint64(len(c.Body))||body!=o.BodyExtent||count!=o.ExpectedCount{return false}
 if c.unit(raw-1)!=0{return false}
 for i:=uint64(0);i+1<raw;i++ {if c.unit(i)==0{return false}}
 start:=uint64(0);characters:=raw-1
 if flags==1 {
  if raw!=1||module==0||count!=1||c.unit(units-1)!=0{return false}
  for i:=raw;i+1<units;i++ {if c.unit(i)==0{return false}}
  start=raw;characters=module-1
 }else if raw<=1||module!=0{return false}
 for i:=uint64(0);i<characters;i++ {
  value:=uint32(c.unit(start+i))
  if value>=0xd800&&value<=0xdbff&&i+1<characters {
   next:=uint32(c.unit(start+i+1))
   if next>=0xdc00&&next<=0xdfff {value=0x10000+(value-0xd800)*0x400+next-0xdc00;i++}
  }
  if !c.putRune(value){return false}
 }
 if flags==1 {
  if o.UTF8Used>uint64(len(c.Parsed)){return false}
  for i:=uint64(0);i<o.UTF8Used;i++ {c.Parsed[i]=c.UTF8[i]}
  o.ParsedUsed=o.UTF8Used
  var fallback string
  if o.ParsedUsed>0 {fallback=unsafe.String(&c.Parsed[0],int(o.ParsedUsed))}
  c.Fallback[0]=fallback
  o.Count=1;o.Public=c.Fallback[:1:1];return true
 }
 for position:=uint64(0);position<o.UTF8Used; {
  value:=c.UTF8[position]
  if value==' '||value=='\t'{position++;continue}
  if o.Count>=uint64(len(c.Public)){return false}
  begin:=o.ParsedUsed;quoted:=false;slashes:=uint64(0)
  for position<o.UTF8Used {
   value=c.UTF8[position];position++
   if (value==' '||value=='\t')&&!quoted{break}
   if value=='"' {
    if !c.slashes(slashes/2){return false}
    if slashes%2==0 {
     if quoted&&position<o.UTF8Used&&c.UTF8[position]=='"'{if !c.putByte(value){return false};position++}
     quoted=!quoted
    }else if !c.putByte(value){return false}
    slashes=0;continue
   }
   if value=='\\'{slashes++;continue}
   if !c.slashes(slashes)||!c.putByte(value){return false};slashes=0
  }
  if !c.slashes(slashes){return false}
  // Empty quoted arguments can end exactly at capacity without dereferencing
  // a nonexistent element. The zero-length string still has its own header.
  var stringView string
  if o.ParsedUsed>begin {stringView=unsafe.String(&c.Parsed[begin],int(o.ParsedUsed-begin))}
  c.Public[o.Count]=stringView;o.Count++
 }
 if o.Count!=count{return false}
 if o.Count!=0 {o.Public=c.Public[:o.Count:o.Count]}
 return true
}
