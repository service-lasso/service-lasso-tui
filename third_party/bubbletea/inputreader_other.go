//go:build !windows
// +build !windows

package tea

import (
	"fmt"
	"io"

	"github.com/muesli/cancelreader"
)

func newInputReader(r io.Reader,mouse bool)(cancelreader.CancelReader,error){return newInputReaderOwned(nil,r,mouse)}
func newInputReaderOwned(owner *Program,r io.Reader,_ bool)(cancelreader.CancelReader,error){
	var attachment *SourceOccurrence
 cr, err := cancelreader.NewReaderOwned(r,func(original *cancelreader.Ownership){attachment=owner.sourceStart("cancelreader.construct-owner",r,original)})
 attachment.Output=cr;attachment.Failure=err;attachment.Returned=true
	if err != nil {
		return nil, fmt.Errorf("bubbletea: error creating cancel reader: %w", err)
	}
	return cr, nil
}
