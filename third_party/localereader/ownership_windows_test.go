//go:build windows

package localereader

import("bytes";"io";"testing";"golang.org/x/text/transform")

// Actual active-ACP calls only when separately admitted on Windows; stored cp
// is ignored by the original CP_ACP implementation and is not a 932 guarantee.
func TestPublicDecoderASCII(t *testing.T){input:=[]byte("ASCII");actual,err:=io.ReadAll(transform.NewReader(bytes.NewReader(input),NewCodePageDecoder(932)));if err!=nil||!bytes.Equal(actual,input){t.Fatal("ASCII decoder projection changed",err)}}
func TestOwnedWindowsActiveACPProjectionAndActualNativeOriginals(t *testing.T){input:=[]byte{0x82,0xA0};var ownership *Ownership;reader:=NewReaderOwned(bytes.NewReader(input),func(value *Ownership){ownership=value});actual,actualErr:=io.ReadAll(reader);expected,expectedErr:=io.ReadAll(NewReader(bytes.NewReader(input)));if !bytes.Equal(actual,expected)||(actualErr==nil)!=(expectedErr==nil){t.Fatal("active-ACP projection changed")}
 observed:=false;ownership.VisitOriginals(func(operation *OriginalOperation){if operation.Source=="codepage.is-dbcs-lead-byte"{frame:=operation.Input.(*nativeCallOriginal);observed=observed||operation.Returned&&len(frame.Source)>0&&len(frame.Arguments)==1&&frame.Arguments[0]==uintptr(frame.Observed[0])}});if !observed||ownership.Reader!=reader||ownership.Decoder==nil{t.Fatal("actual native decoder observations detached")}}