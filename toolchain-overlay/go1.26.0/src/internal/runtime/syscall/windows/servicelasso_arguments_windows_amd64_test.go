//go:build windows && amd64

package windows_test

import (
 "bytes"
 "encoding/binary"
 "internal/runtime/syscall/windows"
 "testing"
 "unsafe"
)

// UNRUN synthetic one-use fixture of the actual lower decoder. No native
// endpoint, creator, read/completion, provider or capacity is qualified here.
func TestServiceLassoFixedPublicArguments(t *testing.T) {
 // The doubled quote toggles out of quoting under the actual pre-2008 rule;
 // the following space therefore separates z without another closing quote.
 raw:=[]uint16{' ','"','x',' ','y','"',' ','"','"',' ','\\','"',' ',0xd800,' ','"','a','"','"','b',' ','z',0}
 body:=make([]byte,len(raw)*2)
 for i,value:=range raw {binary.LittleEndian.PutUint16(body[2*i:2*i+2],value)}
 var header [48]byte;copy(header[:8],"SLWARGV1")
 binary.LittleEndian.PutUint32(header[8:12],1)
 binary.LittleEndian.PutUint64(header[16:24],uint64(len(raw)))
 binary.LittleEndian.PutUint64(header[32:40],6)
 binary.LittleEndian.PutUint64(header[40:48],uint64(len(body)))
 _,ok:=windows.ServiceLassoDecodeArgumentsFixtureForTest(header,body,6)
 if !ok {t.Fatal("actual lower decoder refused source fixture")}
 public:=windows.ServiceLassoDecodedArgumentsForTest()
 expected:=[]string{"x y","","\"","\xed\xa0\x80","a\"b","z"}
 if len(public)!=len(expected)||cap(public)!=len(public){t.Fatal("public header extent")}
 savedHeader,savedBody,parsed:=windows.ServiceLassoArgumentOriginalsForTest()
 if savedHeader!=header||!bytes.Equal(savedBody,body){t.Fatal("native input original changed")}
 base:=uintptr(unsafe.Pointer(unsafe.SliceData(parsed)));end:=base+uintptr(len(parsed))
 for i,value:=range public {
  if value!=expected[i]{t.Fatalf("argument %d got=%q want=%q",i,value,expected[i])}
  if len(value)>0 {pointer:=uintptr(unsafe.Pointer(unsafe.StringData(value)));if pointer<base||pointer>end||uintptr(len(value))>end-pointer {t.Fatal("string outside SAME Parsed backing")}}
 }
 if _,again:=windows.ServiceLassoDecodeArgumentsFixtureForTest(header,body,6);again {t.Fatal("duplicate original reset")}
 after:=windows.ServiceLassoDecodedArgumentsForTest()
 if len(after)!=len(expected){t.Fatal("duplicate changed public extent")}
 for i:=range expected {if after[i]!=expected[i] {t.Fatal("duplicate changed public original")}}
 afterHeader,afterBody,_:=windows.ServiceLassoArgumentOriginalsForTest()
 if afterHeader!=header||!bytes.Equal(afterBody,body){t.Fatal("duplicate changed input original")}
}
