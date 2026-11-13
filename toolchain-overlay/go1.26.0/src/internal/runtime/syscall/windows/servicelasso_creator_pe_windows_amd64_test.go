//go:build windows && amd64

package windows_test

import (
 "bytes"
 "encoding/binary"
 "internal/runtime/syscall/windows"
 "testing"
)

// UNRUN synthetic representation tests of the actual production interpreter.
// No OS load, native file, provider or immutable acquisition is qualified.
func TestServiceLassoCreatorPERepresentation(t *testing.T) {
 makePE:=func() []byte {
  b:=make([]byte,1024)
  put16:=func(o int,v uint16){binary.LittleEndian.PutUint16(b[o:o+2],v)}
  put32:=func(o int,v uint32){binary.LittleEndian.PutUint32(b[o:o+4],v)}
  put16(0,0x5a4d);put32(60,64);put32(64,0x4550)
  put16(68,0x8664);put16(70,1);put16(84,240);put16(86,0x2022)
  put16(88,0x20b);put32(104,4096)
  binary.LittleEndian.PutUint64(b[112:120],0x140000000)
  put32(120,4096);put32(124,512);put32(144,8192);put32(148,512);put32(196,16)
  put32(336,1);put32(340,4096);put32(344,512);put32(348,512)
  return b
 }
 cases:=[]struct{name string; mutate func([]byte)[]byte;want bool}{
  {"ordinary",func(b []byte)[]byte{return b},true},
  {"short",func(b []byte)[]byte{return b[:63]},false},
  {"negative-lfanew",func(b []byte)[]byte{binary.LittleEndian.PutUint32(b[60:64],0xffffffff);return b},false},
  {"optional-short",func(b []byte)[]byte{binary.LittleEndian.PutUint16(b[84:86],111);return b},false},
  {"directory-outside-optional",func(b []byte)[]byte{binary.LittleEndian.PutUint32(b[196:200],17);return b},false},
  {"unaligned-section",func(b []byte)[]byte{binary.LittleEndian.PutUint32(b[340:344],4097);return b},false},
  {"raw-past-file",func(b []byte)[]byte{binary.LittleEndian.PutUint32(b[344:348],1024);return b},false},
  {"raw-exceeds-virtual-mapped-end",func(b []byte)[]byte{binary.LittleEndian.PutUint32(b[344:348],8192);return append(b,make([]byte,8192)...)},false},
  {"security-file-offset",func(b []byte)[]byte{binary.LittleEndian.PutUint32(b[232:236],512);binary.LittleEndian.PutUint32(b[236:240],8);return b},true},
  {"security-outside-file",func(b []byte)[]byte{binary.LittleEndian.PutUint32(b[232:236],4096);binary.LittleEndian.PutUint32(b[236:240],8);return b},false},
 }
 for _,c:=range cases {t.Run(c.name,func(t *testing.T){
  b:=c.mutate(makePE()); before:=append([]byte(nil),b...)
  ok,failure:=windows.ServiceLassoCreatorPEFixtureForTest(b)
  if ok!=c.want || (ok && failure!=0) || (!ok && failure==0) {t.Fatalf("ready=%v failure=%d",ok,failure)}
  if !bytes.Equal(b,before){t.Fatal("interpreter mutated original bytes")}
 })}
 b:=makePE(); binary.LittleEndian.PutUint32(b[336:340],1024)
 for _,c:=range []struct{rva,size,kind uint64}{{0,64,1},{4096,512,1},{4608,512,2},{4607,2,0},{512,1,0},{5120,1,0},{4096,0,0},{8191,2,0}} {
  view,kind:=windows.ServiceLassoCreatorPEViewFixtureForTest(b,c.rva,c.size)
  if kind!=c.kind {t.Fatalf("RVA %d/%d kind=%d want=%d",c.rva,c.size,kind,c.kind)}
  if kind==1 {
   original:=c.rva; if c.rva>=4096 {original=512+c.rva-4096}
   if uint64(len(view))!=c.size || &view[0]!=&b[original] {t.Fatal("view replaced original backing")}
  } else if view!=nil {t.Fatal("gap/zero-fill manufactured backing")}
 }
 relocation:=makePE()
 binary.LittleEndian.PutUint32(relocation[240:244],4096)
 binary.LittleEndian.PutUint32(relocation[244:248],12)
 binary.LittleEndian.PutUint32(relocation[512:516],4096)
 binary.LittleEndian.PutUint32(relocation[516:520],12)
 binary.LittleEndian.PutUint16(relocation[520:522],0xa010)
 binary.LittleEndian.PutUint64(relocation[528:536],0xfffffffffffffff0)
 ok,typ,target,original,expected:=windows.ServiceLassoCreatorPERelocationFixtureForTest(relocation,0,0,0x140000020)
 if !ok || typ!=10 || target!=4112 || original!=0xfffffffffffffff0 || expected!=16 {t.Fatal("DIR64 modular projection")}
 binary.LittleEndian.PutUint16(relocation[520:522],0x3010)
 if ok,_,_,_,_:=windows.ServiceLassoCreatorPERelocationFixtureForTest(relocation,0,0,0x140000020);ok {t.Fatal("unsupported relocation accepted")}
 binary.LittleEndian.PutUint16(relocation[520:522],0xafff)
 if ok,_,_,_,_:=windows.ServiceLassoCreatorPERelocationFixtureForTest(relocation,0,0,0x140000020);ok {t.Fatal("target crosses mapped extent")}
 binary.LittleEndian.PutUint16(relocation[520:522],0)
 if ok,typ,_,_,_:=windows.ServiceLassoCreatorPERelocationFixtureForTest(relocation,0,0,0x140000020);!ok || typ!=0 {t.Fatal("ABSOLUTE padding refused")}
 binary.LittleEndian.PutUint32(relocation[516:520],14)
 if ok,_,_,_,_:=windows.ServiceLassoCreatorPERelocationFixtureForTest(relocation,0,0,0x140000020);ok {t.Fatal("block exceeds directory")}
 imports:=makePE()
 binary.LittleEndian.PutUint32(imports[208:212],4096);binary.LittleEndian.PutUint32(imports[212:216],40)
 binary.LittleEndian.PutUint32(imports[512:516],4160);binary.LittleEndian.PutUint32(imports[524:528],4192);binary.LittleEndian.PutUint32(imports[528:532],4176)
 binary.LittleEndian.PutUint64(imports[576:584],4208)
 copy(imports[608:],"kernel32.dll\x00");binary.LittleEndian.PutUint16(imports[624:626],7);copy(imports[626:],"ReadFile\x00")
 ok,descriptorEnd,thunkEnd,ordinal,module,name:=windows.ServiceLassoCreatorPEImportFixtureForTest(imports,0,0)
 if !ok || descriptorEnd || thunkEnd || ordinal!=0 || string(module)!="kernel32.dll" || string(name)!="ReadFile" || &module[0]!=&imports[608] || &name[0]!=&imports[626] {t.Fatal("import original named views")}
 binary.LittleEndian.PutUint32(imports[524:528],0)
 if ok,_,_,_,_,_:=windows.ServiceLassoCreatorPEImportFixtureForTest(imports,0,0);ok {t.Fatal("null module name interpreted as DOS header")}
 binary.LittleEndian.PutUint32(imports[524:528],4192)
 if ok,de,te,_,_,_:=windows.ServiceLassoCreatorPEImportFixtureForTest(imports,0,1);!ok || de || !te {t.Fatal("thunk terminal")}
 if ok,end,iat:=windows.ServiceLassoCreatorPEImportIATFixtureForTest(imports,0,1);!ok || !end || iat!=4184 {t.Fatal("terminal IAT locator not retained")}
 binary.LittleEndian.PutUint32(imports[528:532],8184)
 if ok,_,_:=windows.ServiceLassoCreatorPEImportIATFixtureForTest(imports,0,1);ok {t.Fatal("terminal IAT beyond image")}
 binary.LittleEndian.PutUint32(imports[528:532],4176)
 if ok,de,_,_,_,_:=windows.ServiceLassoCreatorPEImportFixtureForTest(imports,1,0);!ok || !de {t.Fatal("descriptor terminal")}
 binary.LittleEndian.PutUint64(imports[576:584],0x8000000000000012)
 if ok,_,_,ord,_,name:=windows.ServiceLassoCreatorPEImportFixtureForTest(imports,0,0);!ok || ord!=18 || name!=nil {t.Fatal("ordinal import")}
 binary.LittleEndian.PutUint64(imports[576:584],0x8000000000010012)
 if ok,_,_,_,_,_:=windows.ServiceLassoCreatorPEImportFixtureForTest(imports,0,0);ok {t.Fatal("reserved ordinal bits")}
 binary.LittleEndian.PutUint64(imports[576:584],4208)
 for i:=626;i<len(imports);i++ {imports[i]='x'}
 if ok,_,_,_,_,_:=windows.ServiceLassoCreatorPEImportFixtureForTest(imports,0,0);ok {t.Fatal("unterminated original name")}
 delay:=makePE()
 binary.LittleEndian.PutUint32(delay[304:308],4096);binary.LittleEndian.PutUint32(delay[308:312],64)
 for i,v:=range []uint32{1,4192,4224,4176,4160,4240,4256,0} {binary.LittleEndian.PutUint32(delay[512+4*i:516+4*i],v)}
 binary.LittleEndian.PutUint64(delay[576:584],4208)
 copy(delay[608:],"kernel32.dll\x00");copy(delay[626:],"ReadFile\x00")
 if ok,de,te,iat,bound,module,name:=windows.ServiceLassoCreatorPEDelayFixtureForTest(delay,0,0);!ok || de || te || iat!=4176 || bound!=4240 || string(module)!="kernel32.dll" || string(name)!="ReadFile" || &module[0]!=&delay[608] || &name[0]!=&delay[626] {t.Fatal("delay original locators")}
 if ok,_,end,iat,bound,_,_:=windows.ServiceLassoCreatorPEDelayFixtureForTest(delay,0,1);!ok || !end || iat!=4184 || bound!=4248 {t.Fatal("delay terminal slots")}
 if ok,end,_,_,_,_,_:=windows.ServiceLassoCreatorPEDelayFixtureForTest(delay,1,0);!ok || !end {t.Fatal("delay descriptor terminal")}
 binary.LittleEndian.PutUint32(delay[512:516],3)
 if ok,_,_,_,_,_,_:=windows.ServiceLassoCreatorPEDelayFixtureForTest(delay,0,0);ok {t.Fatal("delay reserved attributes")}
 binary.LittleEndian.PutUint32(delay[512:516],1);binary.LittleEndian.PutUint32(delay[524:528],8184)
 if ok,_,_,_,_,_,_:=windows.ServiceLassoCreatorPEDelayFixtureForTest(delay,0,1);ok {t.Fatal("delay terminal IAT beyond image")}
 tls:=makePE()
 binary.LittleEndian.PutUint32(tls[272:276],4096);binary.LittleEndian.PutUint32(tls[276:280],40)
 binary.LittleEndian.PutUint64(tls[528:536],0x140001080);binary.LittleEndian.PutUint64(tls[536:544],0x140001040)
 binary.LittleEndian.PutUint64(tls[576:584],0x140001100)
 if ok,end,local,va,index:=windows.ServiceLassoCreatorPETLSFixtureForTest(tls,0);!ok || end || !local || va!=0x140001100 || index!=4224 {t.Fatal("TLS original locations")}
 if ok,end,_,_,_:=windows.ServiceLassoCreatorPETLSFixtureForTest(tls,1);!ok || !end {t.Fatal("TLS actual null")}
 binary.LittleEndian.PutUint64(tls[576:584],0x7fff12340000)
 if ok,end,local,va,_:=windows.ServiceLassoCreatorPETLSFixtureForTest(tls,0);!ok || end || local || va!=0x7fff12340000 {t.Fatal("external callback mislabeled")}
 binary.LittleEndian.PutUint64(tls[536:544],0x140001fff)
 if ok,_,_,_,_:=windows.ServiceLassoCreatorPETLSFixtureForTest(tls,0);ok {t.Fatal("TLS array crossing image")}
 exports:=makePE()
 binary.LittleEndian.PutUint32(exports[200:204],4096);binary.LittleEndian.PutUint32(exports[204:208],128)
 binary.LittleEndian.PutUint32(exports[528:532],1);binary.LittleEndian.PutUint32(exports[532:536],2);binary.LittleEndian.PutUint32(exports[536:540],1)
 binary.LittleEndian.PutUint32(exports[540:544],4136);binary.LittleEndian.PutUint32(exports[544:548],4144);binary.LittleEndian.PutUint32(exports[548:552],4148)
 binary.LittleEndian.PutUint32(exports[552:556],4160);binary.LittleEndian.PutUint32(exports[560:564],4184)
 copy(exports[576:],"KERNELBASE.ReadFile\x00");copy(exports[600:],"ReadFile\x00")
 if ok,hole,forwarded,ordinal,name,forwarder:=windows.ServiceLassoCreatorPEExportFixtureForTest(exports,0,true);!ok || hole || !forwarded || ordinal!=1 || string(name)!="ReadFile" || string(forwarder)!="KERNELBASE.ReadFile" {t.Fatal("export original forwarder")}
 if ok,hole,_,ordinal,_,_:=windows.ServiceLassoCreatorPEExportFixtureForTest(exports,1,false);!ok || !hole || ordinal!=2 {t.Fatal("ordinal-only null hole")}
 binary.LittleEndian.PutUint16(exports[564:566],2)
 if ok,_,_,_,_,_:=windows.ServiceLassoCreatorPEExportFixtureForTest(exports,0,true);ok {t.Fatal("export ordinal index bounds")}
 binary.LittleEndian.PutUint16(exports[564:566],0)
 binary.LittleEndian.PutUint32(exports[540:544],0)
 if ok,_,_,_,_,_:=windows.ServiceLassoCreatorPEExportFixtureForTest(exports,0,false);ok {t.Fatal("nonempty export functions at null RVA")}
 binary.LittleEndian.PutUint32(exports[540:544],4136);binary.LittleEndian.PutUint32(exports[544:548],0)
 if ok,_,_,_,_,_:=windows.ServiceLassoCreatorPEExportFixtureForTest(exports,0,true);ok {t.Fatal("nonempty export names at null RVA")}
 binary.LittleEndian.PutUint32(exports[544:548],4144);binary.LittleEndian.PutUint32(exports[548:552],0)
 if ok,_,_,_,_,_:=windows.ServiceLassoCreatorPEExportFixtureForTest(exports,0,true);ok {t.Fatal("nonempty export ordinals at null RVA")}
}
