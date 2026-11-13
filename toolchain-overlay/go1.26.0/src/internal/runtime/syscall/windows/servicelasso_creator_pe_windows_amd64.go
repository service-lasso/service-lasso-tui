//go:build windows && amd64

package windows

import "unsafe"

// Initialized-Go, readonly representation consumer. This does not authorize
// pre-runtime entry, native reads, loaded-byte correspondence or publication.
// All slices and native owners are retained as typed GC roots on refusal too.
type serviceLassoCreatorPE struct {
 Read *serviceLassoCreatorFileReadStorage
 Metadata *serviceLassoCreatorMetadataStorage
 Bytes []byte
 Attempted bool
 Complete bool
 Failure uint64
 Machine uint16
 Characteristics uint16
 Sections uint16
 SectionTable uint64
 Optional uint64
 OptionalBytes uint64
 ImageBase uint64
 Entry uint64
 ImageBytes uint64
 HeaderBytes uint64
 SectionAlignment uint64
 FileAlignment uint64
 Directories uint64
}

func serviceLassoPEWithin(offset, size, limit uint64) bool {
 return offset <= limit && size <= limit-offset
}
func serviceLassoPE16(b []byte, o uint64) uint16 {
 return uint16(b[o]) | uint16(b[o+1])<<8
}
func serviceLassoPE32(b []byte, o uint64) uint64 {
 return uint64(serviceLassoPE16(b,o)) | uint64(serviceLassoPE16(b,o+2))<<16
}
func serviceLassoPE64(b []byte, o uint64) uint64 {
 return serviceLassoPE32(b,o) | serviceLassoPE32(b,o+4)<<32
}
func serviceLassoPEPower(v uint64) bool { return v != 0 && v&(v-1)==0 }
func serviceLassoPEDisjoint(a,an,b,bn uintptr) bool {
 if an>^uintptr(0)-a || bn>^uintptr(0)-b { return false }
 return an==0 || bn==0 || a+an<=b || b+bn<=a
}
func serviceLassoPERound(v,a uint64) (uint64,bool) {
 if !serviceLassoPEPower(a) || v > ^uint64(0)-(a-1) { return 0,false }
 return (v+a-1)&^(a-1),true
}

