//go:build windows && amd64

package windows

import "unsafe"

// Independently preowned readonly export observation. All addresses borrow
// original held-file bytes; no address is callable or authoritative here.
type serviceLassoCreatorNativeExport struct {
 PE uint64
 Named uint64
 SelectedIndex uint64
 DirectoryRVA uint64
 DirectoryBytes uint64
 OrdinalBase uint64
 FunctionCount uint64
 NameCount uint64
 FunctionsRVA uint64
 NamesRVA uint64
 OrdinalsRVA uint64
 FunctionIndex uint64
 NameRVA uint64
 Name uint64
 NameBytes uint64
 Ordinal uint64
 FunctionRVA uint64
 Hole uint64
 Forwarded uint64
 Forwarder uint64
 ForwarderBytes uint64
 Attempted uint64
 Returned uint64
 Failure uint64
}
const serviceLassoCreatorNativeExportExtent=192
var _ [serviceLassoCreatorNativeExportExtent-unsafe.Sizeof(serviceLassoCreatorNativeExport{})]byte
var _ [unsafe.Sizeof(serviceLassoCreatorNativeExport{})-serviceLassoCreatorNativeExportExtent]byte
var _ [8-unsafe.Alignof(serviceLassoCreatorNativeExport{})]byte
var _ [unsafe.Alignof(serviceLassoCreatorNativeExport{})-8]byte
var _ [168-unsafe.Offsetof(serviceLassoCreatorNativeExport{}.Attempted)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorNativeExport{}.Attempted)-168]byte
var _ [184-unsafe.Offsetof(serviceLassoCreatorNativeExport{}.Failure)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorNativeExport{}.Failure)-184]byte
