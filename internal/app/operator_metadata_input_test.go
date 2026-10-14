package app

import (
 "encoding/json"
 "errors"
 "strconv"
 "strings"
 "testing"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// Actual parser/private-wrapper source cases, UNRUN; no durable/native proof.
func TestOperatorMetadataInputPreservesActualParserErrorsPrivately(t *testing.T){
 for _,test:=range []struct{selector,text string;count int}{{"favorite","private-invalid-bool",1},{"position","private-x,private-y",2},{"position","999999999999999999999999999999999,0",1}}{
  favorite,position,err:=operatorMetadataPatchInput(test.selector,test.text);if err==nil||favorite!=nil||position!=nil{t.Fatal("failed input produced a patch")}
  var original *strconv.NumError;if !errors.As(err,&original)||original.Num==""||!strings.Contains(test.text,original.Num){t.Fatal("actual NumError discarded")}
  if !errors.Is(err,original)||!errors.Is(err,original.Err){t.Fatal("original parse cause chain discarded")}
  if test.count==2{joined,ok:=errors.Unwrap(err).(interface{Unwrap()[]error});if !ok||len(joined.Unwrap())!=2{t.Fatal("one coordinate failure discarded")};for _,cause:=range joined.Unwrap(){if !errors.Is(err,cause){t.Fatal("original coordinate object replaced")}}}
  encoded,marshalErr:=json.Marshal(err);if marshalErr!=nil||strings.Contains(string(encoded),original.Num)||strings.Contains(err.Error(),original.Num)||api.SafeError(err)!=api.OperatorMessage("invalid_request"){t.Fatal("private parser diagnostic entered presentation")}
 }
 for _,test:=range []struct{selector,text string}{{"position","1"},{"position","100001,0"},{"unknown","private-semantic"}}{_,_,err:=operatorMetadataPatchInput(test.selector,test.text);if err==nil||errors.Unwrap(err)!=nil||api.SafeError(err)!=api.OperatorMessage("invalid_request"){t.Fatal("semantic refusal fabricated an original parser error")}}
 favorite,position,err:=operatorMetadataPatchInput("favorite","false");if err!=nil||favorite==nil||*favorite||position!=nil{t.Fatal("explicit false lost")}
 favorite,position,err=operatorMetadataPatchInput("position","automatic");if err!=nil||favorite!=nil||position==nil||*position!=nil{t.Fatal("explicit nullable reset lost")}
}
