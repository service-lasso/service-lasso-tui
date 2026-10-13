//go:build windows && amd64

package windows

import "unsafe"

// Independent caller-owned original row, not a callback or provider grant.
// Numeric addresses borrow the held file; no Go pointers are hidden here.
type serviceLassoCreatorNativeTLS struct {
 PE uint64
 CallbackIndex uint64
 DirectoryRVA uint64
 DirectoryBytes uint64
 RawStartVA uint64
 RawEndVA uint64
 IndexVA uint64
 CallbacksVA uint64
 ZeroFill uint64
 Characteristics uint64
 TemplateRVA uint64
 TemplateBytes uint64
 Template uint64
 IndexRVA uint64
 IndexKind uint64
 CallbackArrayRVA uint64
 CallbackSlotRVA uint64
 CallbackVA uint64
 CallbackRVA uint64
 CallbackInImage uint64
 CallbackEnd uint64
 Attempted uint64
 Returned uint64
 Failure uint64
}
const serviceLassoCreatorNativeTLSExtent=192
var _ [serviceLassoCreatorNativeTLSExtent-unsafe.Sizeof(serviceLassoCreatorNativeTLS{})]byte
var _ [unsafe.Sizeof(serviceLassoCreatorNativeTLS{})-serviceLassoCreatorNativeTLSExtent]byte
var _ [8-unsafe.Alignof(serviceLassoCreatorNativeTLS{})]byte
var _ [unsafe.Alignof(serviceLassoCreatorNativeTLS{})-8]byte
var _ [168-unsafe.Offsetof(serviceLassoCreatorNativeTLS{}.Attempted)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorNativeTLS{}.Attempted)-168]byte
var _ [184-unsafe.Offsetof(serviceLassoCreatorNativeTLS{}.Failure)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorNativeTLS{}.Failure)-184]byte
