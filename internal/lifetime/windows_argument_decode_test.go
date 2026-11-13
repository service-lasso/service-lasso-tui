package lifetime

import ("encoding/binary";"testing";"strconv")

type windowsDecodeFixture struct {
 receiver Receiver
 root windowsArgumentDecode
 locator WindowsNativeArguments
 body [256]byte
 utf8 [256]byte
 parsed [256]byte
 public [32]string
}
func(f *windowsDecodeFixture)prepare(raw,module []uint16,count int,fallback bool){
 o:=&f.root;o.Body=f.body[:];o.UTF8=f.utf8[:];o.Parsed=f.parsed[:];o.Public=f.public[:];o.Locator=&f.locator
 f.locator.Raw=[]string{WindowsNativeArgumentZero,WindowsNativeArgumentVersion,"42",strconv.Itoa(count),"128","4","256","--"}
 if err:=f.receiver.ObserveWindowsNativeArguments(&f.locator);err!=nil{panic(err)}
 copy(o.Header[:8],"SLWARGV1");binary.LittleEndian.PutUint32(o.Header[8:12],1)
 if fallback{binary.LittleEndian.PutUint32(o.Header[12:16],1)}
 binary.LittleEndian.PutUint64(o.Header[16:24],uint64(len(raw)));binary.LittleEndian.PutUint64(o.Header[24:32],uint64(len(module)))
 binary.LittleEndian.PutUint64(o.Header[32:40],uint64(count));o.BodyExtent=2*(len(raw)+len(module));binary.LittleEndian.PutUint64(o.Header[40:48],uint64(o.BodyExtent))
 at:=0;for _,span:=range [][]uint16{raw,module}{for _,u:=range span{binary.LittleEndian.PutUint16(f.body[at:at+2],u);at+=2}}
}
func TestWindowsArgumentInterpretationUsesOriginalWTF8ThenQuoteRules(t *testing.T){
 var f windowsDecodeFixture
 // The quote separates surrogate code units during WTF8 conversion. Parsing
 // later removes that quote; it must not retroactively create a scalar pair.
 f.prepare([]uint16{'p',' ','"',0xd800,'"',0xdc00,0},nil,2,false)
 if err:=f.receiver.interpretWindowsArguments(&f.root);err!=nil{t.Fatal(err)}
 if f.root.count!=2||f.public[0]!="p"||f.public[1]!="\xed\xa0\x80\xed\xb0\x80"{t.Fatal("WTF8/quote order changed")}
 original:=f.root.parsedUsed;if err:=f.receiver.interpretWindowsArguments(&f.root);err!=errOriginalEnrollment||f.root.parsedUsed!=original{t.Fatal("original interpretation retried")}
}
func TestWindowsArgumentInterpretationPairAndSlashQuote(t *testing.T){
 var f windowsDecodeFixture
 f.prepare([]uint16{0xd83d,0xde00,' ','"','a','\\','\\','"',' ','b',0},nil,3,false)
 if err:=f.receiver.interpretWindowsArguments(&f.root);err!=nil{t.Fatal(err)}
 if f.public[0]!="\xf0\x9f\x98\x80"||f.public[1]!="a\\"||f.public[2]!="b"{t.Fatal("pair or trailing slash rule changed")}
}
func TestWindowsArgumentInterpretationEmptyFallbackAndWhitespaceDiffer(t *testing.T){
 var empty,spaces windowsDecodeFixture
 empty.prepare([]uint16{0},[]uint16{0},1,true);spaces.prepare([]uint16{' ','\t',0},nil,0,false)
 if err:=empty.receiver.interpretWindowsArguments(&empty.root);err!=nil{t.Fatal(err)}
 if err:=spaces.receiver.interpretWindowsArguments(&spaces.root);err!=nil{t.Fatal(err)}
 if empty.root.count!=1||empty.public[0]!=""||spaces.root.count!=0{t.Fatal("original zero/empty domain collapsed")}
}
func TestWindowsArgumentInterpretationRetainsIndependentFailures(t *testing.T){
 var shortage,incomplete,embedded windowsDecodeFixture
 shortage.prepare([]uint16{0xd800,0},nil,1,false);shortage.root.UTF8=shortage.utf8[:1:1]
 incomplete.prepare([]uint16{'p',0},nil,1,false);incomplete.root.BodyExtent--
 embedded.prepare([]uint16{'p',0,'q',0},nil,1,false)
 for _,f:=range []*windowsDecodeFixture{&shortage,&incomplete,&embedded}{
  before:=f.body;if err:=f.receiver.interpretWindowsArguments(&f.root);err!=errWindowsArgumentDecode||!f.root.returned||f.root.failure!=err{t.Fatal("failed original not retained")}
  if f.body!=before||f.root.count!=0{t.Fatal("source mutated or public strings published")}
 }
 if !shortage.root.exhausted||incomplete.root.utf8Used!=0||embedded.root.utf8Used!=0{t.Fatal("capacity and framing failures conflated")}
}
func TestWindowsArgumentInterpretationRequiresSameObservedLocator(t *testing.T){
 var f windowsDecodeFixture;f.prepare([]uint16{'p',0},nil,1,false)
 before:=f.body
 if err:=NewReceiver().interpretWindowsArguments(&f.root);err!=errOriginalEnrollment||f.root.attempted||f.root.returned||f.body!=before||f.root.parsedUsed!=0{t.Fatal("foreign locator owner consumed")}
 if err:=f.receiver.interpretWindowsArguments(&f.root);err!=nil||f.root.count!=1||f.public[0]!="p"{t.Fatal("actual original owner denied after foreign no-effect call")}
}
