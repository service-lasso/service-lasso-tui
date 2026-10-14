//go:build !windows

package localereader

import "io"

type ownedPassthroughReader struct{source io.Reader;ownership *Ownership}
func newReaderOwned(source io.Reader,ownership *Ownership)io.Reader{reader:=&ownedPassthroughReader{source:source,ownership:ownership};ownership.Reader=reader;return reader}
func(r *ownedPassthroughReader)Read(buffer []byte)(int,error){original:=&readOriginal{Buffer:buffer};operation:=r.ownership.begin("passthrough.read",r.source,original);defer retainInterruption(operation);original.Count,original.Failure=r.source.Read(buffer);observeRead(original);operation.Output=original;operation.Failure=original.Failure;operation.Returned=true;return original.Count,original.Failure}