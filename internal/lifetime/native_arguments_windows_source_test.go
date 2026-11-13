package lifetime

import ("testing"; "strconv"; "errors")

func TestWindowsNativeArgumentsRetainPrivateOnlyLocators(t *testing.T) {
 raw:=[]string{WindowsNativeArgumentZero,WindowsNativeArgumentVersion,"42","3","128","4","256","--"}
 var o WindowsNativeArguments;o.Raw=raw
 r:=NewReceiver();if err:=r.ObserveWindowsNativeArguments(&o);err!=nil{t.Fatal(err)}
 if o.PublicCount!=3 || o.PublicZero!="" || o.PublicTail!=nil || o.Handle!=42 {t.Fatal("locator became public argument authority")}
 if !o.Returned || o.Failure!=nil || &o.Raw[0]!=&raw[0] {t.Fatal("missing original private occurrence")}
}
func TestWindowsNativeArgumentsRejectPublicSuffix(t *testing.T) {
 raw:=[]string{WindowsNativeArgumentZero,WindowsNativeArgumentVersion,"42","1","128","4","256","--","public"}
 var o WindowsNativeArguments;o.Raw=raw
 if err:=NewReceiver().ObserveWindowsNativeArguments(&o);err!=errNativeArguments || !o.Returned || o.PublicTail!=nil {t.Fatal("legacy suffix accepted")}
}
func TestWindowsNativeArgumentsZeroCountDoesNotIndexPublicZero(t *testing.T) {
 raw:=[]string{WindowsNativeArgumentZero,WindowsNativeArgumentVersion,"42","0","128","4","256","--"}
 var o WindowsNativeArguments;o.Raw=raw
 if err:=NewReceiver().ObserveWindowsNativeArguments(&o);err!=nil{t.Fatal(err)}
 if o.PublicCount!=0 || o.PublicZero!="" || len(o.PublicTail)!=0 || cap(o.PublicTail)!=0 {t.Fatal("whitespace-only branch acquired invented public arguments")}
}
func TestWindowsNativeArgumentsRejectInvalidLocators(t *testing.T) {
 for _,value:=range []string{"","0","-1","+42","042","18446744073709551616","18446744073709551615"} {
  raw:=[]string{WindowsNativeArgumentZero,WindowsNativeArgumentVersion,value,"1","128","4","256","--"}
  var o WindowsNativeArguments;o.Raw=raw;r:=NewReceiver()
  if err:=r.ObserveWindowsNativeArguments(&o);err!=errNativeArguments || !o.Returned || o.Failure!=err {t.Fatalf("invalid handle %q did not retain failure",value)}
  if o.Handle!=0 || o.PublicTail!=nil {t.Fatal("failed framing published usable aliases")}
 }
}
func TestWindowsNativeArgumentsRejectStaleOutputWithoutMutation(t *testing.T) {
 raw:=[]string{WindowsNativeArgumentZero,WindowsNativeArgumentVersion,"42","0","128","4","256","--"}
 prior:=errors.New("retained earlier failure")
 cases:=[]WindowsNativeArguments{{Raw:raw,PublicZero:"old argv0"},{Raw:raw,Handle:77},{Raw:raw,Failure:prior},{Raw:raw,PublicTail:[]string{"old tail"}}}
 for i:=range cases {
  o:=&cases[i];zero,handle,failure,tail:=o.PublicZero,o.Handle,o.Failure,o.PublicTail
  if err:=NewReceiver().ObserveWindowsNativeArguments(o);err!=errOriginalEnrollment {t.Fatal("stale output accepted")}
  if o.Attempted || o.Returned || o.PublicZero!=zero || o.Handle!=handle || o.Failure!=failure || len(o.PublicTail)!=len(tail) {t.Fatal("prior occurrence overwritten")}
 }
}
func TestWindowsNativeHandleFullWidthBoundary(t *testing.T) {
 maximum:=^uintptr(0);text:=strconv.FormatUint(uint64(maximum-1),10)
 value,ok:=canonicalWindowsHandle(text);if !ok || value!=maximum-1 {t.Fatal("full-width max-1 narrowed or denied")}
}