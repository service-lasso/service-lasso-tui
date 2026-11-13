//go:build windows && amd64

package windows_test

import (
 windows "internal/runtime/syscall/windows"
 "encoding/binary"
 "testing"
)

// Synthetic source fixture only. Actual test startup/root/native admission is
// separate; no reset or test-only bypass of the production startup is provided.
func TestCompletedDecodeBindingRetainsZeroPublicArgumentsAndRefusesMismatches(t *testing.T){
 var header [48]byte
 copy(header[:],"SLWARGV1");binary.LittleEndian.PutUint32(header[8:12],1)
 binary.LittleEndian.PutUint64(header[16:24],2)
 binary.LittleEndian.PutUint64(header[40:48],4)
 body:=[]byte{' ',0,0,0}
 for mode:=uint64(0);mode<4;mode++{
  public,ok,failure:=windows.ServiceLassoCompletedDecodeFixtureForTest(header,body,mode)
  if mode==0{
   if !ok||failure!=0||public!=nil{t.Fatalf("whitespace original should decode to nil public Args: %v %d %#v",ok,failure,public)}
  }else if ok||failure==0||public!=nil{t.Fatalf("mismatch %d escaped binding: %v %d %#v",mode,ok,failure,public)}
 }
}