// Caller supplies the SAME exact completed file view, not an independently
// copied buffer whose equality is inferred from length or metadata identity.
// Backing association and native immutability through last borrower remain
// obligations of the owning acquisition caller.
func (p *serviceLassoCreatorPE) Interpret(r *serviceLassoCreatorFileReadStorage, m *serviceLassoCreatorMetadataStorage, b []byte) bool {
 if p==nil || p.Attempted || p.Complete || p.Read!=nil || p.Metadata!=nil || p.Bytes!=nil || p.Failure!=0 ||
  p.Machine!=0 || p.Characteristics!=0 || p.Sections!=0 || p.SectionTable!=0 || p.Optional!=0 || p.OptionalBytes!=0 ||
  p.ImageBase!=0 || p.Entry!=0 || p.ImageBytes!=0 || p.HeaderBytes!=0 || p.SectionAlignment!=0 || p.FileAlignment!=0 || p.Directories!=0 { return false }
 // Refuse overlapping output before any capture write can damage originals.
 pa:=uintptr(unsafe.Pointer(p)); pn:=unsafe.Sizeof(*p)
 if r!=nil && !serviceLassoPEDisjoint(pa,pn,uintptr(unsafe.Pointer(r)),unsafe.Sizeof(*r)) { return false }
 if m!=nil && !serviceLassoPEDisjoint(pa,pn,uintptr(unsafe.Pointer(m)),unsafe.Sizeof(*m)) { return false }
 if len(b)!=0 && !serviceLassoPEDisjoint(pa,pn,uintptr(unsafe.Pointer(&b[0])),uintptr(len(b))) { return false }
 p.Read=r; p.Metadata=m; p.Bytes=b; p.Attempted=true; p.Failure=1
 if r==nil || m==nil || r.Attempted!=1 || r.Returned!=1 || r.Failure!=0 || r.Completed!=r.Extent ||
  m.Attempted!=1 || m.Returned!=1 || m.Failure!=0 || r.Metadata!=uint64(uintptr(unsafe.Pointer(m))) || r.File!=m.File || r.Process!=m.Process || r.Address!=m.Address ||
  m.Standard.EndOfFile<0 || uint64(m.Standard.EndOfFile)!=r.Extent || uint64(len(b))!=r.Extent { return false }
 n:=uint64(len(b))
 if !serviceLassoPEWithin(0,64,n) || serviceLassoPE16(b,0)!=0x5a4d { return false }
 h:=serviceLassoPE32(b,60)
 if h<64 || h>0x7fffffff || !serviceLassoPEWithin(h,24,n) || serviceLassoPE32(b,h)!=0x4550 { return false }
 p.Machine=serviceLassoPE16(b,h+4); p.Sections=serviceLassoPE16(b,h+6)
 p.Characteristics=serviceLassoPE16(b,h+22)
 p.Optional=h+24; p.OptionalBytes=uint64(serviceLassoPE16(b,h+20))
 o:=p.Optional
 if !serviceLassoPEWithin(o,p.OptionalBytes,n) || p.OptionalBytes<112 || serviceLassoPE16(b,o)!=0x20b { return false }
 p.Entry=serviceLassoPE32(b,o+16); p.ImageBase=serviceLassoPE64(b,o+24)
 p.SectionAlignment=serviceLassoPE32(b,o+32); p.FileAlignment=serviceLassoPE32(b,o+36)
 p.ImageBytes=serviceLassoPE32(b,o+56); p.HeaderBytes=serviceLassoPE32(b,o+60)
 p.Directories=serviceLassoPE32(b,o+108)
 if p.Directories>(p.OptionalBytes-112)/8 || !serviceLassoPEPower(p.SectionAlignment) || !serviceLassoPEPower(p.FileAlignment) ||
  p.SectionAlignment<p.FileAlignment || p.ImageBytes==0 || p.HeaderBytes==0 || p.HeaderBytes>n || p.HeaderBytes>p.ImageBytes ||
  p.ImageBase>^uint64(0)-p.ImageBytes || p.ImageBytes%p.SectionAlignment!=0 || p.HeaderBytes%p.FileAlignment!=0 || (p.Entry!=0 && p.Entry>=p.ImageBytes) { return false }
 // The sub-page image rule differs from ordinary page-aligned images.
 if p.SectionAlignment<4096 { if p.FileAlignment!=p.SectionAlignment { return false } } else if p.FileAlignment<512 || p.FileAlignment>65536 { return false }
 p.SectionTable=o+p.OptionalBytes
 if p.Sections==0 || !serviceLassoPEWithin(p.SectionTable,uint64(p.Sections)*40,p.HeaderBytes) { return false }
 headerEnd,ok:=serviceLassoPERound(p.HeaderBytes,p.SectionAlignment); if !ok || headerEnd>p.ImageBytes { return false }
 for i:=uint64(0); i<uint64(p.Sections); i++ {
  s:=p.section(i)
  extent:=s.VirtualBytes; if s.RawBytes>extent { extent=s.RawBytes }
  mapped,ok:=serviceLassoPERound(extent,p.SectionAlignment)
  if !ok || s.RVA%p.SectionAlignment!=0 || !serviceLassoPEWithin(s.RVA,mapped,p.ImageBytes) || (mapped!=0 && s.RVA<headerEnd) ||
   (s.RawBytes!=0 && (s.RawOffset<p.HeaderBytes || s.RawOffset%p.FileAlignment!=0 || s.RawBytes%p.FileAlignment!=0 || !serviceLassoPEWithin(s.RawOffset,s.RawBytes,n))) { return false }
  for j:=uint64(0); j<i; j++ {
   t:=p.section(j); te:=t.VirtualBytes; if t.RawBytes>te { te=t.RawBytes }; tm,tok:=serviceLassoPERound(te,p.SectionAlignment)
   if !tok || (mapped!=0 && tm!=0 && s.RVA<t.RVA+tm && t.RVA<s.RVA+mapped) ||
    (s.RawBytes!=0 && t.RawBytes!=0 && s.RawOffset<t.RawOffset+t.RawBytes && t.RawOffset<s.RawOffset+s.RawBytes) { return false }
  }
 }
 for i:=uint64(0); i<p.Directories; i++ {
  d:=o+112+i*8; start:=serviceLassoPE32(b,d); size:=serviceLassoPE32(b,d+4)
  if size==0 { continue }
  // SECURITY is a file offset, unlike every other data directory RVA.
  limit:=p.ImageBytes; if i==4 { limit=n }
  if start==0 || !serviceLassoPEWithin(start,size,limit) { return false }
 }
 p.Failure=0; p.Complete=true; return true
}

