//go:build windows && amd64

package lifetime

import "unsafe"

// Matching initialized-Go provider pull. Normal linker checks stay enabled;
// the owned lower provider marks this exact symbol with a one-argument push.
// No foreign record layout, pre-TLS Go call or public argument locator exists.
//go:linkname adoptWindowsProviderOriginals internal/runtime/syscall/windows.serviceLassoAdoptOriginals
func adoptWindowsProviderOriginals() (unsafe.Pointer,string,[]string,[]string)

// Ordinary typed receiver roots retain the SAME borrowed references. The raw
// pointer is opaque association data, never dereferenced as a foreign Go record
// or treated as authentication. This receiver is not an entry/read/LOCAL grant.
type nativeWindowsProviderOriginals struct {
 Root unsafe.Pointer
 PrivateCommand string
 PrivateArguments []string
 PublicArguments []string
 Failure uint64
 Attempted bool
 Returned bool
}

func (o *nativeWindowsProviderOriginals) adopt() bool {
 if o==nil||o.Attempted||o.Returned||o.Root!=nil||o.PrivateCommand!=""||o.PrivateArguments!=nil||o.PublicArguments!=nil||o.Failure!=0{return false}
 o.Attempted=true
 // Capture all returned references together before interpreting any of them.
 // Refusal cannot erase private/public GC roots or manufacture replacement
 // strings. A zero-length public slice is a valid original public argument
 // outcome; do not substitute argv[0] or a fabricated fallback here.
 o.Root,o.PrivateCommand,o.PrivateArguments,o.PublicArguments=adoptWindowsProviderOriginals()
 if o.Root==nil {o.Failure=1;o.Returned=true;return false}
 o.Returned=true
 return true
}
