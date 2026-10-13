//go:build windows

package app

import (

 "unsafe"

 "golang.org/x/sys/windows"
)

// Microsoft NtCreateFile CreateOptions documents this flag for files and
// directories. CreateFileW documents NTFS metadata flush for write-through
// requests, including rename. This binds supported API prerequisites only;
// device persistence and complete native qualification remain separate.
const nativeFileWriteThrough uint32=0x00000002
var nativeVolumeInformation=windows.NewLazySystemDLL("kernel32.dll").NewProc("GetVolumeInformationByHandleW")
func qualifyWindowsNamespace(root windows.Handle)error{
 if err:=nativeVolumeInformation.Find();err!=nil{return recordJoinedErrors(storageError(),err)}
 var volumeName [261]uint16;var filesystem [32]uint16;var serial,maximumComponent,flags uint32
 result,_,cause:=nativeVolumeInformation.Call(uintptr(root),uintptr(unsafe.Pointer(&volumeName[0])),uintptr(len(volumeName)),uintptr(unsafe.Pointer(&serial)),uintptr(unsafe.Pointer(&maximumComponent)),uintptr(unsafe.Pointer(&flags)),uintptr(unsafe.Pointer(&filesystem[0])),uintptr(len(filesystem)))
 if result==0{return recordJoinedErrors(storageError(),cause)}
 info,err:=winInfo(root);if err!=nil{return err};if windows.UTF16ToString(filesystem[:])!="NTFS"||serial!=info.VolumeSerialNumber{return storageError()};return nil
}
