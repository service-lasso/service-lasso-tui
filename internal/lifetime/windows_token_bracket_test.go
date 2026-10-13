package lifetime

import (
 "bytes"
 "encoding/binary"
 "testing"
)

// Synthetic interpreter cases, UNRUN; these are not native token observations.
func TestWindowsTokenBracketRetainsOriginals(t *testing.T) {
 var original [56]byte
 binary.LittleEndian.PutUint64(original[0:8],17)
 binary.LittleEndian.PutUint64(original[8:16],23)
 binary.LittleEndian.PutUint64(original[48:56],31)
 binary.LittleEndian.PutUint32(original[24:28],1)
 cases:=[]struct{name string; offset int; value uint32; want bool}{
  {"unchanged",-1,0,true},
  {"changed token",0,18,false},
  {"changed authentication",8,24,false},
  {"changed modified",48,32,false},
  {"changed type",24,2,false},
  {"primary unused impersonation",28,3,true},
  {"dynamic available is not identity",36,4096,true},
 }
 for _,test:=range cases { t.Run(test.name,func(t *testing.T){
  before:=original; after:=original
  if test.offset>=0 {binary.LittleEndian.PutUint32(after[test.offset:test.offset+4],test.value)}
  savedBefore:=before; savedAfter:=after
  if got:=interpretWindowsTokenBracket(before[:],after[:]);got!=test.want {t.Fatalf("got %v want %v",got,test.want)}
  if !bytes.Equal(before[:],savedBefore[:])||!bytes.Equal(after[:],savedAfter[:]) {t.Fatal("original changed")}
 }) }
 for _,extent:=range []int{0,55,57} {
  malformed:=make([]byte,extent)
  if interpretWindowsTokenBracket(malformed,original[:])||interpretWindowsTokenBracket(original[:],malformed) {t.Fatal("malformed statistics extent accepted")}
 }
 before:=original; after:=original
 binary.LittleEndian.PutUint32(before[24:28],2); binary.LittleEndian.PutUint32(after[24:28],2)
 if !interpretWindowsTokenBracket(before[:],after[:]) {t.Fatal("same impersonation level refused")}
 binary.LittleEndian.PutUint32(after[28:32],3)
 if interpretWindowsTokenBracket(before[:],after[:]) {t.Fatal("changed impersonation level accepted")}
 binary.LittleEndian.PutUint32(before[28:32],4); binary.LittleEndian.PutUint32(after[28:32],4)
 if interpretWindowsTokenBracket(before[:],after[:]) {t.Fatal("unknown impersonation level accepted")}
 binary.LittleEndian.PutUint32(before[24:28],0); binary.LittleEndian.PutUint32(after[24:28],0)
 if interpretWindowsTokenBracket(before[:],after[:]) {t.Fatal("unknown token type accepted")}
}
