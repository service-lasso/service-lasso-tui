//go:build windows && amd64

package windows

import "unsafe"

// Readonly initialized-Go observation, never an acquisition/entry grant.
type serviceLassoCreatorLoadedView struct {
 Walk *serviceLassoCreatorWalkStorage
 Extent *serviceLassoCreatorExtentStorage
 Regions []serviceLassoCreatorRegionStorage
 Queries []serviceLassoCreatorRegionQuery
 Reads []serviceLassoCreatorMemoryCall
 Arena []byte
 Bytes []byte
 Address,Size uint64
 Complete bool
}

func serviceLassoCreatorOriginalView(phase,index,rva,size uint64) (v serviceLassoCreatorLoadedView) {
 v.Regions=serviceLassoCreatorRegionOriginals[:]
 v.Queries=serviceLassoCreatorRegionQueries[:]
 v.Reads=serviceLassoCreatorRegionReads[:]
 v.Arena=serviceLassoCreatorMemoryBytes[:]
 if index>=uint64(len(serviceLassoCreatorWalksBeforeOriginal)) { return v }
 if phase==0 {
  v.Walk=&serviceLassoCreatorWalksBeforeOriginal[index]; v.Extent=&serviceLassoCreatorExtentsBeforeOriginal[index]
 } else if phase==1 {
  v.Walk=&serviceLassoCreatorWalksAfterOriginal[index]; v.Extent=&serviceLassoCreatorExtentsAfterOriginal[index]
 } else { return v }
 w:=v.Walk; e:=v.Extent
 if w.Attempted!=1 || w.Returned!=1 || w.Failure!=0 || w.Phase!=phase || w.ModuleIndex!=index ||
  w.ExtentOwner!=uint64(uintptr(unsafe.Pointer(e))) || e.Attempted!=1 || e.Returned!=1 || e.Failure!=0 ||
  e.Process!=w.Process || e.Address!=w.Base || e.Information.Base!=w.Base || e.ImageEnd!=w.ImageEnd ||
  w.ImageEnd<=w.Base || w.ImageEnd-w.Base!=uint64(e.Information.SizeOfImage) || w.AddressCursor!=w.ImageEnd ||
  size==0 || !serviceLassoPEWithin(rva,size,w.ImageEnd-w.Base) || w.Used==0 || !serviceLassoPEWithin(w.FirstRegion,w.Used,uint64(len(v.Regions))) { return v }
 v.Address=w.Base+rva; v.Size=size
 cursor:=w.Base
 var selected []byte
 for i:=uint64(0); i<w.Used; i++ {
  row:=w.FirstRegion+i; r:=&v.Regions[row]; q:=&v.Queries[row]; read:=&v.Reads[row]
  if r.Attempted!=1 || r.Returned!=1 || r.Failure!=0 || r.QueryIndex!=row || r.ReadIndex!=row ||
   r.Process!=w.Process || r.ModuleBase!=w.Base || r.ImageEnd!=w.ImageEnd || r.Address!=cursor ||
   r.Extent==0 || !serviceLassoPEWithin(r.Address,r.Extent,w.ImageEnd) ||
   q.Attempted!=1 || q.Returned!=1 || q.Failure!=0 || q.Process!=r.Process || q.Address!=r.Address ||
   q.ModuleBase!=r.ModuleBase || q.ImageEnd!=r.ImageEnd || q.RegionEnd<r.Address+r.Extent ||
   q.Call.Entered!=1 || q.Call.Returned!=1 || q.Call.Info.R1!=48 || q.Call.Info.Fn!=serviceLassoVirtualQueryEx || q.Call.Info.N!=4 ||
   q.Call.Info.Args!=uintptr(unsafe.Pointer(&q.Call.Arguments[0])) || q.Call.Arguments[0]!=uintptr(r.Process) || q.Call.Arguments[1]!=uintptr(r.Address) ||
   q.Call.Arguments[2]!=uintptr(unsafe.Pointer(&q.Information)) || q.Call.Arguments[3]!=48 { return v }
  info:=&q.Information
  if info.AllocationBase!=w.Base || info.BaseAddress>r.Address || info.RegionSize==0 || info.RegionSize>^uint64(0)-info.BaseAddress ||
   info.BaseAddress+info.RegionSize!=q.RegionEnd || q.RegionEnd<=r.Address { return v }
  end:=q.RegionEnd; if end>w.ImageEnd {end=w.ImageEnd}
  if r.Extent!=end-r.Address { return v }
  disposition:=uint64(3)
  if info.State==0x2000 {disposition=2} else {
   if info.State!=0x1000 || info.Type!=0x1000000 {return v}
   protect:=info.Protect&255
   if info.Protect&0x100==0 && (protect==2 || protect==4 || protect==8 || protect==0x20 || protect==0x40 || protect==0x80) {disposition=1}
  }
  if r.Disposition!=disposition {return v}
  if r.Disposition==1 {
   if read.Entered!=1 || read.Returned!=1 || uint32(read.Info.R1)==0 || read.Count!=r.Extent || read.Offset!=r.Address || read.Requested!=r.Extent ||
    read.Info.Fn!=serviceLassoReadProcessMemory || read.Info.N!=5 || read.Info.Args!=uintptr(unsafe.Pointer(&read.Arguments[0])) ||
    read.Arguments[0]!=uintptr(r.Process) || read.Arguments[1]!=uintptr(r.Address) || read.Arguments[3]!=uintptr(r.Extent) ||
    !serviceLassoPEWithin(r.ArenaOffset,r.Extent,uint64(len(v.Arena))) { return v }
   b:=v.Arena[r.ArenaOffset:r.ArenaOffset+r.Extent]
   if read.Arguments[2]!=uintptr(unsafe.Pointer(&b[0])) || read.Arguments[4]!=uintptr(unsafe.Pointer(&read.Count)) { return v }
   if v.Address>=r.Address && serviceLassoPEWithin(v.Address-r.Address,size,r.Extent) {
    off:=v.Address-r.Address; selected=b[off:off+size]
   }
  } else if r.Disposition!=2 && r.Disposition!=3 { return v }
  cursor=r.Address+r.Extent
 }
 if cursor!=w.ImageEnd || selected==nil { return v }
 v.Bytes=selected; v.Complete=true; return v
}
