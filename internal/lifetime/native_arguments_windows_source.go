package lifetime

const WindowsNativeArgumentVersion = "service-lasso-tui-native-windows-v3"
const WindowsNativeArgumentZero = "service-lasso-tui-native-payload"

// WindowsNativeArguments retains the actual runtime argv. Framing is a locator,
// never endpoint, loaded-code or reservation authority. This phase does not
// publish public arguments or allocate a reconstructed argument roster.
type WindowsNativeArguments struct {
 Original Original
 Raw []string
 PublicZero string
 PublicTail []string
 Handle uintptr
 PublicCount int
 SecurityLabelBytes, TerminalCalls, RepresentationBytes int
 HandleText, CountText, SecurityText, TerminalText, RepresentationText string
 Attempted, Returned bool
 Failure error
}
func canonicalWindowsHandle(text string) (uintptr, bool) {
 if len(text)==0 || len(text)>1 && text[0]=='0' { return 0,false }
 value:=uintptr(0);maximum:=^uintptr(0)
 for i:=0;i<len(text);i++ { digit:=text[i]-'0';if digit>9 || value>(maximum-uintptr(digit))/10 {return 0,false};value=value*10+uintptr(digit) }
 // NULL and INVALID_HANDLE_VALUE are not usable endpoint locators.
 return value,value!=0 && value!=maximum
}
func(r *Receiver)ObserveWindowsNativeArguments(o *WindowsNativeArguments)error {
 if r==nil || o==nil { return errOriginalEnrollment }
 // The candidate constructor owns fresh zeroed output storage. Reject stale
 // output before enrollment or mutation; never clear an earlier occurrence.
 if o.Attempted || o.Returned || o.Failure!=nil || o.Handle!=0 || o.PublicCount!=0 || o.PublicZero!="" || o.PublicTail!=nil || o.SecurityLabelBytes!=0 || o.TerminalCalls!=0 || o.RepresentationBytes!=0 || o.HandleText!="" || o.CountText!="" || o.SecurityText!="" || o.TerminalText!="" || o.RepresentationText!="" {return errOriginalEnrollment}
 if err:=r.Enroll(&o.Original,"native-payload.windows-private-argv",o);err!=nil{return err}
 if o.Attempted{return errOriginalEnrollment};o.Attempted=true
 fail:=func()error{o.Failure=errNativeArguments;o.Returned=true;return o.Failure}
 if len(o.Raw)!=8 || o.Raw[0]!=WindowsNativeArgumentZero || o.Raw[1]!=WindowsNativeArgumentVersion || o.Raw[7]!="--" {return fail()}
 o.HandleText=o.Raw[2];o.CountText=o.Raw[3];o.SecurityText=o.Raw[4];o.TerminalText=o.Raw[5];o.RepresentationText=o.Raw[6]
 handle,ok:=canonicalWindowsHandle(o.HandleText);if !ok{return fail()}
 count,ok:=canonicalNativeNumber(o.CountText);if !ok{return fail()}
 security,ok:=canonicalNativeNumber(o.SecurityText);if !ok || security<1 || uint64(security)>uint64(^uint32(0)){return fail()}
 terminal,ok:=canonicalNativeNumber(o.TerminalText);if !ok || terminal<1{return fail()}
 representation,ok:=canonicalNativeNumber(o.RepresentationText);if !ok || representation<1{return fail()}
 o.Handle=handle;o.PublicCount=count;o.SecurityLabelBytes=security;o.TerminalCalls=terminal;o.RepresentationBytes=representation
 // PublicCount is only a comparison locator. The original public spans are
 // published by the separately owned completed UTF16 transport, never argv.
 o.Returned=true;return nil
}