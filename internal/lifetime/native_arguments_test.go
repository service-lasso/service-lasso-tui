package lifetime
import ("testing";"strconv")
// UNRUN: framing observes actual aliases/quantities only, never endpoint authority.
func TestNativeArgumentsPreservePublicArguments(t *testing.T){
 raw:=[]string{"/installed/service-lasso-tui",NativeArgumentVersion,"19","4","256","3","64","--","--api","https://runtime.example","--compat-v1"}
 receiver:=NewReceiver();o:=&NativeArguments{Raw:raw}
 if err:=receiver.ObserveNativeArguments(o);err!=nil{t.Fatal(err)}
 if o.Descriptor!=19||o.PublicCount!=4||len(o.Public)!=4||o.Public[0]!=raw[0]||o.Public[1]!=raw[8]||o.Public[3]!=raw[10]||o.SecurityLabelBytes!=256||o.TerminalCalls!=3||o.RepresentationBytes!=64{t.Fatal("public argv or native quantity locators changed")}
 if &o.Raw[0]!=&raw[0]||o.Original.Value!=o{t.Fatal("actual private argv not retained")};receiver.Stop();receiver.Join()
}
func TestNativeArgumentsRejectAmbiguousPrefix(t *testing.T){
 valid:=[]string{"public",NativeArgumentVersion,"19","1","256","3","64","--"}
 cases:=[][]string{{"public","--help"},{"public","service-lasso-tui-native-v1","19","1","--"}}
 for _,change:=range []struct{Index int;Value string}{{1,"other"},{2,"019"},{2,"2"},{3,"01"},{3,"2"},{4,"0"},{4,"4294967296"},{5,"03"},{5,"18446744073709551615"},{6,"0"},{6,"18446744073709551615"},{7,"-"}}{
  raw:=append([]string(nil),valid...);raw[change.Index]=change.Value;cases=append(cases,raw)
 }
 for _,index:=range []int{4,5,6}{for _,value:=range []string{"","-1","+1","1a"," 1","00"}{raw:=append([]string(nil),valid...);raw[index]=value;cases=append(cases,raw)}}
 for _,raw:=range cases{receiver:=NewReceiver();o:=&NativeArguments{Raw:raw};if receiver.ObserveNativeArguments(o)==nil||o.Public!=nil{t.Fatal("ambiguous framing admitted")};receiver.Stop();receiver.Join()}
}
func TestNativeNumberActualTargetIntBoundary(t *testing.T){
 maximum:=int(^uint(0)>>1);text:=strconv.FormatUint(uint64(maximum),10)
 if value,ok:=canonicalNativeNumber(text);!ok||value!=maximum{t.Fatal("actual target int maximum rejected or narrowed")}
 overflow:=strconv.FormatUint(uint64(maximum)+1,10)
 if _,ok:=canonicalNativeNumber(overflow);ok{t.Fatal("actual target int overflow accepted")}
 for _,text:=range []string{"","-1","+1","00","1a"," 1"}{if _,ok:=canonicalNativeNumber(text);ok{t.Fatal("noncanonical number accepted")}}
}
func TestNativeArgumentSpansPreserveOriginalBackingWithoutProjection(t *testing.T){
 raw:=[]string{"public",NativeArgumentVersion,"19","3","1","1","1","--","--api","endpoint"}
 receiver:=NewReceiver();o:=&NativeArguments{Raw:raw}
 if err:=receiver.observeNativeArguments(o,false);err!=nil{t.Fatal(err)}
 if o.Public!=nil||o.PublicZero!=raw[0]||len(o.PublicTail)!=2||cap(o.PublicTail)!=2||&o.PublicTail[0]!=&raw[8]||o.PublicTail[1]!=raw[9]{t.Fatal("native argument span projected or changed")}
 if raw[7]!="--"||o.Original.Value!=o{t.Fatal("original framing/root changed")}
}
// UNRUN paired production receiver: empty transport name never becomes public argv0.
func TestNativeArgumentZeroCountTransport(t *testing.T){
 raw:=[]string{"",NativeArgumentVersion,"19","0","1","1","1","--"}
 for _,project:=range []bool{false,true}{
  r:=NewReceiver();o:=&NativeArguments{Raw:raw}
  if err:=r.observeNativeArguments(o,project);err!=nil{t.Fatal(err)}
  if o.PublicCount!=0||len(o.Public)!=0||o.PublicZero!=""||o.PublicTail!=nil||&o.Raw[0]!=&raw[0]{t.Fatal("zero public arguments changed")}
 }
 for _,bad:=range [][]string{{"invented",NativeArgumentVersion,"19","0","1","1","1","--"},{"",NativeArgumentVersion,"19","0","1","1","1","--","tail"},{"",NativeArgumentVersion,"19","00","1","1","1","--"}}{
  o:=&NativeArguments{Raw:bad};if NewReceiver().ObserveNativeArguments(o)==nil||o.Public!=nil{t.Fatal("ambiguous zero framing admitted")}
 }
}