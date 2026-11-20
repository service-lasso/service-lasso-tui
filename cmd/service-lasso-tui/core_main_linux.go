//go:build linux && service_lasso_core_entry

package main

import (
	"os"

	"github.com/service-lasso/service-lasso-tui/internal/coreentry"
	"github.com/service-lasso/service-lasso-tui/internal/lifetime"
)

func main() {
	os.Exit(runCoreEntry())
}

// The Core-bound product entry refuses unless the inherited Core handoff is
// present. The receiver below owns local API/app lifetime only; it does not
// acquire a charge, source, domain, issuer, deadline, or custody object.
func runCoreEntry() int {
	entry, err := coreentry.BindInherited(false)
	if err != nil {
		return 2
	}
	defer entry.Close()
	defer entry.Release()
	receiver := lifetime.NewReceiver()
	coreOriginal := &lifetime.Original{}
	if err := receiver.Enroll(coreOriginal, "core.entry", entry); err != nil {
		return 2
	}
	if entry.AcquireNative() != coreentry.NativeAcquired {
		return 3
	}
	startup := &startupOccurrence{}
	publicZero := ""
	var publicTail []string
	if len(os.Args) > 0 {
		publicZero = os.Args[0]
		publicTail = os.Args[1:]
	}
	return runPublicCommand(startup, receiver, publicZero, publicTail, nil)
}
