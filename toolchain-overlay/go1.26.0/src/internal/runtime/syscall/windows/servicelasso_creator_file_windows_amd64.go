//go:build windows && amd64

package windows

import "unsafe"

// A native existing-file open is an acquired handle occurrence, not image or
// loaded-provider authentication. Exact file/PE/writer joins remain separate.
//go:cgo_import_dynamic internal/runtime/syscall/windows.serviceLassoCreateImageFile CreateFileW%7 "kernel32.dll"
var serviceLassoCreateImageFile uintptr

const serviceLassoGlobalRootPrefixUnits=14

type serviceLassoCreatorFileStorage struct {
 Call serviceLassoNativeCallStorage
 Process uint64
 Address uint64
 NameBuffer uint64
 PathBuffer uint64
 NameCount uint32
 Padding [4]byte
 PathUnits uint64
 File uint64
 Attempted uint64
 Returned uint64
 Failure uint64
}
const serviceLassoCreatorFileExtent=480
var _ [serviceLassoCreatorFileExtent-unsafe.Sizeof(serviceLassoCreatorFileStorage{})]byte
var _ [unsafe.Sizeof(serviceLassoCreatorFileStorage{})-serviceLassoCreatorFileExtent]byte
var _ [8-unsafe.Alignof(serviceLassoCreatorFileStorage{})]byte
var _ [unsafe.Alignof(serviceLassoCreatorFileStorage{})-8]byte
var _ [432-unsafe.Offsetof(serviceLassoCreatorFileStorage{}.NameCount)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorFileStorage{}.NameCount)-432]byte
var _ [440-unsafe.Offsetof(serviceLassoCreatorFileStorage{}.PathUnits)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorFileStorage{}.PathUnits)-440]byte
var _ [448-unsafe.Offsetof(serviceLassoCreatorFileStorage{}.File)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorFileStorage{}.File)-448]byte
