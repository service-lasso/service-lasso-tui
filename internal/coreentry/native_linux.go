//go:build linux

package coreentry

// NativeResult mirrors the private sl_core_initial_entry_result values.
type NativeResult int

const (
	NativeAcquired            NativeResult = 0
	NativeReleased            NativeResult = 1
	NativeRetainedUnavailable NativeResult = 2
	NativeRefused             NativeResult = 3
)
