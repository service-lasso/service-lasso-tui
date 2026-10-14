package cancelreader

import("errors";"testing";"io")

type originalFailingInput struct{cause error;ownership **Ownership;seenBefore bool;buffer []byte}
func(r *originalFailingInput)Read(buffer []byte)(int,error){r.seenBefore=*r.ownership!=nil&&(*r.ownership).Partial!=nil;r.buffer=buffer;return 0,r.cause}
func TestOwnedFallbackAttachesBeforeInputAndRetainsSameOriginal(t *testing.T){
 var owned *Ownership;cause:=errors.New("actual private fallback read failure");input:=&originalFailingInput{cause:cause,ownership:&owned}
 reader,err:=NewReaderOwned(input,func(value *Ownership){owned=value});if err!=nil{t.Fatal(err)};buffer:=make([]byte,4)
 if count,err:=reader.Read(buffer);count!=0||err!=cause{t.Fatal("actual input result replaced")}
 if !input.seenBefore||owned.Reader!=input||owned.Partial!=reader||owned.Result!=reader{t.Fatal("constructor/input source not actually held before Read")}
 found:=false;owned.VisitOriginals(func(operation *OriginalOperation){if operation.Source=="fallback-input.read"{actual:=operation.Input.([]byte);found=operation.Subject==input&&operation.Failure==cause && &actual[0]==&buffer[0]}});if !found{t.Fatal("same original input/buffer/failure lost")}
}
var _ io.Reader=(*originalFailingInput)(nil)
