//go:build windows

package localereader

import "io"

func newReaderOwned(source io.Reader,ownership *Ownership)io.Reader{decoder:=&codepageDecoder{cp:CP_ACP,ownership:ownership};ownership.Decoder=decoder;return newOwnedTransformReader(source,decoder,ownership)}