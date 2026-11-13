// Copyright 2013 The Go Authors. All rights reserved.
// Derived from golang.org/x/text v0.3.8 transform.Reader; BSD-style license in
// LICENSE.x-text. This is locally introduced source, not an upstream mattn file.
package localereader

import("io";"errors";"golang.org/x/text/transform")
var errInconsistentByteCount=errors.New("transform: inconsistent byte count returned")


// Reader wraps another io.Reader by transforming the bytes read.
type ownedTransformReader struct {
	r   io.Reader
 ownership *Ownership
	t   transform.Transformer
	err error

	// dst[dst0:dst1] contains bytes that have been transformed by t but
	// not yet copied out via Read.
	dst        []byte
	dst0, dst1 int

	// src[src0:src1] contains bytes that have been read from r but not
	// yet transformed through t.
	src        []byte
	src0, src1 int

	// transformComplete is whether the transformation is complete,
	// regardless of whether or not it was successful.
	transformComplete bool
}

const defaultBufSize = 4096

// NewReader returns a new Reader that wraps r by transforming the bytes read
// via t. It calls Reset on t.
func newOwnedTransformReader(r io.Reader,t transform.Transformer,ownership *Ownership)*ownedTransformReader {
	result:=&ownedTransformReader{r:r,t:t,ownership:ownership};ownership.Reader=result;ownership.Decoder=t
 reset:=ownership.begin("transform.reset",t,nil);defer retainInterruption(reset);t.Reset();reset.Returned=true
 result.dst=make([]byte,defaultBufSize);result.src=make([]byte,defaultBufSize);return result
}

func(r *ownedTransformReader)Read(buffer []byte)(int,error){original:=&readOriginal{Buffer:buffer};operation:=r.ownership.begin("transform-reader.read",r,original);defer retainInterruption(operation);original.Count,original.Failure=r.read(buffer);observeRead(original);operation.Output=original;operation.Failure=original.Failure;operation.Returned=true;return original.Count,original.Failure}
// Read implements the io.Reader interface.
func (r *ownedTransformReader)read(p []byte)(int,error) {
	n, err := 0, error(nil)
	for {
		// Copy out any transformed bytes and return the final error if we are done.
		if r.dst0 != r.dst1 {
			copying:=r.ownership.begin("transform.copy-out",r,struct{Caller,Source []byte}{p,r.dst[r.dst0:r.dst1]})
 n = copy(p,r.dst[r.dst0:r.dst1]);copying.Output=n;copying.Returned=true
			r.dst0 += n
			if r.dst0 == r.dst1 && r.transformComplete {
				return n, r.err
			}
			return n, nil
		} else if r.transformComplete {
			return 0, r.err
		}

		// Try to transform some source bytes, or to flush the transformer if we
		// are out of source bytes. We do this even if r.r.Read returned an error.
		// As the io.Reader documentation says, "process the n > 0 bytes returned
		// before considering the error".
		if r.src0 != r.src1 || r.err != nil {
			r.dst0 = 0
			call:=&transformOriginal{Destination:r.dst,Source:r.src[r.src0:r.src1],AtEOF:r.err==io.EOF};operation:=r.ownership.begin("transform.invoke",r.t,call)
 call.SourceObserved=append([]byte(nil),call.Source...);call.DestinationBefore=append([]byte(nil),call.Destination...)
 call.DestinationCount,call.SourceCount,call.Failure=r.t.Transform(call.Destination,call.Source,call.AtEOF);operation.Output=call;operation.Failure=call.Failure;operation.Returned=true
 if call.DestinationCount<0||call.DestinationCount>len(call.Destination)||call.SourceCount<0||call.SourceCount>len(call.Source){r.err=&invalidTransformExtent{Original:call};r.transformComplete=true;continue}
 call.DestinationAfter=append([]byte(nil),call.Destination[:call.DestinationCount]...)
 r.dst1,n,err=call.DestinationCount,call.SourceCount,call.Failure
			r.src0 += n

			switch {
			case err == nil:
				if r.src0 != r.src1 {
					r.err = errInconsistentByteCount
				}
				// The Transform call was successful; we are complete if we
				// cannot read more bytes into src.
				r.transformComplete = r.err != nil
				continue
			case err == transform.ErrShortDst && (r.dst1 != 0 || n != 0):
				// Make room in dst by copying out, and try again.
				continue
			case err == transform.ErrShortSrc && r.src1-r.src0 != len(r.src) && r.err == nil:
				// Read more bytes into src via the code below, and try again.
			default:
				r.transformComplete = true
				// The reader error (r.err) takes precedence over the
				// transformer error (err) unless r.err is nil or io.EOF.
				if r.err == nil || r.err == io.EOF {
					r.err = err
				}
				continue
			}
		}

		// Move any untransformed source bytes to the start of the buffer
		// and read more bytes.
		if r.src0 != 0 {
			moving:=r.ownership.begin("transform.compact-source",r,struct{Destination,Source []byte}{r.src,r.src[r.src0:r.src1]})
 r.src0,r.src1=0,copy(r.src,r.src[r.src0:r.src1]);moving.Output=r.src1;moving.Returned=true
		}
		call:=&readOriginal{Buffer:r.src[r.src1:]};operation:=r.ownership.begin("transform.source-read",r.r,call)
 call.Count,call.Failure=r.r.Read(call.Buffer);operation.Output=call;operation.Failure=call.Failure;operation.Returned=true
 if call.Count<0||call.Count>len(call.Buffer){r.err=&invalidReadExtent{Original:call};r.transformComplete=true;continue}
 observeRead(call);n,r.err=call.Count,call.Failure
		r.src1 += n
	}
}


type invalidTransformExtent struct{Original *transformOriginal}
func(e *invalidTransformExtent)Error()string{return "locale transformer returned an invalid byte extent"}
func(e *invalidTransformExtent)Unwrap()error{return e.Original.Failure}
type invalidReadExtent struct{Original *readOriginal}
func(e *invalidReadExtent)Error()string{return "locale source returned an invalid byte extent"}
func(e *invalidReadExtent)Unwrap()error{return e.Original.Failure}