type serviceLassoPESection struct { VirtualBytes,RVA,RawBytes,RawOffset,Characteristics uint64 }
func (p *serviceLassoCreatorPE) section(i uint64) serviceLassoPESection {
 o:=p.SectionTable+i*40; b:=p.Bytes
 return serviceLassoPESection{serviceLassoPE32(b,o+8),serviceLassoPE32(b,o+12),serviceLassoPE32(b,o+16),serviceLassoPE32(b,o+20),serviceLassoPE32(b,o+36)}
}
func (p *serviceLassoCreatorPE) Section(i uint64) (serviceLassoPESection,bool) {
 if p==nil || !p.Complete || i>=uint64(p.Sections) { return serviceLassoPESection{},false }
 return p.section(i),true
}

// Kind1 is an exact readonly original file view; kind2 is a virtual zero-fill
// interval with no invented byte backing; kind0 refuses gaps/padding/crossings.
// Complete only records prior representation checks. It is never authority to
// change protection, accept a loaded mutation or perform a native operation.
func (p *serviceLassoCreatorPE) RVA(rva,size uint64) (view []byte,kind uint64) {
 if p==nil || !p.Complete || size==0 || !serviceLassoPEWithin(rva,size,p.ImageBytes) { return nil,0 }
 if rva<p.HeaderBytes {
  if !serviceLassoPEWithin(rva,size,p.HeaderBytes) { return nil,0 }
  return p.Bytes[rva:rva+size],1
 }
 for i:=uint64(0); i<uint64(p.Sections); i++ {
  s:=p.section(i)
  if rva<s.RVA { continue }
  relative:=rva-s.RVA
  if relative<s.RawBytes {
   if !serviceLassoPEWithin(relative,size,s.RawBytes) { return nil,0 }
   start:=s.RawOffset+relative
   return p.Bytes[start:start+size],1
  }
  if relative<s.VirtualBytes && serviceLassoPEWithin(relative,size,s.VirtualBytes) { return nil,2 }
 }
 return nil,0
}

type serviceLassoPERelocation struct {
 DirectoryRVA,DirectoryBytes,BlockOffset,BlockBytes,NextBlock,EntryIndex,EntryCount uint64
 PageRVA,RawEntry,Type,TargetRVA,Original,Expected,OriginalKind uint64
}

// The returned name borrows SAME immutable held bytes; terminator is checked
// inside its original representation interval, never read across a gap.
func (p *serviceLassoCreatorPE) name(rva uint64) ([]byte,bool) {
 if p==nil || !p.Complete || rva==0 {return nil,false}
 available:=uint64(0)
 if rva<p.HeaderBytes {available=p.HeaderBytes-rva} else {
  for i:=uint64(0); i<uint64(p.Sections); i++ {
   s:=p.section(i)
   if rva>=s.RVA && rva-s.RVA<s.RawBytes {available=s.RawBytes-(rva-s.RVA);break}
  }
 }
 if available==0 {return nil,false}
 b,kind:=p.RVA(rva,available); if kind!=1 {return nil,false}
 for i,c:=range b {if c==0 {if i==0 {return nil,false};return b[:i],true}}
 return nil,false
}

type serviceLassoPEImport struct {
 DescriptorIndex,ThunkIndex,OriginalFirstThunk,Timestamp,ForwarderChain,NameRVA,FirstThunk uint64
 ThunkRVA,IATRVA,RawThunk,Hint,Ordinal uint64
 Module,Name []byte
 DescriptorEnd,ThunkEnd,UsesFirstThunk bool
}

type serviceLassoPETLS struct {
 DirectoryRVA,DirectoryBytes,RawStartVA,RawEndVA,IndexVA,CallbacksVA,ZeroFill,Characteristics uint64
 TemplateRVA,TemplateBytes,IndexRVA,CallbackArrayRVA,CallbackIndex,CallbackVA,CallbackRVA uint64
 Template []byte
 CallbackEnd,CallbackInImage bool
}

