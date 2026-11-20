//go:build linux && !cgo

package coreentry

// AcquireNative refuses because the private native contract is not linked in
// this build. It never substitutes Go-side operands for native originals.
func (e *Entry) AcquireNative() NativeResult {
	if e == nil || e.Image == nil {
		return NativeRefused
	}
	return NativeRetainedUnavailable
}
