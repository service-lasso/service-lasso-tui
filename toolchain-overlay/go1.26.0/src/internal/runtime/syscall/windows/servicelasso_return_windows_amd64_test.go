//go:build windows && amd64

package windows_test

import (
 "internal/runtime/syscall/windows"
 "testing"
)

// UNRUN. This invokes the SAME production assembly BOOL projection, rather
// than certifying a separate Go cast. It does not qualify any native provider.
func TestServiceLassoNativeBOOLWidth(t *testing.T) {
 cases:=[]struct{raw uint64; want bool}{
  {0,false},{0x100000000,false},{0xffffffff00000000,false},
  {1,true},{0x100000001,true},{0xffffffff,true},{0xffffffffffffffff,true},
 }
 for _,test:=range cases {
  original:=test.raw
  if got:=windows.ServiceLassoNativeBOOLForTest(test.raw); got!=test.want {t.Fatalf("raw=%#x got=%v want=%v",test.raw,got,test.want)}
  if test.raw!=original {t.Fatal("raw original changed")}
 }
}
