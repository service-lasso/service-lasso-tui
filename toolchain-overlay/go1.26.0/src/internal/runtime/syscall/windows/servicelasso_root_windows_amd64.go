//go:build windows && amd64

// Owned overlay backing for the existing physical standard-library package.
// This declaration performs no initialization, acquisition or authority grant.
package windows

import "unsafe"

// Fixed transport descriptor only. Full native observation/argument/operation
// backing and the genuine before-entry binding remain separate obligations.
// All fields are pointer-free: numeric native views are never Go references.
type serviceLassoNativeRootStorage struct {
	LayoutTag           [8]byte
	LayoutVersion       uint64
	SourceExtent        uint64
	Endpoint            uint64
	PublicCount         uint64
	SecurityLabelBytes  uint64
	TerminalCalls       uint64
	RepresentationBytes uint64
	EntryGate           uint64
	ArgumentHeader      [48]byte
	Phase               uint64
}

// The actual linker symbol must have this exact name for offline COFF lookup.
// Compiler-owned pointer-free BSS; there is deliberately no duplicate GLOBL.
// Declaration alone does not establish reachability from the default entry.
var serviceLassoNativeRoot serviceLassoNativeRootStorage

const serviceLassoNativeRootExtent = 8 + 8*8 + 48 + 8
const serviceLassoNativeRootAlignment = 8
// Structural representation only; matching these never authenticates entry.
const serviceLassoNativeRootLayoutTag = "SLWROOT1"
const serviceLassoNativeRootLayoutVersion = 1

// Both directions refuse any unexpected source/compiler size or alignment.
var _ [serviceLassoNativeRootExtent - unsafe.Sizeof(serviceLassoNativeRoot)]byte
var _ [unsafe.Sizeof(serviceLassoNativeRoot) - serviceLassoNativeRootExtent]byte
var _ [serviceLassoNativeRootAlignment - unsafe.Alignof(serviceLassoNativeRoot)]byte
var _ [unsafe.Alignof(serviceLassoNativeRoot) - serviceLassoNativeRootAlignment]byte

var _ [24 - unsafe.Offsetof(serviceLassoNativeRoot.Endpoint)]byte
var _ [unsafe.Offsetof(serviceLassoNativeRoot.Endpoint) - 24]byte
var _ [64 - unsafe.Offsetof(serviceLassoNativeRoot.EntryGate)]byte
var _ [unsafe.Offsetof(serviceLassoNativeRoot.EntryGate) - 64]byte
var _ [72 - unsafe.Offsetof(serviceLassoNativeRoot.ArgumentHeader)]byte
var _ [unsafe.Offsetof(serviceLassoNativeRoot.ArgumentHeader) - 72]byte
var _ [120 - unsafe.Offsetof(serviceLassoNativeRoot.Phase)]byte
var _ [unsafe.Offsetof(serviceLassoNativeRoot.Phase) - 120]byte
