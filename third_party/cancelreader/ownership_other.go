//go:build !linux && !windows

package cancelreader
import "io"
// Deferred/other platforms preserve their current API. This is not qualification
// of those platform original graphs; macOS remains explicitly deferred N/A.
func newReaderWithOwnership(reader io.Reader,ownership *Ownership)(CancelReader,error){result,err:=NewReader(reader);ownership.Partial=result;return result,err}
