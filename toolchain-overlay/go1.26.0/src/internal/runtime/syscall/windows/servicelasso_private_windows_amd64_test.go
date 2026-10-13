//go:build windows && amd64

package windows_test

import (
 "internal/runtime/syscall/windows"
 "testing"
 "unsafe"
)

// UNRUN synthetic private-constructor fixture. No genuine os/native/creator
// observation or entry/publication authority is inferred from these strings.
func TestServiceLassoPrivateConstructorKeepsActualBacking(t *testing.T) {
 command:="source fixture private constructor command"
 arguments:=[]string{"service-lasso-tui-native-payload","service-lasso-tui-native-windows-v3","18446744073709551614","0","1","1","1","--"}
 if !windows.ServiceLassoPrivateConstructorFixtureForTest(command,arguments){t.Fatal("private locator interpretation refused")}
 actualCommand,actualArguments,handle,count,failure:=windows.ServiceLassoPrivateConstructorForTest()
 if failure!=0||handle!=^uint64(0)-1||count!=0||len(actualArguments)!=8 {t.Fatal("full-width handle/zero public count")}
 if unsafe.StringData(actualCommand)!=unsafe.StringData(command)||unsafe.SliceData(actualArguments)!=unsafe.SliceData(arguments){t.Fatal("constructor originals replaced")}
 for i:=range arguments {if unsafe.StringData(actualArguments[i])!=unsafe.StringData(arguments[i])||actualArguments[i]!=arguments[i]{t.Fatal("private string original replaced")}}
 if windows.ServiceLassoPrivateConstructorFixtureForTest("foreign",[]string{"foreign"}){t.Fatal("duplicate reset")}
 afterCommand,afterArguments,afterHandle,afterCount,afterFailure:=windows.ServiceLassoPrivateConstructorForTest()
 if afterCommand!=command||unsafe.SliceData(afterArguments)!=unsafe.SliceData(arguments)||afterHandle!=handle||afterCount!=count||afterFailure!=failure {t.Fatal("duplicate changed retained original")}
}
func TestServiceLassoPrivateUnsignedBoundaries(t *testing.T) {
 cases:=[]struct{text string;maximum uint64;value uint64;valid bool}{
  {"0",0,0,true},{"1",0,0,false},{"",99,0,false},{"00",99,0,false},
  {"01",99,0,false},{"-1",99,0,false},{" 1",99,0,false},{"١",99,0,false},
  {"18446744073709551615",^uint64(0),^uint64(0),true},
  {"18446744073709551616",^uint64(0),0,false},
  {"9223372036854775808",^uint64(0)>>1,0,false},
 }
 for _,test:=range cases {
  value,ok:=windows.ServiceLassoPrivateUnsignedForTest(test.text,test.maximum)
  if ok!=test.valid||ok&&value!=test.value {t.Fatalf("text=%q got=(%d,%v)",test.text,value,ok)}
 }
}
