//go:build windows && amd64

package windows

// Original delay-loader operands only. A loaded helper may write module/IAT
// and restore unload slots; each actual writer remains separately qualified.
type serviceLassoPEDelay struct {
 DescriptorIndex,ThunkIndex,Attributes uint64
 RawName,RawModule,RawIAT,RawINT,RawBound,RawUnload,Timestamp uint64
 NameRVA,ModuleRVA,IATRVA,INTRVA,BoundRVA,UnloadRVA uint64
 INTSlot,IATSlot,BoundSlot,UnloadSlot,RawThunk,Hint,Ordinal uint64
 Module,Name []byte
 DescriptorEnd,ThunkEnd bool
}

func (p *serviceLassoCreatorPE) delayAddress(raw,size,attributes uint64) (uint64,bool) {
 if raw==0 {return 0,true}
 if attributes==1 {return raw,serviceLassoPEWithin(raw,size,p.ImageBytes)}
 return p.sourceVA(raw,size)
}

func (p *serviceLassoCreatorPE) DelayImportAt(descriptorIndex,thunkIndex uint64) (r serviceLassoPEDelay,ok bool) {
 r.DescriptorIndex=descriptorIndex;r.ThunkIndex=thunkIndex
 if p==nil || !p.Complete || p.Directories<=13 {return r,false}
 d:=p.Optional+112+13*8
 start,size:=serviceLassoPE32(p.Bytes,d),serviceLassoPE32(p.Bytes,d+4)
 if size<32 || descriptorIndex>(size-32)/32 {return r,false}
 b,kind:=p.RVA(start+descriptorIndex*32,32);if kind!=1 {return r,false}
 r.Attributes=serviceLassoPE32(b,0);r.RawName=serviceLassoPE32(b,4);r.RawModule=serviceLassoPE32(b,8)
 r.RawIAT=serviceLassoPE32(b,12);r.RawINT=serviceLassoPE32(b,16);r.RawBound=serviceLassoPE32(b,20)
 r.RawUnload=serviceLassoPE32(b,24);r.Timestamp=serviceLassoPE32(b,28)
 if r.Attributes|r.RawName|r.RawModule|r.RawIAT|r.RawINT|r.RawBound|r.RawUnload|r.Timestamp==0 {
  r.DescriptorEnd=true;return r,thunkIndex==0
 }
 if r.Attributes>1 || r.RawName==0 || r.RawModule==0 || r.RawIAT==0 || r.RawINT==0 {return r,false}
 r.NameRVA,ok=p.delayAddress(r.RawName,1,r.Attributes);if !ok {return r,false}
 r.Module,ok=p.name(r.NameRVA);if !ok {return r,false}
 r.ModuleRVA,ok=p.delayAddress(r.RawModule,8,r.Attributes);if !ok {return r,false}
 // The writable module cell may be backed by a zero-fill representation.
 _,kind=p.RVA(r.ModuleRVA,8);if kind==0 {return r,false}
 r.IATRVA,ok=p.delayAddress(r.RawIAT,8,r.Attributes);if !ok {return r,false}
 r.INTRVA,ok=p.delayAddress(r.RawINT,8,r.Attributes);if !ok {return r,false}
 r.BoundRVA,ok=p.delayAddress(r.RawBound,8,r.Attributes);if !ok {return r,false}
 r.UnloadRVA,ok=p.delayAddress(r.RawUnload,8,r.Attributes);if !ok {return r,false}
 if thunkIndex>(p.ImageBytes-r.INTRVA)/8 || thunkIndex>(p.ImageBytes-r.IATRVA)/8 {return r,false}
 r.INTSlot=r.INTRVA+thunkIndex*8;r.IATSlot=r.IATRVA+thunkIndex*8
 if !serviceLassoPEWithin(r.IATSlot,8,p.ImageBytes) {return r,false}
 if r.RawBound!=0 {
  if thunkIndex>(p.ImageBytes-r.BoundRVA)/8 {return r,false}
  r.BoundSlot=r.BoundRVA+thunkIndex*8
  if !serviceLassoPEWithin(r.BoundSlot,8,p.ImageBytes) {return r,false}
 }
 if r.RawUnload!=0 {
  if thunkIndex>(p.ImageBytes-r.UnloadRVA)/8 {return r,false}
  r.UnloadSlot=r.UnloadRVA+thunkIndex*8
  if !serviceLassoPEWithin(r.UnloadSlot,8,p.ImageBytes) {return r,false}
 }
 b,kind=p.RVA(r.INTSlot,8);if kind!=1 {return r,false}
 r.RawThunk=serviceLassoPE64(b,0)
 if r.RawThunk==0 {r.ThunkEnd=true;return r,true}
 if r.RawThunk>>63!=0 {
  if r.RawThunk&0x7fffffffffff0000!=0 {return r,false}
  r.Ordinal=r.RawThunk&65535;return r,true
 }
 // Current RVA-based INT uses ordinary PE32+ hint/name RVA encoding.
 // Legacy descriptor VA conversion does not establish its helper's nested
 // name encoding; retain the raw row and refuse that unresolved projection.
 if r.Attributes!=1 {return r,false}
 if r.RawThunk>0xffffffff {return r,false}
 b,kind=p.RVA(r.RawThunk,2);if kind!=1 {return r,false}
 r.Hint=uint64(serviceLassoPE16(b,0));r.Name,ok=p.name(r.RawThunk+2)
 return r,ok
}
