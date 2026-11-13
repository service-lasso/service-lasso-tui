//go:build windows && amd64

package windows

import "unsafe"

//go:cgo_import_dynamic internal/runtime/syscall/windows.serviceLassoGetModuleInformation K32GetModuleInformation%4 "kernel32.dll"
var serviceLassoGetModuleInformation uintptr

type serviceLassoModuleInformation struct {
 Base uint64
 SizeOfImage uint32
 Padding [4]byte
 Entry uint64
}
var _ [24-unsafe.Sizeof(serviceLassoModuleInformation{})]byte
var _ [unsafe.Sizeof(serviceLassoModuleInformation{})-24]byte
var _ [8-unsafe.Alignof(serviceLassoModuleInformation{})]byte
var _ [unsafe.Alignof(serviceLassoModuleInformation{})-8]byte
var _ [8-unsafe.Offsetof(serviceLassoModuleInformation{}.SizeOfImage)]byte
var _ [unsafe.Offsetof(serviceLassoModuleInformation{}.SizeOfImage)-8]byte
var _ [16-unsafe.Offsetof(serviceLassoModuleInformation{}.Entry)]byte
var _ [unsafe.Offsetof(serviceLassoModuleInformation{}.Entry)-16]byte

type serviceLassoCreatorExtentStorage struct {
 Call serviceLassoNativeCallStorage
 Information serviceLassoModuleInformation
 Process uint64
 Address uint64
 ImageEnd uint64
 Failure uint64
 Attempted uint64
 Returned uint64
}
const serviceLassoCreatorExtentBytes=472
var _ [serviceLassoCreatorExtentBytes-unsafe.Sizeof(serviceLassoCreatorExtentStorage{})]byte
var _ [unsafe.Sizeof(serviceLassoCreatorExtentStorage{})-serviceLassoCreatorExtentBytes]byte
var _ [8-unsafe.Alignof(serviceLassoCreatorExtentStorage{})]byte
var _ [unsafe.Alignof(serviceLassoCreatorExtentStorage{})-8]byte
var _ [400-unsafe.Offsetof(serviceLassoCreatorExtentStorage{}.Information)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorExtentStorage{}.Information)-400]byte
var _ [440-unsafe.Offsetof(serviceLassoCreatorExtentStorage{}.ImageEnd)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorExtentStorage{}.ImageEnd)-440]byte
