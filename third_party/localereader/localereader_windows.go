// +build windows

package localereader

import (
	"io"
	"syscall"
	"unicode/utf8"
	"unsafe"

	"golang.org/x/text/transform"
)

const (
	CP_ACP               = 0
	MB_ERR_INVALID_CHARS = 8
)

var (
	modkernel32             = syscall.NewLazyDLL("kernel32.dll")
	procMultiByteToWideChar = modkernel32.NewProc("MultiByteToWideChar")
	procIsDBCSLeadByte      = modkernel32.NewProc("IsDBCSLeadByte")
)

type codepageDecoder struct {
	transform.NopResetter

	cp int
 ownership *Ownership
}

func (decoder codepageDecoder) Transform(dst, src []byte, atEOF bool) (nDst, nSrc int, err error) {
	original:=&transformOriginal{Destination:dst,Source:src,AtEOF:atEOF};operation:=decoder.ownership.begin("codepage.transform",decoder,original);defer retainInterruption(operation)
 r, size := rune(0), 0
loop:
	for ; nSrc < len(src); nSrc += size {
		switch c0 := src[nSrc]; {
		case c0 < utf8.RuneSelf:
			r, size = rune(c0), 1

		default:
			lead:=&nativeCallOriginal{Source:src[nSrc:],Arguments:[]uintptr{uintptr(src[nSrc])}};leadOperation:=decoder.ownership.begin("codepage.is-dbcs-lead-byte",decoder,lead)
 lead.Observed=append([]byte(nil),lead.Source...);lead.Result,lead.Secondary,lead.Failure=procIsDBCSLeadByte.Call(lead.Arguments...);leadOperation.Output=lead;leadOperation.Failure=lead.Failure;leadOperation.Returned=true;br:=lead.Result
			if br == 0 {
				r = rune(src[nSrc])
				size = 1
				break
			}
			if nSrc >= len(src)-1 {
				r = rune(src[nSrc])
				size = 1
				break
			}
			query:=&nativeCallOriginal{Source:src[nSrc:],Arguments:[]uintptr{CP_ACP,0,uintptr(unsafe.Pointer(&src[nSrc])),2,0,0}};queryOperation:=decoder.ownership.begin("codepage.wide-char-size",decoder,query)
 query.Observed=append([]byte(nil),query.Source...);query.Result,query.Secondary,query.Failure=procMultiByteToWideChar.Call(query.Arguments...);if query.Result==0{query.CompatibilityObserved=true;query.CompatibilityError=syscall.GetLastError()};queryOperation.Output=query;queryOperation.Failure=query.Failure;queryOperation.Returned=true;n:=query.Result
			if n <= 0 {
				err=query.CompatibilityError
				break
			}
			conversion:=&nativeCallOriginal{Source:src[nSrc:]};conversionOperation:=decoder.ownership.begin("codepage.wide-char-convert",decoder,conversion)
 conversion.Arguments=[]uintptr{CP_ACP,0,uintptr(unsafe.Pointer(&src[nSrc])),2,uintptr(unsafe.Pointer(&conversion.Wide[0])),1}
 conversion.Observed=append([]byte(nil),conversion.Source...);conversion.Result,conversion.Secondary,conversion.Failure=procMultiByteToWideChar.Call(conversion.Arguments...);conversionOperation.Output=conversion;conversionOperation.Failure=conversion.Failure;conversionOperation.Returned=true;rc:=conversion.Result
			if rc == 0 {
				size = 1
				break
			}
			r = rune(conversion.Wide[0])
			size = 2
		}
		if nDst+utf8.RuneLen(r) > len(dst) {
			err = transform.ErrShortDst
			break loop
		}
		nDst += utf8.EncodeRune(dst[nDst:], r)
	}
	original.DestinationCount=nDst;original.SourceCount=nSrc;original.Failure=err;operation.Output=original;operation.Failure=err;operation.Returned=true;return nDst,nSrc,err

}

func newReader(r io.Reader) io.Reader {
	return transform.NewReader(r, NewAcpDecoder())
}

func NewCodePageDecoder(cp int) transform.Transformer {
	return &codepageDecoder{cp: cp}
}

func NewAcpDecoder() transform.Transformer {
	return &codepageDecoder{cp: CP_ACP}
}
type nativeCallOriginal struct{Source,Observed []byte;Arguments []uintptr;Wide [1]uint16;Result,Secondary uintptr;Failure error;CompatibilityObserved bool;CompatibilityError error}