type serviceLassoPEExport struct {
 DirectoryRVA,DirectoryBytes,OrdinalBase,FunctionCount,NameCount,FunctionsRVA,NamesRVA,OrdinalsRVA uint64
 FunctionIndex,NameIndex,Ordinal,FunctionRVA,NameRVA uint64
 Name,Forwarder []byte
 Named,Hole,Forwarded bool
}
func (p *serviceLassoCreatorPE) exportDirectory() (r serviceLassoPEExport,ok bool) {
 if p==nil || !p.Complete || p.Directories==0 {return r,false}
 d:=p.Optional+112;r.DirectoryRVA=serviceLassoPE32(p.Bytes,d);r.DirectoryBytes=serviceLassoPE32(p.Bytes,d+4)
 if r.DirectoryBytes<40 {return r,false}
 b,kind:=p.RVA(r.DirectoryRVA,40);if kind!=1 {return r,false}
 r.OrdinalBase=serviceLassoPE32(b,16);r.FunctionCount=serviceLassoPE32(b,20);r.NameCount=serviceLassoPE32(b,24)
 r.FunctionsRVA=serviceLassoPE32(b,28);r.NamesRVA=serviceLassoPE32(b,32);r.OrdinalsRVA=serviceLassoPE32(b,36)
 if (r.FunctionCount!=0 && r.FunctionsRVA==0) || (r.NameCount!=0 && (r.NamesRVA==0 || r.OrdinalsRVA==0)) {return r,false}
 if !serviceLassoPEWithin(r.FunctionsRVA,r.FunctionCount*4,p.ImageBytes) || !serviceLassoPEWithin(r.NamesRVA,r.NameCount*4,p.ImageBytes) ||
  !serviceLassoPEWithin(r.OrdinalsRVA,r.NameCount*2,p.ImageBytes) {return r,false}
 return r,true
}
func (p *serviceLassoCreatorPE) exportFunction(r serviceLassoPEExport,index uint64) (serviceLassoPEExport,bool) {
 r.FunctionIndex=index
 if index>=r.FunctionCount {return r,false}
 b,kind:=p.RVA(r.FunctionsRVA+index*4,4);if kind!=1 {return r,false}
 r.Ordinal=r.OrdinalBase+index;r.FunctionRVA=serviceLassoPE32(b,0)
 if r.FunctionRVA==0 {r.Hole=true;return r,true}
 if r.FunctionRVA>=r.DirectoryRVA && r.FunctionRVA-r.DirectoryRVA<r.DirectoryBytes {
  r.Forwarded=true
  var ok bool;r.Forwarder,ok=p.name(r.FunctionRVA)
  // A terminator beyond the export directory is not a forwarder string.
  if !ok || uint64(len(r.Forwarder))+1>r.DirectoryBytes-(r.FunctionRVA-r.DirectoryRVA) {return r,false}
 } else if !serviceLassoPEWithin(r.FunctionRVA,1,p.ImageBytes) {return r,false}
 return r,true
}
// Includes ordinal-only exports and explicit null holes. No address is called.
func (p *serviceLassoCreatorPE) ExportFunctionAt(index uint64) (serviceLassoPEExport,bool) {
 r,ok:=p.exportDirectory();if !ok {return r,false};return p.exportFunction(r,index)
}
func (p *serviceLassoCreatorPE) ExportNameAt(index uint64) (r serviceLassoPEExport,ok bool) {
 r,ok=p.exportDirectory();if !ok || index>=r.NameCount {return r,false}
 r.Named=true;r.NameIndex=index
 b,kind:=p.RVA(r.NamesRVA+index*4,4);if kind!=1 {return r,false}
 r.NameRVA=serviceLassoPE32(b,0);r.Name,ok=p.name(r.NameRVA);if !ok {return r,false}
 b,kind=p.RVA(r.OrdinalsRVA+index*2,2);if kind!=1 {return r,false}
 return p.exportFunction(r,uint64(serviceLassoPE16(b,0)))
}
func (p *serviceLassoCreatorPE) sourceVA(va,size uint64) (uint64,bool) {
 if va<p.ImageBase {return 0,false}
 rva:=va-p.ImageBase
 return rva,serviceLassoPEWithin(rva,size,p.ImageBytes)
}

