package lifetime

import "encoding/binary"

// The caller binds these SAME immutable completed TOKEN_STATISTICS views to
// successful native calls on the SAME retained token, bracketing the actual
// access-information query. This interprets representation stability only;
// neither equal LUIDs nor ModifiedId authenticate a creator or access policy.
func interpretWindowsTokenBracket(before, after []byte) bool {
 if len(before)!=56 || len(after)!=56 { return false }
 for _,offset:=range [3]int{0,8,48} {
  if binary.LittleEndian.Uint64(before[offset:offset+8])!=binary.LittleEndian.Uint64(after[offset:offset+8]) { return false }
 }
 tokenType:=binary.LittleEndian.Uint32(before[24:28])
 if tokenType!=binary.LittleEndian.Uint32(after[24:28]) { return false }
 switch tokenType {
 case 1: // ImpersonationLevel is not meaningful for primary tokens.
  return true
 case 2:
  level:=binary.LittleEndian.Uint32(before[28:32])
  return level<=3 && level==binary.LittleEndian.Uint32(after[28:32])
 default:
  return false
 }
}
