//go:build !linux && !windows

package app

// Native operation restoration is qualified only on the selected Windows and
// Linux programme. Other platforms do not substitute a weaker file protocol.
func acquireOperationStore(string)(heldOperationStore,error){return nil,storageError()}