// Original held-image VA geometry only. Loader index writes and callbacks are
// retained obligations; this never calls a callback or admits an executable.
func (p *serviceLassoCreatorPE) TLSAt(callbackIndex uint64) (r serviceLassoPETLS,ok bool) {
 r.CallbackIndex=callbackIndex
 if p==nil || !p.Complete || p.Directories<=9 {return r,false}
 d:=p.Optional+112+9*8
 r.DirectoryRVA=serviceLassoPE32(p.Bytes,d);r.DirectoryBytes=serviceLassoPE32(p.Bytes,d+4)
 if r.DirectoryBytes<40 {return r,false}
 b,kind:=p.RVA(r.DirectoryRVA,40);if kind!=1 {return r,false}
 r.RawStartVA=serviceLassoPE64(b,0);r.RawEndVA=serviceLassoPE64(b,8);r.IndexVA=serviceLassoPE64(b,16);r.CallbacksVA=serviceLassoPE64(b,24)
 r.ZeroFill=serviceLassoPE32(b,32);r.Characteristics=serviceLassoPE32(b,36)
 if r.RawStartVA!=0 || r.RawEndVA!=0 {
  if r.RawEndVA<r.RawStartVA {return r,false}
  r.TemplateBytes=r.RawEndVA-r.RawStartVA
  r.TemplateRVA,ok=p.sourceVA(r.RawStartVA,r.TemplateBytes);if !ok {return r,false}
  if r.TemplateBytes!=0 {r.Template,kind=p.RVA(r.TemplateRVA,r.TemplateBytes);if kind!=1 {return r,false}}
 }
 if r.IndexVA!=0 {
  r.IndexRVA,ok=p.sourceVA(r.IndexVA,4);if !ok {return r,false}
  _,kind=p.RVA(r.IndexRVA,4);if kind==0 {return r,false}
 }
 if r.CallbacksVA==0 {r.CallbackEnd=true;return r,callbackIndex==0}
 r.CallbackArrayRVA,ok=p.sourceVA(r.CallbacksVA,8);if !ok {return r,false}
 if callbackIndex>(p.ImageBytes-r.CallbackArrayRVA)/8 {return r,false}
 slot:=r.CallbackArrayRVA+callbackIndex*8
 b,kind=p.RVA(slot,8);if kind!=1 {return r,false}
 r.CallbackVA=serviceLassoPE64(b,0)
 if r.CallbackVA==0 {r.CallbackEnd=true;return r,true}
 r.CallbackRVA,r.CallbackInImage=p.sourceVA(r.CallbackVA,1)
 return r,true
}

// Locators and original names only. The owning traversal must consume all
// descriptor/thunk rows through their genuine retained null terminators.
// No imported pointer or parsed name acquires provider/loader authority.
func (p *serviceLassoCreatorPE) ImportAt(descriptorIndex,thunkIndex uint64) (r serviceLassoPEImport,ok bool) {
 r.DescriptorIndex=descriptorIndex;r.ThunkIndex=thunkIndex
 if p==nil || !p.Complete || p.Directories<=1 {return r,false}
 d:=p.Optional+112+8
 start:=serviceLassoPE32(p.Bytes,d); size:=serviceLassoPE32(p.Bytes,d+4)
 if size<20 || descriptorIndex>(size-20)/20 {return r,false}
 b,kind:=p.RVA(start+descriptorIndex*20,20);if kind!=1 {return r,false}
 r.OriginalFirstThunk=serviceLassoPE32(b,0);r.Timestamp=serviceLassoPE32(b,4);r.ForwarderChain=serviceLassoPE32(b,8)
 r.NameRVA=serviceLassoPE32(b,12);r.FirstThunk=serviceLassoPE32(b,16)
 if r.OriginalFirstThunk==0 && r.Timestamp==0 && r.ForwarderChain==0 && r.NameRVA==0 && r.FirstThunk==0 {
  r.DescriptorEnd=true;return r,thunkIndex==0
 }
 r.Module,ok=p.name(r.NameRVA);if !ok || r.FirstThunk==0 {return r,false}
 source:=r.OriginalFirstThunk
 if source==0 {source=r.FirstThunk;r.UsesFirstThunk=true}
 if source>=p.ImageBytes || thunkIndex>(p.ImageBytes-source)/8 {return r,false}
 r.ThunkRVA=source+thunkIndex*8
 b,kind=p.RVA(r.ThunkRVA,8);if kind!=1 {return r,false}
 r.RawThunk=serviceLassoPE64(b,0)
 if r.FirstThunk>=p.ImageBytes || thunkIndex>(p.ImageBytes-r.FirstThunk)/8 {return r,false}
 r.IATRVA=r.FirstThunk+thunkIndex*8
 if !serviceLassoPEWithin(r.IATRVA,8,p.ImageBytes) {return r,false}
 if r.RawThunk==0 {r.ThunkEnd=true;return r,true}
 if r.RawThunk>>63!=0 {
  if r.RawThunk&0x7fffffffffff0000!=0 {return r,false}
  r.Ordinal=r.RawThunk&65535;return r,true
 }
 if r.RawThunk>0xffffffff {return r,false}
 hint,kind:=p.RVA(r.RawThunk,2);if kind!=1 {return r,false}
 r.Hint=uint64(serviceLassoPE16(hint,0))
 r.Name,ok=p.name(r.RawThunk+2)
 return r,ok
}

