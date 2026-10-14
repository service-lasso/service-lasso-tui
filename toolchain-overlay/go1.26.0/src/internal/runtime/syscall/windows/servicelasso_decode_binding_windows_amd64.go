//go:build windows && amd64

package windows

// Ordinary typed GC roots, never written by native/preGo assembly. References
// identify retained originals; pointer equality and completion fields do not
// authenticate entry, provider, creator, containing reservation or public use.
type serviceLassoDecodeBindingStorage struct {
 Root *serviceLassoNativeRootStorage
 Private *serviceLassoPrivateConstructorStorage
 HeaderRead *serviceLassoHeaderReadStorage
 HeaderInterpret *serviceLassoHeaderInterpretStorage
 BodyRead *serviceLassoBodyReadStorage
 Decoder *serviceLassoArgumentDecodeStorage
 Failure uint64
 Attempted bool
 Returned bool
}
var serviceLassoDecodeBindingOriginal serviceLassoDecodeBindingStorage

// Called only by the complete initialized startup caller after its authentic
// entry conjunction. This prepares representation, never publishes os.Args or
// usable adoption views. Its activation caller remains separately required.
func ServiceLassoDecodeCompletedPublicArguments() bool {
 c:=serviceLassoArgumentDecodeContext{
  Original:&serviceLassoArgumentDecodeOriginal,
  Header:&serviceLassoArgumentHeaderOriginal,
  Body:serviceLassoArgumentBodyOriginal[:],UTF8:serviceLassoArgumentUTF8Original[:],
  Parsed:serviceLassoArgumentParsedOriginal[:],Public:serviceLassoArgumentPublicOriginal[:],
  Fallback:&serviceLassoArgumentFallbackOriginal,
 }
 return serviceLassoBindCompletedDecode(&serviceLassoDecodeBindingOriginal,
  &serviceLassoNativeRoot,&serviceLassoPrivateConstructorOriginal,
  &serviceLassoHeaderReadOriginal,&serviceLassoHeaderInterpretOriginal,
  &serviceLassoBodyReadOriginal,&c)
}

// Explicit occurrence/backing arguments permit independent fixture ownership;
// fixtures cannot consume or reset the production once-state.
func serviceLassoBindCompletedDecode(o *serviceLassoDecodeBindingStorage,r *serviceLassoNativeRootStorage,p *serviceLassoPrivateConstructorStorage,h *serviceLassoHeaderReadStorage,i *serviceLassoHeaderInterpretStorage,b *serviceLassoBodyReadStorage,c *serviceLassoArgumentDecodeContext) bool {
 if o==nil||o.Attempted||o.Returned||o.Failure!=0||o.Root!=nil||o.Private!=nil||o.HeaderRead!=nil||o.HeaderInterpret!=nil||o.BodyRead!=nil||o.Decoder!=nil{return false}
 o.Attempted=true;o.Root=r;o.Private=p;o.HeaderRead=h;o.HeaderInterpret=i;o.BodyRead=b
 if c!=nil{o.Decoder=c.Original}
 fail:=func()bool{o.Failure=1;o.Returned=true;return false}
 if r==nil||p==nil||h==nil||i==nil||b==nil||c==nil||c.Original==nil||c.Header==nil||c.Fallback==nil{return fail()}
 if !p.Attempted||!p.Returned||p.Failure!=0||h.Attempted!=1||h.Returned!=1||h.Failure!=0||h.Completed!=48||h.Used==0||h.Used>48||i.Attempted!=1||i.Returned!=1||i.Failure!=0||b.Attempted!=1||b.Returned!=1||b.Failure!=0||b.Used==0||b.Used>b.Completed{return fail()}
 if p.Endpoint!=r.Endpoint||h.Endpoint!=p.Endpoint||b.Endpoint!=p.Endpoint||p.PublicCount!=r.PublicCount||i.ExpectedCount!=p.PublicCount||p.SecurityBytes!=r.SecurityLabelBytes||p.TerminalCalls!=r.TerminalCalls||p.RepresentationBytes!=r.RepresentationBytes{return fail()}
 if i.BodyExtent==0||b.Completed!=i.BodyExtent||i.BodyExtent>uint64(len(c.Body)){return fail()}
 for n:=range c.Header{if c.Header[n]!=r.ArgumentHeader[n]{return fail()}}
 d:=c.Original
 if d.Attempted||d.Returned||d.Failure!=0||d.BodyExtent!=0||d.ExpectedCount!=0||d.UTF8Used!=0||d.ParsedUsed!=0||d.Count!=0||d.Public!=nil{return fail()}
 d.BodyExtent=b.Completed;d.ExpectedCount=p.PublicCount
 if !c.decode(){return fail()}
 o.Returned=true;return true
}
