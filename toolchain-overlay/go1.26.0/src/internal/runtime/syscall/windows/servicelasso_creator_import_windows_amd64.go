//go:build windows && amd64

package windows

import "unsafe"

// Numeric native locators/counts, not Go pointers or provider capabilities.
// The actual owning factory must enroll/finance every distinct retained row.
type serviceLassoCreatorNativeImport struct {
 PE uint64
 DescriptorIndex uint64
 ThunkIndex uint64
 OriginalFirstThunk uint64
 Timestamp uint64
 ForwarderChain uint64
 NameRVA uint64
 FirstThunk uint64
 ThunkRVA uint64
 IATRVA uint64
 RawThunk uint64
 Hint uint64
 Ordinal uint64
 Module uint64
 ModuleBytes uint64
 Name uint64
 NameBytes uint64
 DescriptorEnd uint64
 ThunkEnd uint64
 UsesFirstThunk uint64
 Attempted uint64
 Returned uint64
 Failure uint64
}
const serviceLassoCreatorNativeImportExtent=184
var _ [serviceLassoCreatorNativeImportExtent-unsafe.Sizeof(serviceLassoCreatorNativeImport{})]byte
var _ [unsafe.Sizeof(serviceLassoCreatorNativeImport{})-serviceLassoCreatorNativeImportExtent]byte
var _ [8-unsafe.Alignof(serviceLassoCreatorNativeImport{})]byte
var _ [unsafe.Alignof(serviceLassoCreatorNativeImport{})-8]byte
var _ [160-unsafe.Offsetof(serviceLassoCreatorNativeImport{}.Attempted)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorNativeImport{}.Attempted)-160]byte
var _ [176-unsafe.Offsetof(serviceLassoCreatorNativeImport{}.Failure)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorNativeImport{}.Failure)-176]byte