// Readonly one-entry interpretation. The owning traversal must start block0,
// follow exact NextBlock until DirectoryBytes, consume every EntryCount row,
// and retain each result; arbitrary caller offsets do not establish traversal.
// Expected is a representation projection, never loaded-byte evidence.
func (p *serviceLassoCreatorPE) RelocationAt(blockOffset,entryIndex,loadedBase uint64) (r serviceLassoPERelocation,ok bool) {
 if p==nil || !p.Complete || p.Directories<=5 { return r,false }
 d:=p.Optional+112+5*8
 r.DirectoryRVA=serviceLassoPE32(p.Bytes,d); r.DirectoryBytes=serviceLassoPE32(p.Bytes,d+4)
 r.BlockOffset=blockOffset; r.EntryIndex=entryIndex
 if r.DirectoryBytes==0 || !serviceLassoPEWithin(blockOffset,8,r.DirectoryBytes) { return r,false }
 header,kind:=p.RVA(r.DirectoryRVA+blockOffset,8)
 if kind!=1 { return r,false }
 r.PageRVA=serviceLassoPE32(header,0); r.BlockBytes=serviceLassoPE32(header,4)
 if r.PageRVA%4096!=0 || r.BlockBytes<8 || r.BlockBytes%2!=0 || !serviceLassoPEWithin(blockOffset,r.BlockBytes,r.DirectoryBytes) { return r,false }
 r.NextBlock=blockOffset+r.BlockBytes; r.EntryCount=(r.BlockBytes-8)/2
 if r.EntryCount==0 && entryIndex==0 { return r,true }
 if entryIndex>=r.EntryCount { return r,false }
 entry,kind:=p.RVA(r.DirectoryRVA+blockOffset+8+entryIndex*2,2)
 if kind!=1 { return r,false }
 r.RawEntry=uint64(serviceLassoPE16(entry,0)); r.Type=r.RawEntry>>12
 r.TargetRVA=r.PageRVA+(r.RawEntry&4095)
 if r.Type==0 { return r,true }
 // Other architectures and relocation encodings require their own exact
 // provider/source consumer. They cannot silently become unchanged bytes.
 if p.Machine!=0x8664 || r.Type!=10 { return r,false }
 original,kind:=p.RVA(r.TargetRVA,8); r.OriginalKind=kind
 if kind==1 { r.Original=serviceLassoPE64(original,0) } else if kind!=2 { return r,false }
 r.Expected=r.Original+(loadedBase-p.ImageBase)
 return r,true
}

// Production binding chooses the actual native row and SAME original byte
// arena. It does not accept a caller-supplied replacement byte slice.
func (p *serviceLassoCreatorPE) InterpretOriginal(phase,index uint64) bool {
 var r *serviceLassoCreatorFileReadStorage
 var m *serviceLassoCreatorMetadataStorage
 var snapshot *serviceLassoCreatorModuleStorage
 if phase==0 {
  snapshot=&serviceLassoCreatorModulesBeforeOriginal
 } else if phase==1 {
  snapshot=&serviceLassoCreatorModulesAfterOriginal
 } else { return false }
 if snapshot.Returned!=1 || snapshot.Failure!=0 || snapshot.Count>uint64(len(serviceLassoCreatorFileReadsBeforeOriginal)) || index>=snapshot.Count { return false }
 if phase==0 {
  r=&serviceLassoCreatorFileReadsBeforeOriginal[index]; m=&serviceLassoCreatorMetadataBeforeOriginal[index]
 } else {
  r=&serviceLassoCreatorFileReadsAfterOriginal[index]; m=&serviceLassoCreatorMetadataAfterOriginal[index]
 }
 if r.Process!=snapshot.Process || !serviceLassoPEWithin(r.ArenaOffset,r.Extent,uint64(len(serviceLassoCreatorFileBytes))) { return false }
 return p.Interpret(r,m,serviceLassoCreatorFileBytes[r.ArenaOffset:r.ArenaOffset+r.Extent])
}
