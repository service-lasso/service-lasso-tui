//go:build windows && amd64

package windows

import "unsafe"

//go:cgo_import_dynamic internal/runtime/syscall/windows.serviceLassoGetImageFileInformation GetFileInformationByHandleEx%4 "kernel32.dll"
var serviceLassoGetImageFileInformation uintptr

type serviceLassoFileBasicInformation struct {
 CreationTime int64
 LastAccessTime int64
 LastWriteTime int64
 ChangeTime int64
 FileAttributes uint32
 Padding [4]byte
}
type serviceLassoFileStandardInformation struct {
 AllocationSize int64
 EndOfFile int64
 NumberOfLinks uint32
 DeletePending uint8
 Directory uint8
 Padding [2]byte
}
type serviceLassoFileIDInformation struct {
 VolumeSerialNumber uint64
 FileID [16]byte
}
var _ [40-unsafe.Sizeof(serviceLassoFileBasicInformation{})]byte
var _ [unsafe.Sizeof(serviceLassoFileBasicInformation{})-40]byte
var _ [24-unsafe.Sizeof(serviceLassoFileStandardInformation{})]byte
var _ [unsafe.Sizeof(serviceLassoFileStandardInformation{})-24]byte
var _ [24-unsafe.Sizeof(serviceLassoFileIDInformation{})]byte
var _ [unsafe.Sizeof(serviceLassoFileIDInformation{})-24]byte
var _ [32-unsafe.Offsetof(serviceLassoFileBasicInformation{}.FileAttributes)]byte
var _ [unsafe.Offsetof(serviceLassoFileBasicInformation{}.FileAttributes)-32]byte
var _ [20-unsafe.Offsetof(serviceLassoFileStandardInformation{}.DeletePending)]byte
var _ [unsafe.Offsetof(serviceLassoFileStandardInformation{}.DeletePending)-20]byte
var _ [21-unsafe.Offsetof(serviceLassoFileStandardInformation{}.Directory)]byte
var _ [unsafe.Offsetof(serviceLassoFileStandardInformation{}.Directory)-21]byte

type serviceLassoCreatorMetadataStorage struct {
 BasicCall serviceLassoNativeCallStorage
 StandardCall serviceLassoNativeCallStorage
 IDCall serviceLassoNativeCallStorage
 Basic serviceLassoFileBasicInformation
 Standard serviceLassoFileStandardInformation
 ID serviceLassoFileIDInformation
 File uint64
 Process uint64
 Address uint64
 Attempted uint64
 Returned uint64
 Failure uint64
}
const serviceLassoCreatorMetadataExtent=1336
var _ [serviceLassoCreatorMetadataExtent-unsafe.Sizeof(serviceLassoCreatorMetadataStorage{})]byte
var _ [unsafe.Sizeof(serviceLassoCreatorMetadataStorage{})-serviceLassoCreatorMetadataExtent]byte
var _ [8-unsafe.Alignof(serviceLassoCreatorMetadataStorage{})]byte
var _ [unsafe.Alignof(serviceLassoCreatorMetadataStorage{})-8]byte
var _ [1200-unsafe.Offsetof(serviceLassoCreatorMetadataStorage{}.Basic)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorMetadataStorage{}.Basic)-1200]byte
var _ [1240-unsafe.Offsetof(serviceLassoCreatorMetadataStorage{}.Standard)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorMetadataStorage{}.Standard)-1240]byte
var _ [1264-unsafe.Offsetof(serviceLassoCreatorMetadataStorage{}.ID)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorMetadataStorage{}.ID)-1264]byte
var _ [1288-unsafe.Offsetof(serviceLassoCreatorMetadataStorage{}.File)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorMetadataStorage{}.File)-1288]byte

var _ [8-unsafe.Alignof(serviceLassoFileBasicInformation{})]byte
var _ [unsafe.Alignof(serviceLassoFileBasicInformation{})-8]byte

var _ [8-unsafe.Alignof(serviceLassoFileStandardInformation{})]byte
var _ [unsafe.Alignof(serviceLassoFileStandardInformation{})-8]byte

var _ [8-unsafe.Alignof(serviceLassoFileIDInformation{})]byte
var _ [unsafe.Alignof(serviceLassoFileIDInformation{})-8]byte
