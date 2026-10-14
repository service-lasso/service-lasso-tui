//go:build windows && amd64

package windows

import "unsafe"

// Independent synthetic occurrence, never a production state reset. No native
// call or provider/entry authority follows from these constructed observations.
func ServiceLassoCreatorPEFixtureForTest(b []byte) (bool,uint64) {
 p,ok:=serviceLassoCreatorPEFixture(b)
 return ok,p.Failure
}
func serviceLassoCreatorPEFixture(b []byte) (*serviceLassoCreatorPE,bool) {
 m:=&serviceLassoCreatorMetadataStorage{File:7,Process:9,Address:11,Attempted:1,Returned:1}
 m.Standard.EndOfFile=int64(len(b))
 r:=&serviceLassoCreatorFileReadStorage{File:7,Process:9,Address:11,Metadata:uint64(uintptr(unsafe.Pointer(m))),Extent:uint64(len(b)),Completed:uint64(len(b)),Attempted:1,Returned:1}
 p:=&serviceLassoCreatorPE{}
 ok:=p.Interpret(r,m,b)
 return p,ok
}
func ServiceLassoCreatorPEViewFixtureForTest(b []byte,rva,size uint64) ([]byte,uint64) {
 p,ok:=serviceLassoCreatorPEFixture(b)
 if !ok { return nil,0 }
 return p.RVA(rva,size)
}
func ServiceLassoCreatorPERelocationFixtureForTest(b []byte,block,index,base uint64) (bool,uint64,uint64,uint64,uint64) {
 p,ok:=serviceLassoCreatorPEFixture(b)
 if !ok { return false,0,0,0,0 }
 r,ok:=p.RelocationAt(block,index,base)
 return ok,r.Type,r.TargetRVA,r.Original,r.Expected
}
func ServiceLassoCreatorPEImportFixtureForTest(b []byte,descriptor,index uint64) (bool,bool,bool,uint64,[]byte,[]byte) {
 p,ok:=serviceLassoCreatorPEFixture(b)
 if !ok {return false,false,false,0,nil,nil}
 r,ok:=p.ImportAt(descriptor,index)
 return ok,r.DescriptorEnd,r.ThunkEnd,r.Ordinal,r.Module,r.Name
}
func ServiceLassoCreatorPEImportIATFixtureForTest(b []byte,descriptor,index uint64) (bool,bool,uint64) {
 p,ok:=serviceLassoCreatorPEFixture(b);if !ok {return false,false,0}
 r,ok:=p.ImportAt(descriptor,index);return ok,r.ThunkEnd,r.IATRVA
}
func ServiceLassoCreatorPEDelayFixtureForTest(b []byte,descriptor,index uint64) (bool,bool,bool,uint64,uint64,[]byte,[]byte) {
 p,ok:=serviceLassoCreatorPEFixture(b);if !ok {return false,false,false,0,0,nil,nil}
 r,ok:=p.DelayImportAt(descriptor,index)
 return ok,r.DescriptorEnd,r.ThunkEnd,r.IATSlot,r.BoundSlot,r.Module,r.Name
}
func ServiceLassoCreatorPETLSFixtureForTest(b []byte,index uint64) (bool,bool,bool,uint64,uint64) {
 p,ok:=serviceLassoCreatorPEFixture(b);if !ok {return false,false,false,0,0}
 r,ok:=p.TLSAt(index);return ok,r.CallbackEnd,r.CallbackInImage,r.CallbackVA,r.IndexRVA
}
func ServiceLassoCreatorPEExportFixtureForTest(b []byte,index uint64,named bool) (bool,bool,bool,uint64,[]byte,[]byte) {
 p,ok:=serviceLassoCreatorPEFixture(b);if !ok {return false,false,false,0,nil,nil}
 var r serviceLassoPEExport
 if named {r,ok=p.ExportNameAt(index)} else {r,ok=p.ExportFunctionAt(index)}
 return ok,r.Hole,r.Forwarded,r.Ordinal,r.Name,r.Forwarder
}

// No testing import here: testing -> runtime -> this lower package otherwise
// creates an import cycle. Export only to the separately compiled test package.
func serviceLassoNativeBOOLForTest(raw uint64) bool
var ServiceLassoNativeBOOLForTest = serviceLassoNativeBOOLForTest

