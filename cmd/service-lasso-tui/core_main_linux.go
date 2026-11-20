//go:build linux && service_lasso_core_entry

package main

import (
	"os"

	"github.com/service-lasso/service-lasso-tui/internal/coreentry"
	"github.com/service-lasso/service-lasso-tui/internal/lifetime"
)

// The Core-bound product entry refuses unless the inherited Core handoff is
// present. The receiver below owns local API/app lifetime only; it does not
// acquire a charge, source, domain, issuer, deadline, or custody object.
func main() {
	entry, err := coreentry.BindInherited(false)
	if err != nil {
		os.Exit(2)
	}
	defer entry.Close()
	receiver := lifetime.NewReceiver()
	coreOriginal := &lifetime.Original{}
	if err := receiver.Enroll(coreOriginal, "core.entry", entry); err != nil {
		os.Exit(2)
	}
	startup := &startupOccurrence{}
	publicZero := ""
	var publicTail []string
	if len(os.Args) > 0 {
		publicZero = os.Args[0]
		publicTail = os.Args[1:]
	}
	status := runPublicCommand(startup, receiver, publicZero, publicTail, nil)
	if status != 0 {
		os.Exit(status)
	}
}
