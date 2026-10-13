//go:build windows && amd64

package windows

import "unsafe"

// This source-owned monotone cursor apportions preowned static bytes only.
// It is not a containing reservation, a capacity certificate or authority.
var serviceLassoCreatorFileArenaUsed uint64

type serviceLassoCreatorFileReadStorage struct {
 File uint64
 Process uint64
 Address uint64
 Metadata uint64
 ArenaOffset uint64
 Extent uint64
 Completed uint64
 CallsOffset uint64
 Used uint64
 Attempted uint64
 Returned uint64
 Failure uint64
}
const serviceLassoCreatorFileReadExtent=96
var _ [serviceLassoCreatorFileReadExtent-unsafe.Sizeof(serviceLassoCreatorFileReadStorage{})]byte
var _ [unsafe.Sizeof(serviceLassoCreatorFileReadStorage{})-serviceLassoCreatorFileReadExtent]byte
var _ [8-unsafe.Alignof(serviceLassoCreatorFileReadStorage{})]byte
var _ [unsafe.Alignof(serviceLassoCreatorFileReadStorage{})-8]byte
var _ [40-unsafe.Offsetof(serviceLassoCreatorFileReadStorage{}.Extent)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorFileReadStorage{}.Extent)-40]byte
var _ [48-unsafe.Offsetof(serviceLassoCreatorFileReadStorage{}.Completed)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorFileReadStorage{}.Completed)-48]byte
var _ [64-unsafe.Offsetof(serviceLassoCreatorFileReadStorage{}.Used)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorFileReadStorage{}.Used)-64]byte
// Native records are the SAME guarded128B ReadFile layout, not a copied model.
type serviceLassoCreatorFileReadCall = serviceLassoBodyReadCall
