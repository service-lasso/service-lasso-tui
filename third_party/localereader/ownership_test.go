package localereader

import("bytes";"errors";"io";"testing";"golang.org/x/text/transform")

type sourceAndError struct{input []byte;cause error;read bool}
func(r *sourceAndError)Read(buffer []byte)(int,error){if r.read{return 0,io.EOF};r.read=true;return copy(buffer,r.input),r.cause}
type partialTransformer struct{resets int;cause error}
func(t *partialTransformer)Reset(){t.resets++}
func(t *partialTransformer)Transform(dst,src []byte,_ bool)(int,int,error){if len(src)==0{return 0,0,nil};if len(dst)==0{return 0,0,transform.ErrShortDst};dst[0]=src[0];if len(src)>1{return 1,1,transform.ErrShortDst};return 1,1,t.cause}
// UNRUN comparison of observable buffering/error precedence with exact selected
// transform.Reader. This is not native or loaded-byte qualification.
func TestOwnedTransformReaderPreservesPartialOutputAndOriginalErrorPrecedence(t *testing.T){
 sourceFailure:=errors.New("actual source failure");transformFailure:=errors.New("actual transform failure")
 source:=&sourceAndError{input:[]byte("abc"),cause:sourceFailure};decoder:=&partialTransformer{cause:transformFailure};ownership:=&Ownership{Source:source};owned:=newOwnedTransformReader(source,decoder,ownership)
 referenceDecoder:=&partialTransformer{cause:transformFailure};reference:=transform.NewReader(&sourceAndError{input:[]byte("abc"),cause:sourceFailure},referenceDecoder)
 actual,actualErr:=io.ReadAll(owned);expected,expectedErr:=io.ReadAll(reference)
 if !bytes.Equal(actual,expected)||string(actual)!="abc"||actualErr!=sourceFailure||expectedErr!=sourceFailure||decoder.resets!=1||referenceDecoder.resets!=1{t.Fatal("buffer/partial/error precedence/reset changed")}
 sourceHeld,transformHeld:=false,false;ownership.VisitOriginals(func(operation *OriginalOperation){if operation.Source=="transform.source-read"{sourceHeld=sourceHeld||operation.Failure==sourceFailure};if operation.Source=="transform.invoke"{transformHeld=transformHeld||operation.Failure==transformFailure}})
 if !sourceHeld||!transformHeld||ownership.Reader!=owned||ownership.Decoder!=decoder{t.Fatal("independent raw failures or SAME reader/decoder lost")}
}
type splitSource struct{parts [][]byte;next int}
func(r *splitSource)Read(buffer []byte)(int,error){if r.next==len(r.parts){return 0,io.EOF};part:=r.parts[r.next];r.next++;return copy(buffer,part),nil}
type pairedTransformer struct{transform.NopResetter}
func(pairedTransformer)Transform(dst,src []byte,atEOF bool)(int,int,error){if len(src)<2&&!atEOF{return 0,0,transform.ErrShortSrc};count:=copy(dst,src);if count<len(src){return count,count,transform.ErrShortDst};return count,count,nil}
func TestOwnedTransformReaderPreservesShortSourceCompactionAndEOF(t *testing.T){
 source:=&splitSource{parts:[][]byte{[]byte("a"),[]byte("b")}};ownership:=&Ownership{Source:source};owned:=newOwnedTransformReader(source,pairedTransformer{},ownership)
 actual,actualErr:=io.ReadAll(owned);reference:=transform.NewReader(&splitSource{parts:[][]byte{[]byte("a"),[]byte("b")}},pairedTransformer{});expected,expectedErr:=io.ReadAll(reference)
 if !bytes.Equal(actual,expected)||string(actual)!="ab"||actualErr!=expectedErr{t.Fatal("short source/EOF buffering changed")}
}
func TestOwnedTransformReaderReturnsPartialBytesWithSameEOF(t *testing.T){source:=&sourceAndError{input:[]byte("abc"),cause:io.EOF};ownership:=&Ownership{Source:source};owned:=newOwnedTransformReader(source,transform.Nop,ownership);buffer:=make([]byte,4);count,err:=owned.Read(buffer);if count!=3||string(buffer[:count])!="abc"||err!=io.EOF{t.Fatal("n>0 plus EOF projection changed")};held:=false;ownership.VisitOriginals(func(operation *OriginalOperation){if operation.Source=="transform.source-read"{original:=operation.Input.(*readOriginal);held=held||original.Count==3&&original.Failure==io.EOF}});if !held{t.Fatal("actual partial EOF original lost")}}
type invalidCountInput struct{cause error}
func(r invalidCountInput)Read(buffer []byte)(int,error){return len(buffer)+1,r.cause}
func TestOwnedTransformReaderInvalidCountRetainsRawOriginal(t *testing.T){cause:=errors.New("actual invalid-count source error");source:=invalidCountInput{cause};ownership:=&Ownership{Source:source};reader:=newOwnedTransformReader(source,transform.Nop,ownership);_,err:=reader.Read(make([]byte,1));failure,ok:=err.(*invalidReadExtent);if !ok||failure.Original.Count!=4097||failure.Original.Failure!=cause||!errors.Is(err,cause){t.Fatal("raw source extent/count/cause lost")}}
type invalidCountTransformer struct{transform.NopResetter;cause error}
func(t invalidCountTransformer)Transform(dst,src []byte,_ bool)(int,int,error){return len(dst)+1,0,t.cause}
func TestOwnedTransformReaderInvalidTransformCountRetainsRawOriginal(t *testing.T){cause:=errors.New("actual invalid transform count error");source:=bytes.NewReader([]byte("a"));ownership:=&Ownership{Source:source};reader:=newOwnedTransformReader(source,invalidCountTransformer{cause:cause},ownership);_,err:=reader.Read(make([]byte,1));failure,ok:=err.(*invalidTransformExtent);if !ok||failure.Original.DestinationCount!=4097||failure.Original.Failure!=cause||!errors.Is(err,cause){t.Fatal("raw transform extent/count/cause lost")}}