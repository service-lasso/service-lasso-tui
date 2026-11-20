//go:build !service_lasso_core_entry

package main

import (
	"os"

	"github.com/service-lasso/service-lasso-tui/internal/lifetime"
)

func main() {
	receiver := lifetime.NewReceiver()
	startup := &startupOccurrence{}
	// Selected original whitespace command lines may have zero public arguments.
	// Borrow only actual entries; do not synthesize an executable argument.
	publicZero := ""
	var publicTail []string
	if len(os.Args) > 0 {
		publicZero = os.Args[0]
		publicTail = os.Args[1:]
	}
	status := runPublicCommand(startup, receiver, publicZero, publicTail, nil)
	// Existing outer boundary, still unqualified: native generated startup,
	// complete LOCAL representation/custody and original disposition must be
	// connected here before this becomes a selected native payload exit.
	if status != 0 {
		os.Exit(status)
	}
}
