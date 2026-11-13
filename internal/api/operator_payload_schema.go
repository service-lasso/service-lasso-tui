package api

import (
 "encoding/json"
 "errors"
 "math"
 "strings"
 "unicode/utf8"
)

// This tree retains every source-declared variant; null is distinct from
// optional property absence. It is never inferred from a payload value.
type OperatorPayloadSchema struct {
 Types []string `json:"types"`
 Constraints OperatorPayloadConstraints `json:"constraints"`
 Properties []OperatorPayloadProperty `json:"properties"`
 Required []string `json:"required"`
 Items *OperatorPayloadSchema `json:"items"`
}
type OperatorPayloadProperty struct{Name string `json:"name"`;Schema OperatorPayloadSchema `json:"schema"`}
type OperatorPayloadConstraints struct{
 Enum []json.RawMessage `json:"enum"`
 Minimum *float64 `json:"minimum"`;Maximum *float64 `json:"maximum"`
 MinLength *int `json:"minLength"`;MaxLength *int `json:"maxLength"`
 MinItems *int `json:"minItems"`;MaxItems *int `json:"maxItems"`
 AdditionalProperties *bool `json:"additionalProperties"`
}
var operatorPayloadTypes=[]string{"string","boolean","integer","number","object","array","null"}
func(s OperatorPayloadSchema)Has(kind string)bool{for _,value:=range s.Types{if value==kind{return true}};return false}
func(s OperatorPayloadSchema)ValidRoot()bool{ok,_:=s.ValidRootCause();return ok}
func(s OperatorPayloadSchema)ValidRootCause()(bool,error){if len(s.Types)!=1||s.Types[0]!="object"{return false,nil};count:=0;return s.validCause(1,&count)}
func(s OperatorPayloadSchema)valid(depth int,count *int)bool{ok,_:=s.validCause(depth,count);return ok}
func(s OperatorPayloadSchema)validCause(depth int,count *int)(bool,error){
 if depth>8||len(s.Types)==0||len(s.Types)>len(operatorPayloadTypes)||s.Properties==nil||s.Required==nil{return false,nil}
 previous:=-1;for _,kind:=range s.Types{index:=-1;for i,value:=range operatorPayloadTypes{if value==kind{index=i;break}};if index<=previous{return false,nil};previous=index}
 c:=s.Constraints
 if s.Has("string"){if c.MinLength==nil||c.MaxLength==nil||*c.MinLength<0||*c.MaxLength<*c.MinLength||float64(*c.MaxLength)>9007199254740991{return false,nil}}else if c.MinLength!=nil||c.MaxLength!=nil{return false,nil}
 if s.Has("integer")||s.Has("number"){if c.Minimum==nil||c.Maximum==nil||!operatorPayloadNumber(*c.Minimum)||!operatorPayloadNumber(*c.Maximum)||*c.Minimum>*c.Maximum{return false,nil};if !s.Has("number")&&(math.Trunc(*c.Minimum)!=*c.Minimum||math.Trunc(*c.Maximum)!=*c.Maximum){return false,nil}}else if c.Minimum!=nil||c.Maximum!=nil{return false,nil}
 if s.Has("array"){if s.Items==nil||c.MinItems==nil||c.MaxItems==nil||*c.MinItems<0||*c.MaxItems<*c.MinItems||*c.MaxItems>128{return false,nil};ok,err:=s.Items.validCause(depth+1,count);if err!=nil{return false,err};if !ok{return false,nil}}else if s.Items!=nil||c.MinItems!=nil||c.MaxItems!=nil{return false,nil}
 if s.Has("object"){
  if c.AdditionalProperties==nil||*c.AdditionalProperties{return false,nil};last:="";names:=map[string]bool{}
  for _,property:=range s.Properties{*count++;if *count>64||!operatorDisplay(property.Name,128)||property.Name==""||reservedOperatorKey(property.Name)||property.Name<=last{return false,nil};ok,err:=property.Schema.validCause(depth+1,count);if err!=nil{return false,err};if !ok{return false,nil};last=property.Name;names[property.Name]=true}
  last="";for _,name:=range s.Required{if !names[name]||name<=last{return false,nil};last=name}
 }else if len(s.Properties)!=0||len(s.Required)!=0||c.AdditionalProperties!=nil{return false,nil}
 if c.Enum!=nil{if len(c.Enum)==0||len(c.Enum)>32{return false,nil};values:=[]any{};for _,raw:=range c.Enum{value,err:=operatorJSON(raw,OperatorRequestBytes);if err!=nil{return false,err};ok,err:=s.valueValidCause(value,false);if err!=nil{return false,err};if !ok{return false,nil};for _,prior:=range values{equal,err:=operatorPayloadEqualCause(prior,value);if err!=nil{return false,err};if equal{return false,nil}};values=append(values,value)}}
 return true,nil
}
func operatorPayloadNumber(value float64)bool{return !math.IsNaN(value)&&!math.IsInf(value,0)&&math.Abs(value)<=9007199254740991}
func operatorPayloadEqual(a,b any)bool{ok,_:=operatorPayloadEqualCause(a,b);return ok}
func operatorPayloadEqualCause(a,b any)(bool,error){
 an,aok:=a.(json.Number);bn,bok:=b.(json.Number);if aok||bok{if !aok||!bok{return false,nil};af,ae:=an.Float64();bf,be:=bn.Float64();if ae!=nil||be!=nil{return false,errors.Join(ae,be)};return af==bf,nil}
 switch av:=a.(type){
 case nil:return b==nil,nil
 case bool:bv,ok:=b.(bool);return ok&&av==bv,nil
 case string:bv,ok:=b.(string);return ok&&av==bv,nil
 case []any:bv,ok:=b.([]any);if !ok||len(av)!=len(bv){return false,nil};for i,value:=range av{equal,err:=operatorPayloadEqualCause(value,bv[i]);if err!=nil{return false,err};if !equal{return false,nil}};return true,nil
 case map[string]any:bv,ok:=b.(map[string]any);if !ok||len(av)!=len(bv){return false,nil};for key,value:=range av{other,exists:=bv[key];if !exists{return false,nil};equal,err:=operatorPayloadEqualCause(value,other);if err!=nil{return false,err};if !equal{return false,nil}};return true,nil
 };return false,nil
}
func(s OperatorPayloadSchema)ValueValid(raw []byte)bool{ok,_:=s.ValueValidCause(raw);return ok}
func(s OperatorPayloadSchema)ValueValidCause(raw []byte)(bool,error){value,err:=operatorJSON(raw,OperatorRequestBytes);if err!=nil{return false,err};return s.valueValidCause(value,true)}
// Only the root overlay may omit required properties and its structural enum.
// Every supplied subtree is a complete replacement with its original schema.
// The server validates the final reference-first shallow merge in full.
func(s OperatorPayloadSchema)OverlayValid(raw []byte)bool{
 ok,_:=s.OverlayValidCause(raw);return ok
}
func(s OperatorPayloadSchema)OverlayValidCause(raw []byte)(bool,error){
 rootOK,err:=s.ValidRootCause();if err!=nil{return false,err};if !rootOK{return false,nil};value,err:=operatorJSON(raw,OperatorRequestBytes);if err!=nil{return false,err};object,ok:=value.(map[string]any);if !ok{return false,nil}
 properties:=map[string]OperatorPayloadSchema{};for _,property:=range s.Properties{properties[property.Name]=property.Schema}
 for name,child:=range object{schema,exists:=properties[name];if !exists{return false,nil};ok,err:=schema.valueValidCause(child,true);if err!=nil{return false,err};if !ok{return false,nil}}
 return true,nil
}
func(s OperatorPayloadSchema)valueValid(value any,enum bool)bool{ok,_:=s.valueValidCause(value,enum);return ok}
func(s OperatorPayloadSchema)valueValidCause(value any,enum bool)(bool,error){
 admitted:=false;c:=s.Constraints
 switch v:=value.(type){
 case nil:admitted=s.Has("null")
 case bool:admitted=s.Has("boolean")
 case string:admitted=s.Has("string")&&utf8.ValidString(v)&&len(v)<=4096&&c.MinLength!=nil&&c.MaxLength!=nil&&utf8.RuneCountInString(v)>=*c.MinLength&&utf8.RuneCountInString(v)<=*c.MaxLength
 case json.Number:n,err:=v.Float64();if err!=nil{return false,err};admitted=operatorPayloadNumber(n)&&(s.Has("number")||s.Has("integer")&&math.Trunc(n)==n)&&c.Minimum!=nil&&c.Maximum!=nil&&n>=*c.Minimum&&n<=*c.Maximum
 case []any:admitted=s.Has("array")&&s.Items!=nil&&c.MinItems!=nil&&c.MaxItems!=nil&&len(v)>=*c.MinItems&&len(v)<=*c.MaxItems;if admitted{for _,child:=range v{ok,err:=s.Items.valueValidCause(child,true);if err!=nil{return false,err};if !ok{return false,nil}}}
 case map[string]any:
  admitted=s.Has("object");if admitted{properties:=map[string]OperatorPayloadSchema{};for _,property:=range s.Properties{properties[property.Name]=property.Schema};for _,name:=range s.Required{if _,ok:=v[name];!ok{return false,nil}};for name,child:=range v{schema,ok:=properties[name];if !ok{return false,nil};ok,err:=schema.valueValidCause(child,true);if err!=nil{return false,err};if !ok{return false,nil}}}
 }
 if !admitted{return false,nil};if enum&&c.Enum!=nil{for _,raw:=range c.Enum{choice,err:=operatorJSON(raw,OperatorRequestBytes);if err!=nil{return false,err};equal,err:=operatorPayloadEqualCause(choice,value);if err!=nil{return false,err};if equal{return true,nil}};return false,nil};return true,nil
}
func operatorPayloadPointer(parent,name string)string{return parent+"/"+strings.ReplaceAll(strings.ReplaceAll(name,"~","~0"),"/","~1")}
