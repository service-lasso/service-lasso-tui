//go:build windows && amd64

package windows

import "unsafe"

// This object is the final initialized-Go publication occurrence, not a native
// authentication flag. Its sole future writer must be the complete os.init
// startup caller AFTER authentic entry, immutable completed reads, decode and
// public Args publication. No helper in this file creates that occurrence.
// Until that connected caller exists, adoption must return no usable views.
type serviceLassoPublishedOriginals struct {
 Root *serviceLassoNativeRootStorage
 Command string
 Private []string
 Public []string
}
var serviceLassoPublishedOriginal *serviceLassoPublishedOriginals

type serviceLassoAdoptionStorage struct {
 Published *serviceLassoPublishedOriginals
 Failure uint64
 Attempted bool
 Returned bool
}
var serviceLassoAdoptionOriginal serviceLassoAdoptionStorage

// The ordinary checked provider-push symbol has the exact matching built-in
// signature. It is never called before initialized Go and exports no foreign
// record ABI. Once state controls occurrence only; pointers and equality below
// are representation checks, never creator/entry/endpoint authority.
//go:linkname serviceLassoAdoptOriginals
func serviceLassoAdoptOriginals() (unsafe.Pointer,string,[]string,[]string) {
 o:=&serviceLassoAdoptionOriginal
 if o.Attempted||o.Returned||o.Published!=nil||o.Failure!=0{return nil,"",nil,nil}
 o.Attempted=true
 p:=serviceLassoPublishedOriginal
 // Retain the observed publication occurrence even if a later representation
 // check refuses it. Never reset, reconstruct, close or overwrite originals.
 o.Published=p
 if p==nil {o.Failure=1;o.Returned=true;return nil,"",nil,nil}
 private:=&serviceLassoPrivateConstructorOriginal
 public:=&serviceLassoArgumentDecodeOriginal
 if p.Root!=&serviceLassoNativeRoot||!private.Attempted||!private.Returned||private.Failure!=0||!public.Attempted||!public.Returned||public.Failure!=0||uint64(len(p.Public))!=public.Count||uint64(len(p.Public))!=private.PublicCount {
  o.Failure=2;o.Returned=true;return nil,"",nil,nil
 }
 if len(p.Command)!=len(private.Command)||unsafe.StringData(p.Command)!=unsafe.StringData(private.Command)||len(p.Private)!=len(private.Arguments)||unsafe.SliceData(p.Private)!=unsafe.SliceData(private.Arguments)||len(p.Public)!=len(public.Public)||unsafe.SliceData(p.Public)!=unsafe.SliceData(public.Public) {
  o.Failure=3;o.Returned=true;return nil,"",nil,nil
 }
 o.Returned=true
 return unsafe.Pointer(p.Root),p.Command,p.Private,p.Public
}
