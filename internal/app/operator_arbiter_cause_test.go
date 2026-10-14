package app

import (
 "encoding/json"
 "errors"
 "io"
 "reflect"
 "strconv"
 "testing"
)

// Source-only cases; native journal ownership and qualification are separate.
func TestArbiterDecodeRetainsActualTokenAndNumericFailures(t *testing.T){
 for _,raw:=range []string{`{"a":1`, `[1`, `{"a":1} @`}{
  value,err:=recordArbiterJSON([]byte(raw));if value!=nil||err==nil{t.Fatal("failed token must zero decoded value")}
  var syntax *json.SyntaxError;if !errors.Is(err,io.EOF)&&!errors.As(err,&syntax){t.Fatal("actual token original lost")}
 }
 _,err:=recordArbiterJSON([]byte(`"\uZZZZ"`));var number *strconv.NumError;if !errors.As(err,&number){t.Fatal("actual unicode numeric parser original lost")}
 typ:=reflect.TypeOf(struct{Small uint32 `json:"small"`}{})
 admitted,err:=recordArbiterShapeCause(map[string]any{"small":json.Number("4294967296")},typ)
 if admitted||!errors.As(err,&number){t.Fatal("nested exact numeric overflow original lost")}
 admitted,err=recordArbiterShapeCause(map[string]any{"small":"wrong kind"},typ)
 if admitted||err!=nil{t.Fatal("semantic kind refusal must not invent a numeric cause")}
}
