//go:build windows && amd64

package windows

// Ordinary typed Go GC roots retain the ACTUAL os.init constructor outputs.
// Native/pre-Go-G/TLS assembly never writes Go string/slice headers here.
// Private parsing is a locator interpretation, not creator or entry authority.
type serviceLassoPrivateConstructorStorage struct {
 Command string
 Arguments []string
 Endpoint uint64
 PublicCount uint64
 SecurityBytes uint64
 TerminalCalls uint64
 RepresentationBytes uint64
 Failure uint64
 Attempted bool
 Returned bool
}
var serviceLassoPrivateConstructorOriginal serviceLassoPrivateConstructorStorage

func serviceLassoPrivateUnsigned(text string,maximum uint64)(uint64,bool) {
 if len(text)==0||len(text)>1&&text[0]=='0'{return 0,false}
 value:=uint64(0)
 for i:=0;i<len(text);i++ {
  digit:=text[i]-'0'
  if digit>9||uint64(digit)>maximum||value>(maximum-uint64(digit))/10{return 0,false}
  value=value*10+uint64(digit)
 }
 return value,true
}

// Called at the source-owned os.init seam only AFTER its genuine original
// GetCommandLine/UTF16-WTF8/parser/fallback constructor. Retain actual private
// allocations before interpreting, including failure. No second constructor,
// rebuilt argument roster, copied strings or bootstrap IO is performed here.
func ServiceLassoCapturePrivateConstructor(command string,arguments []string) bool {
 return serviceLassoCapturePrivateConstructor(&serviceLassoPrivateConstructorOriginal,command,arguments)
}
func serviceLassoCapturePrivateConstructor(o *serviceLassoPrivateConstructorStorage,command string,arguments []string) bool {
 if o==nil{return false}
 if o.Attempted||o.Returned||o.Command!=""||o.Arguments!=nil||o.Failure!=0||o.Endpoint!=0||o.PublicCount!=0||o.SecurityBytes!=0||o.TerminalCalls!=0||o.RepresentationBytes!=0{return false}
 o.Attempted=true;o.Command=command;o.Arguments=arguments
 if !serviceLassoInterpretPrivateConstructor(o){o.Failure=1;o.Returned=true;return false}
 o.Returned=true;return true
}
func serviceLassoInterpretPrivateConstructor(o *serviceLassoPrivateConstructorStorage) bool {
 a:=o.Arguments
 if len(a)!=8||a[0]!="service-lasso-tui-native-payload"||a[1]!="service-lasso-tui-native-windows-v3"||a[7]!="--"{return false}
 endpoint,ok:=serviceLassoPrivateUnsigned(a[2],^uint64(0));if !ok||endpoint==0||endpoint==^uint64(0){return false};o.Endpoint=endpoint
 count,ok:=serviceLassoPrivateUnsigned(a[3],uint64(^uint64(0)>>1));if !ok{return false};o.PublicCount=count
 security,ok:=serviceLassoPrivateUnsigned(a[4],uint64(^uint32(0)));if !ok||security==0{return false};o.SecurityBytes=security
 terminal,ok:=serviceLassoPrivateUnsigned(a[5],uint64(^uint64(0)>>1));if !ok||terminal==0{return false};o.TerminalCalls=terminal
 representation,ok:=serviceLassoPrivateUnsigned(a[6],uint64(^uint64(0)>>1));if !ok||representation==0{return false};o.RepresentationBytes=representation
 return true
}
