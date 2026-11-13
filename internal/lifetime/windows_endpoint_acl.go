package lifetime

import (
 "encoding/binary"
 "unsafe"
)

// The native constructor creates exactly one noninheriting ACCESS_ALLOWED_ACE
// for its actual TokenUser SID. This is a representation interpreter only, not
// process/entry authentication or general conditional-ACE policy evaluation.
const windowsEndpointAllowedMask uint32 = 0x0012018f

func windowsSIDExtent(bytes []byte, offset int) (int, bool) {
 if offset < 0 || offset > len(bytes) || len(bytes)-offset < 8 { return 0, false }
 if bytes[offset] != 1 || bytes[offset+1] > 15 { return 0, false }
 extent := 8+int(bytes[offset+1])*4
 return extent, extent <= len(bytes)-offset
}

// Both slices must be the candidate constructor's SAME immutable native output
// views. The caller separately joins actual successful calls/subjects/lengths,
// source/provider/own-writer lifetimes and containing reservation originals.
// No pointer supplied by native output is dereferenced: checked offsets select
// only bytes inside the actual original User buffer.
func interpretWindowsEndpointACL(descriptor []byte, descriptorExtent int, user []byte, userExtent int) bool {
 if descriptorExtent < 20 || descriptorExtent > len(descriptor) || userExtent < 16 || userExtent > 84 || userExtent > len(user) { return false }
 descriptor=descriptor[:descriptorExtent]; user=user[:userExtent]
 if descriptor[0]!=1 || descriptor[1]!=0 { return false }
 control:=binary.LittleEndian.Uint16(descriptor[2:4])
 if control&0x9004 != 0x9004 { return false }
 // Owner/group views are independently range checked; their principal/access
 // significance is part of the complete policy join, not inferred from DACL.
 for _, position := range [2]int{4,8} {
  offset:=uint64(binary.LittleEndian.Uint32(descriptor[position:position+4]))
  if offset<20 || offset%4!=0 || offset>uint64(len(descriptor)) { return false }
  if _,ok:=windowsSIDExtent(descriptor,int(offset)); !ok { return false }
 }
 dacl:=uint64(binary.LittleEndian.Uint32(descriptor[16:20]))
 if dacl<20 || dacl%4!=0 || dacl>uint64(len(descriptor)) || uint64(len(descriptor))-dacl<8 { return false }
 acl:=descriptor[int(dacl):]
 aclExtent:=int(binary.LittleEndian.Uint16(acl[2:4]))
 if acl[0]!=2 || acl[1]!=0 || acl[6]!=0 || acl[7]!=0 || aclExtent<16 || aclExtent>len(acl) || binary.LittleEndian.Uint16(acl[4:6])!=1 { return false }
 acl=acl[:aclExtent]; ace:=acl[8:]
 aceExtent:=int(binary.LittleEndian.Uint16(ace[2:4]))
 if ace[0]!=0 || ace[1]!=0 || aceExtent!=len(ace) || aceExtent<16 || binary.LittleEndian.Uint32(ace[4:8])!=windowsEndpointAllowedMask { return false }
 sidExtent,ok:=windowsSIDExtent(ace,8)
 if !ok || sidExtent!=aceExtent-8 { return false }
 nativeSID:=binary.LittleEndian.Uint64(user[:8])
 base:=uint64(uintptr(unsafe.Pointer(unsafe.SliceData(user))))
 if base>^uint64(0)-uint64(len(user)) || nativeSID<base || nativeSID>base+uint64(len(user)) { return false }
 offset:=nativeSID-base
 if offset<16 || offset%4!=0 { return false }
 userSIDExtent,ok:=windowsSIDExtent(user,int(offset))
 if !ok || userSIDExtent!=sidExtent { return false }
 for index:=0; index<sidExtent; index++ { if user[int(offset)+index]!=ace[8+index] { return false } }
 return true
}
