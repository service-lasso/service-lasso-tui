package lifetime

import (
 "bytes"
 "encoding/binary"
 "testing"
 "unsafe"
)

// Synthetic, UNRUN fixtures of the selected constructor's exact one-ACE form.
// They are not actual GetKernelObjectSecurity/TokenUser/provider evidence.
// This serial fixture root forces native-pointer bytes to refer to stable heap
// backing rather than a movable Go stack array. No test uses t.Parallel.
var endpointACLFixtureUserOriginal []byte

func endpointACLFixture() ([]byte, []byte) {
 sid:=[]byte{1,1,0,0,0,0,0,5,21,0,0,0}
 descriptor:=make([]byte,60)
 descriptor[0]=1
 binary.LittleEndian.PutUint16(descriptor[2:4],0x9004)
 binary.LittleEndian.PutUint32(descriptor[4:8],20)
 binary.LittleEndian.PutUint32(descriptor[8:12],20)
 binary.LittleEndian.PutUint32(descriptor[16:20],32)
 copy(descriptor[20:32],sid)
 descriptor[32]=2
 binary.LittleEndian.PutUint16(descriptor[34:36],28)
 binary.LittleEndian.PutUint16(descriptor[36:38],1)
 binary.LittleEndian.PutUint16(descriptor[42:44],20)
 binary.LittleEndian.PutUint32(descriptor[44:48],windowsEndpointAllowedMask)
 copy(descriptor[48:60],sid)
 user:=make([]byte,28)
 endpointACLFixtureUserOriginal=user
 copy(user[16:],sid)
 binary.LittleEndian.PutUint64(user[:8],uint64(uintptr(unsafe.Pointer(unsafe.SliceData(user))))+16)
 return descriptor,user
}

func TestEndpointACLSelectedShapeReadOnly(t *testing.T) {
 descriptor,user:=endpointACLFixture()
 beforeDescriptor,beforeUser:=bytes.Clone(descriptor),bytes.Clone(user)
 if !interpretWindowsEndpointACL(descriptor,len(descriptor),user,len(user)) { t.Fatal("selected source shape rejected") }
 if !bytes.Equal(descriptor,beforeDescriptor)||!bytes.Equal(user,beforeUser) { t.Fatal("original views changed") }
 // Capacity beyond native completed extent is not additional policy bytes.
 descriptor=append(descriptor,0xff,0xff,0xff,0xff)
 if !interpretWindowsEndpointACL(descriptor,60,user,len(user)) { t.Fatal("unused capacity interpreted") }
}

func TestEndpointACLRejectsMalformedOriginalViews(t *testing.T) {
 cases:=[]struct{name string; mutate func([]byte,[]byte); descriptorExtent,userExtent int}{
  {"short descriptor",nil,19,28},
  {"negative descriptor",nil,-1,28},
  {"foreign descriptor extent",nil,61,28},
  {"short user",nil,60,15},
  {"foreign user extent",nil,60,29},
  {"revision",func(d,u []byte){d[0]=2},60,28},
  {"unprotected",func(d,u []byte){binary.LittleEndian.PutUint16(d[2:4],0x8004)},60,28},
  {"null DACL",func(d,u []byte){binary.LittleEndian.PutUint32(d[16:20],0)},60,28},
  {"owner in header",func(d,u []byte){binary.LittleEndian.PutUint32(d[4:8],16)},60,28},
  {"group incomplete",func(d,u []byte){binary.LittleEndian.PutUint32(d[8:12],56)},60,28},
  {"oversized ACL",func(d,u []byte){binary.LittleEndian.PutUint16(d[34:36],29)},60,28},
  {"extra ACE",func(d,u []byte){binary.LittleEndian.PutUint16(d[36:38],2)},60,28},
  {"conditional ACE",func(d,u []byte){d[40]=9},60,28},
  {"inheritance",func(d,u []byte){d[41]=1},60,28},
  {"ACE trailing byte",func(d,u []byte){binary.LittleEndian.PutUint16(d[42:44],19)},60,28},
  {"broader mask",func(d,u []byte){binary.LittleEndian.PutUint32(d[44:48],windowsEndpointAllowedMask|0x40000)},60,28},
  {"SID count",func(d,u []byte){d[49]=16},60,28},
  {"SID mismatch",func(d,u []byte){u[24]^=1},60,28},
  {"pointer in header",func(d,u []byte){binary.LittleEndian.PutUint64(u[:8],uint64(uintptr(unsafe.Pointer(unsafe.SliceData(u))))+8)},60,28},
  {"pointer misaligned",func(d,u []byte){binary.LittleEndian.PutUint64(u[:8],uint64(uintptr(unsafe.Pointer(unsafe.SliceData(u))))+17)},60,28},
  {"pointer beyond original",func(d,u []byte){binary.LittleEndian.PutUint64(u[:8],uint64(uintptr(unsafe.Pointer(unsafe.SliceData(u))))+uint64(len(u)))},60,28},
 }
 for _,item:=range cases { t.Run(item.name,func(t *testing.T){
  descriptor,user:=endpointACLFixture()
  if item.mutate!=nil { item.mutate(descriptor,user) }
  beforeDescriptor,beforeUser:=bytes.Clone(descriptor),bytes.Clone(user)
  if interpretWindowsEndpointACL(descriptor,item.descriptorExtent,user,item.userExtent) { t.Fatal("malformed view accepted") }
  if !bytes.Equal(descriptor,beforeDescriptor)||!bytes.Equal(user,beforeUser) { t.Fatal("refusal changed original views") }
 }) }
}
