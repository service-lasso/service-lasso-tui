//go:build linux

package main

import (
	"fmt"
	"os"

	"github.com/service-lasso/service-lasso-tui/internal/coreentry"
)

// Test/local consumer harness. It consumes the inherited Core entry over fds
// 3/4 and exits before any app/API effect.
func main() {
	allowSynthetic := false
	if len(os.Args) == 2 && os.Args[1] == "--development-synthetic" {
		allowSynthetic = true
	} else if len(os.Args) != 1 {
		os.Exit(2)
	}
	entry, err := coreentry.BindInherited(allowSynthetic)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(2)
	}
	fmt.Println(entry.LeaseID)
	native := entry.AcquireNative()
	if native != coreentry.NativeAcquired && native != coreentry.NativeRetainedUnavailable {
		fmt.Fprintf(os.Stderr, "unexpected native result: %d\n", native)
		os.Exit(5)
	}
	fmt.Printf("native=%d\n", native)
	if err := entry.Release(); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(3)
	}
	if err := entry.Close(); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(4)
	}
}
