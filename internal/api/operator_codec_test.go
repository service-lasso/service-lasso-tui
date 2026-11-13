package api

import (
 "strings"
 "testing"
)

// These are unit contract cases, not packaged/native qualification evidence.
// Execution requires the separate whole-source review and parent admission.
func TestOperatorOriginalNumericSpelling(t *testing.T){
 cases:=[]struct{raw string;valid bool}{
  {"9007199254740991",true},{"9007199254740991.0",true},{"9007199254740991.1",false},
  {"-9007199254740991.1",false},{"90071992547409911e-1",false},{"90071992547409910e-1",true},
  {"9007199254740992",false},{"1e309",false},{"1e-309",true},
 }
 for _,test:=range cases{_,err:=operatorJSON([]byte(test.raw),OperatorResponseBytes);if (err==nil)!=test.valid{t.Errorf("original spelling %s admitted=%t",test.raw,err==nil)}}
}
func TestOperatorFatalWireCases(t *testing.T){
 cases:=[][]byte{[]byte(`{"x":1,"x":2}`),[]byte(`{"x":1,"\u0078":2}`),[]byte(`{"__proto__":1}`),[]byte(`{"x":"\ud800"}`),[]byte(`{"x":"\udc00"}`),append([]byte{0xef,0xbb,0xbf},[]byte(`{}`)...),[]byte{'"',0xff,'"'},[]byte(strings.Repeat("[",17)+"0"+strings.Repeat("]",17)),[]byte(`{} {}`)}
 for i,raw:=range cases{if _,err:=operatorJSON(raw,OperatorResponseBytes);err==nil{t.Errorf("invalid wire case %d admitted",i)}}
}
func TestOperatorIntegerValueGrammar(t *testing.T){
 for _,raw:=range []string{`{"n":1}`,`{"n":1.0}`,`{"n":1e0}`} {var value struct{N int `json:"n"`};if err:=decodeOperator([]byte(raw),&value,OperatorResponseBytes);err!=nil||value.N!=1{t.Errorf("integer-valued spelling rejected: %s",raw)}}
 for _,raw:=range []string{`{"n":1.1}`,`{"n":null}`,`{"n":"1"}`,`{"n":1,"extra":0}`,`{}`} {var value struct{N int `json:"n"`};if err:=decodeOperator([]byte(raw),&value,OperatorResponseBytes);err==nil{t.Errorf("invalid integer DTO admitted: %s",raw)}}
}
func TestOperatorCanonicalAndOpaqueIdentities(t *testing.T){
 for _,id:=range []string{"svc","@provider","service.name","a-b_1"}{if !OperatorServiceID(id){t.Errorf("canonical ID rejected %s",id)}}
 for _,id:=range []string{"CON","@nul","com1","LPT9","service.","a/b","日本語"}{if OperatorServiceID(id){t.Errorf("nonportable service ID admitted %s",id)}}
 if !OperatorID("opaque /?% handle"){t.Fatal("opaque handle was treated as a public path")}
 if OperatorID("opaque\x00handle"){t.Fatal("control-bearing opaque handle admitted")}
}
func TestOperatorFixedMillisecondTime(t *testing.T){
 for _,value:=range []string{"2026-10-08T00:00:00.000Z","2026-10-08T00:00:00.120Z"}{if !OperatorTime(value){t.Errorf("actual Core spelling rejected %s",value)}}
 for _,value:=range []string{"2026-10-08T00:00:00Z","2026-10-08T00:00:00.12Z","2026-02-30T00:00:00.000Z","2026-10-08T00:00:00.000+00:00"}{if OperatorTime(value){t.Errorf("noncanonical time admitted %s",value)}}
}
func TestOperatorFileContentBoundsAndClassification(t *testing.T){
 ready:=OperatorAvailability{Supported:true,Available:true}
 field:=OperatorField{Selector:"config.files[entry]",Type:"file_entry",Value:[]byte(`{"targetHandle":"issued","content":"a\n\tb"}`),Constraints:[]byte(`{"catalogRevision":"r","maxEntries":1}`),Availability:ready}
 if !operatorFieldValid(field){t.Fatal("legitimate multiline/tabbed file data rejected")}
 field.Constraints=[]byte(`{"catalogRevision":"r","maxEntries":1,"maxContentBytes":1}`);if operatorFieldValid(field){t.Fatal("declared content byte limit ignored")}
 field.Constraints=[]byte(`{"catalogRevision":"r","maxEntries":1}`)
 field.Value=[]byte(`{"targetHandle":"issued","content":"\u0007"}`);if operatorFieldValid(field){t.Fatal("file BEL admitted")}
 field.Value=[]byte(`{"targetHandle":"issued","content":{"content":"x"}}`);if operatorFieldValid(field){t.Fatal("nested content object received literal-content authority")}
 if operatorFileContent("a\x85b",32768){t.Fatal("C1 control admitted as file data")}
}
func TestOperatorClosedErrorPolicy(t *testing.T){
 for code,policy:=range operatorErrorPolicies{
  body:=operatorErrorBody{Code:code,MessageKey:"operator.error."+code,Retryability:policy.Retry}
  if !operatorErrorPolicyValid(body,policy.Status){t.Errorf("selected error table rejected %s",code)}
  if operatorErrorPolicyValid(body,200){t.Errorf("error with successful HTTP status admitted %s",code)}
  body.MessageKey=code;if operatorErrorPolicyValid(body,policy.Status){t.Errorf("unselected message key admitted %s",code)}
 }
 if operatorErrorSelectorAllowed("C:\\private\\file","/inbox",nil){t.Fatal("private error path admitted")}
 if !operatorErrorSelectorAllowed("cursor","/inbox?cursor=opaque",nil){t.Fatal("actual request cursor selector rejected")}
}
