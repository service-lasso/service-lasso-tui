//go:build windows && amd64

package windows

import "unsafe"

// Pointer-free native numeric views, never Go pointer headers or authority.
// Actual native acquisition and generated static arenas outlive all borrowers.
type serviceLassoCreatorNativePE struct {
 Read uint64
 Metadata uint64
 Bytes uint64
 Extent uint64
 Phase uint64
 ModuleIndex uint64
 Machine uint64
 Characteristics uint64
 Sections uint64
 Optional uint64
 OptionalBytes uint64
 SectionTable uint64
 ImageBase uint64
 Entry uint64
 ImageBytes uint64
 HeaderBytes uint64
 SectionAlignment uint64
 FileAlignment uint64
 Directories uint64
 Attempted uint64
 Returned uint64
 Failure uint64
}
const serviceLassoCreatorNativePEExtent=176
var _ [serviceLassoCreatorNativePEExtent-unsafe.Sizeof(serviceLassoCreatorNativePE{})]byte
var _ [unsafe.Sizeof(serviceLassoCreatorNativePE{})-serviceLassoCreatorNativePEExtent]byte
var _ [8-unsafe.Alignof(serviceLassoCreatorNativePE{})]byte
var _ [unsafe.Alignof(serviceLassoCreatorNativePE{})-8]byte
var _ [16-unsafe.Offsetof(serviceLassoCreatorNativePE{}.Bytes)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorNativePE{}.Bytes)-16]byte
var _ [152-unsafe.Offsetof(serviceLassoCreatorNativePE{}.Attempted)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorNativePE{}.Attempted)-152]byte
var _ [168-unsafe.Offsetof(serviceLassoCreatorNativePE{}.Failure)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorNativePE{}.Failure)-168]byte