// Synthetic parser fixture only, never native acquisition or an entry grant.
// Separate single-use fixture backing, never production's os.init occurrence.
// Calls the SAME production decoder methods without a reset/authority bypass.
var serviceLassoArgumentFixtureState serviceLassoArgumentDecodeStorage
var serviceLassoArgumentFixtureHeader [48]byte
var serviceLassoArgumentFixtureBody [len(serviceLassoArgumentBodyOriginal)]byte
var serviceLassoArgumentFixtureUTF8 [len(serviceLassoArgumentUTF8Original)]byte
var serviceLassoArgumentFixtureParsed [len(serviceLassoArgumentParsedOriginal)]byte
var serviceLassoArgumentFixturePublic [len(serviceLassoArgumentPublicOriginal)]string
var serviceLassoArgumentFixtureFallback [1]string
func ServiceLassoDecodeArgumentsFixtureForTest(header [48]byte,body []byte,expected uint64)([]string,bool) {
 o:=&serviceLassoArgumentFixtureState
 if o.Attempted||o.Returned||len(body)>len(serviceLassoArgumentFixtureBody){return nil,false}
 serviceLassoArgumentFixtureHeader=header
 copy(serviceLassoArgumentFixtureBody[:],body)
 o.BodyExtent=uint64(len(body));o.ExpectedCount=expected
 c:=serviceLassoArgumentDecodeContext{
  Original:o,Header:&serviceLassoArgumentFixtureHeader,
  Body:serviceLassoArgumentFixtureBody[:],UTF8:serviceLassoArgumentFixtureUTF8[:],
  Parsed:serviceLassoArgumentFixtureParsed[:],Public:serviceLassoArgumentFixturePublic[:],
  Fallback:&serviceLassoArgumentFixtureFallback,
 }
 ready:=c.decode()
 return o.Public,ready
}
func ServiceLassoDecodedArgumentsForTest() []string {return serviceLassoArgumentFixtureState.Public}
func ServiceLassoArgumentOriginalsForTest()([48]byte,[]byte,[]byte) {
 return serviceLassoArgumentFixtureHeader,serviceLassoArgumentFixtureBody[:serviceLassoArgumentFixtureState.BodyExtent],serviceLassoArgumentFixtureParsed[:]
}
var ServiceLassoPrivateUnsignedForTest=serviceLassoPrivateUnsigned
// Separate fixture-owned occurrence invokes the SAME production capture and
// interpreter bodies. It never resets or consumes os.init's production slot.
var serviceLassoPrivateFixtureOriginal serviceLassoPrivateConstructorStorage
func ServiceLassoPrivateConstructorFixtureForTest(command string,arguments []string) bool {
 return serviceLassoCapturePrivateConstructor(&serviceLassoPrivateFixtureOriginal,command,arguments)
}
func ServiceLassoPrivateConstructorForTest()(string,[]string,uint64,uint64,uint64) {
 o:=&serviceLassoPrivateFixtureOriginal
 return o.Command,o.Arguments,o.Endpoint,o.PublicCount,o.Failure
}

// Independently retained synthetic completed-read occurrences. No production
// original is reset/consumed; these flags/bytes establish no native completion.
type serviceLassoBindingFixture struct {
 Binding serviceLassoDecodeBindingStorage
 Root serviceLassoNativeRootStorage
 Private serviceLassoPrivateConstructorStorage
 HeaderRead serviceLassoHeaderReadStorage
 HeaderInterpret serviceLassoHeaderInterpretStorage
 BodyRead serviceLassoBodyReadStorage
 Decoder serviceLassoArgumentDecodeStorage
 Header [48]byte
 Body [8]byte
 UTF8 [24]byte
 Parsed [24]byte
 Public [2]string
 Fallback [1]string
}
var serviceLassoBindingFixtures [4]*serviceLassoBindingFixture
var serviceLassoBindingFixtureUsed uint64
func ServiceLassoCompletedDecodeFixtureForTest(header [48]byte,body []byte,mode uint64)([]string,bool,uint64) {
 if serviceLassoBindingFixtureUsed==uint64(len(serviceLassoBindingFixtures))||len(body)==0||len(body)>8{return nil,false,0}
 f:=new(serviceLassoBindingFixture)
 serviceLassoBindingFixtures[serviceLassoBindingFixtureUsed]=f;serviceLassoBindingFixtureUsed++
 f.Header=header;copy(f.Body[:],body)
 c:=serviceLassoArgumentDecodeContext{Original:&f.Decoder,Header:&f.Header,Body:f.Body[:],UTF8:f.UTF8[:],Parsed:f.Parsed[:],Public:f.Public[:],Fallback:&f.Fallback}
 count:=c.u64(32);countText:="0";if count==1{countText="1"}else if count!=0{return nil,false,0}
 args:=[]string{"service-lasso-tui-native-payload","service-lasso-tui-native-windows-v3","55",countText,"1","1","1","--"}
 if !serviceLassoCapturePrivateConstructor(&f.Private,"private fixture",args){return nil,false,0}
 f.Root.Endpoint=55;f.Root.PublicCount=count;f.Root.SecurityLabelBytes=1;f.Root.TerminalCalls=1;f.Root.RepresentationBytes=1;f.Root.ArgumentHeader=header
 f.HeaderRead.Endpoint=55;f.HeaderRead.Completed=48;f.HeaderRead.Used=1;f.HeaderRead.Attempted=1;f.HeaderRead.Returned=1
 f.HeaderInterpret.ExpectedCount=count;f.HeaderInterpret.BodyExtent=uint64(len(body));f.HeaderInterpret.Attempted=1;f.HeaderInterpret.Returned=1
 f.BodyRead.Endpoint=55;f.BodyRead.Completed=uint64(len(body));f.BodyRead.Used=1;f.BodyRead.Attempted=1;f.BodyRead.Returned=1
 if mode==1{f.BodyRead.Completed--}
 if mode==2{f.BodyRead.Endpoint=56}
 if mode==3{f.Root.ArgumentHeader[0]^=1}
 ok:=serviceLassoBindCompletedDecode(&f.Binding,&f.Root,&f.Private,&f.HeaderRead,&f.HeaderInterpret,&f.BodyRead,&c)
 return f.Decoder.Public,ok,f.Binding.Failure
}
