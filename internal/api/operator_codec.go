package api

import (
	"bytes"
	"encoding/json"
	"io"
	"math"
	"reflect"
	"regexp"
	"strconv"
	"strings"
	"unicode"
	"unicode/utf8"
)

const OperatorContract = "service-lasso-operator.v2"
const OperatorRequestBytes = 65536
const OperatorResponseBytes = 262144
const OperatorErrorBytes = 8192
const OperatorContextBytes = 4096

// V2 parsing is independent of the authentic v1 compatibility decoder. It
// checks original bytes before encoding/json can replace invalid surrogates.
func operatorJSON(raw []byte, limit int) (any, error) {
	value,err:=operatorJSONCandidate(raw,limit);if err!=nil{return nil,err};if !operatorPublicStrings(value,4096){return nil,&OperatorError{Stage:"response"}};return value,nil
}
func operatorJSONCandidate(raw []byte, limit int) (any, error) {
 bad:=&OperatorError{Stage:"response"}
 if len(raw)==0||len(raw)>limit||!utf8.Valid(raw)||bytes.HasPrefix(raw,[]byte{0xef,0xbb,0xbf}){return nil,bad}
 valid,err:=validJSONSurrogates(raw);if !valid{return nil,&OperatorError{Stage:"response",cause:err}}
 d:=json.NewDecoder(bytes.NewReader(raw));d.UseNumber();values:=0
 value,ok,err:=operatorValue(d,0,&values);if !ok{return nil,&OperatorError{Stage:"response",cause:err}}
 if _,err:=d.Token();err!=io.EOF{return nil,&OperatorError{Stage:"response",cause:err}}
 return value,nil
}
func operatorValue(d *json.Decoder,depth int,count *int)(any,bool,error){return operatorValueStringLimit(d,depth,count,32768)}
func operatorValueStringLimit(d *json.Decoder,depth int,count *int,stringLimit int)(any,bool,error){
 *count++
 if depth>16||*count>8192{return nil,false,nil}
 t,err:=d.Token();if err!=nil{return nil,false,err}
 switch value:=t.(type){
 case json.Delim:
  if value=='{'{
   result:=map[string]any{}
   for d.More(){
    keyToken,err:=d.Token();if err!=nil{return nil,false,err};key,ok:=keyToken.(string)
    if !ok||len(key)==0||len(key)>128||reservedOperatorKey(key){return nil,false,nil}
    if _,exists:=result[key];exists{return nil,false,nil}
    child,ok,err:=operatorValue(d,depth+1,count);if !ok{return nil,false,err};result[key]=child
   }
   end,err:=d.Token();if err!=nil{return nil,false,err};return result,end==json.Delim('}'),nil
  }
  if value=='['{
   result:=[]any{}
   for d.More(){child,ok,err:=operatorValue(d,depth+1,count);if !ok{return nil,false,err};result=append(result,child)}
   end,err:=d.Token();if err!=nil{return nil,false,err};return result,end==json.Delim(']'),nil
  }
  return nil,false,nil
 case string:return value,len(value)<=stringLimit,nil
 case json.Number:
  n,err:=strconv.ParseFloat(string(value),64);if err!=nil{return nil,false,err}
  safe,err:=operatorSafeMagnitude(string(value));if err!=nil{return nil,false,err};return value,!math.IsNaN(n)&&!math.IsInf(n,0)&&safe,nil
 case bool,nil:return value,true,nil
 default:return nil,false,nil
 }
}

// Compare the coefficient before binary floating-point rounding. The JSON lexer
// has already checked the spelling; a rounded maximum-safe value is insufficient.
func operatorSafeMagnitude(spelling string)(bool,error){
	spelling = strings.TrimPrefix(spelling, "-")
	exponent := 0.0
	if at := strings.IndexAny(spelling, "eE"); at >= 0 {
		parsed,err:=strconv.ParseFloat(spelling[at+1:],64);if err!=nil{return false,err};exponent=parsed
		spelling = spelling[:at]
	}
	if at := strings.IndexByte(spelling, '.'); at >= 0 {
		exponent -= float64(len(spelling)-at-1)
		spelling = spelling[:at] + spelling[at+1:]
	}
	coefficient := strings.TrimLeft(spelling, "0")
	if coefficient == "" { return true,nil }
	digits := float64(len(coefficient)) + exponent
	if digits < 16 { return true,nil }; if digits > 16 { return false,nil }
	integer := coefficient
	if len(integer) > 16 { integer = integer[:16] }
	integer += strings.Repeat("0", 16-len(integer))
	if integer != "9007199254740991" { return integer < "9007199254740991",nil }
	if len(coefficient) <= 16 { return true,nil }
	return strings.Trim(coefficient[16:], "0") == "",nil
}

func reservedOperatorKey(key string) bool { return key == "__proto__" || key == "prototype" || key == "constructor" }

func validJSONSurrogates(raw []byte)(bool,error){
	quoted := false
	for i := 0; i < len(raw); i++ {
		if raw[i] == '"' { quoted = !quoted; continue }
		if !quoted || raw[i] != '\\' { continue }
		i++; if i >= len(raw) { return false,nil }
		if raw[i] != 'u' { continue }
		if i+4 >= len(raw) { return false,nil }
		n, err := strconv.ParseUint(string(raw[i+1:i+5]), 16, 16); if err != nil { return false,err }; i += 4
		if n >= 0xdc00 && n <= 0xdfff { return false,nil }
		if n >= 0xd800 && n <= 0xdbff {
			if i+6 >= len(raw) || raw[i+1] != '\\' || raw[i+2] != 'u' { return false,nil }
			low, err := strconv.ParseUint(string(raw[i+3:i+7]), 16, 16)
			if err != nil { return false,err }; if low < 0xdc00 || low > 0xdfff { return false,nil }; i += 6
		}
	}
	return !quoted,nil
}

// Every struct member is required unless explicitly tagged omitempty. This
// also rejects case aliases accepted by encoding/json and closes nested DTOs.
func operatorShape(value any, typ reflect.Type) bool {ok,_:=operatorShapeCause(value,typ);return ok}
func operatorShapeCause(value any, typ reflect.Type)(bool,error) {
	if typ == reflect.TypeOf(json.RawMessage{}) { return true,nil }
 if typ == reflect.TypeOf(OperatorPayloadConstraintMap{}) { return operatorPayloadConstraintShapeCause(value) }
	if typ.Kind() == reflect.Pointer { if value == nil { return true,nil }; return operatorShapeCause(value, typ.Elem()) }
	switch typ.Kind() {
	case reflect.Struct:
		object, ok := value.(map[string]any); if !ok { return false,nil }
		fields := map[string]reflect.StructField{}
		for i := 0; i < typ.NumField(); i++ {
			field := typ.Field(i); tag := strings.Split(field.Tag.Get("json"), ",")[0]
			if tag == "-" || field.PkgPath != "" { continue }; if tag == "" { return false,nil }; fields[tag] = field
			child, exists := object[tag]
			if !exists { if !strings.Contains(field.Tag.Get("json"), ",omitempty") { return false,nil }; continue }
			if child==nil&&field.Type.Kind()==reflect.Pointer&&strings.Contains(field.Tag.Get("json"),",omitempty"){return false,nil}
			admitted,err:=operatorShapeCause(child,field.Type);if err!=nil{return false,err};if !admitted { return false,nil }
		}
		for key := range object { if _, exists := fields[key]; !exists { return false,nil } }; return true,nil
	case reflect.Slice, reflect.Array:
		array, ok := value.([]any); if !ok { return false,nil }; if typ.Kind() == reflect.Array && len(array) != typ.Len() { return false,nil }
		for _, child := range array { admitted,err:=operatorShapeCause(child,typ.Elem());if err!=nil{return false,err};if !admitted { return false,nil } }; return true,nil
	case reflect.String:
		_, ok := value.(string); return ok,nil
	case reflect.Bool:
		_, ok := value.(bool); return ok,nil
	case reflect.Int, reflect.Int64, reflect.Int32:
		n, ok := value.(json.Number); if !ok { return false,nil }; number, err := strconv.ParseFloat(string(n), 64)
		if err!=nil{return false,err};return math.Trunc(number) == number && math.Abs(number) <= 9007199254740991 && (typ.Kind() != reflect.Int32 || number >= -2147483648 && number <= 2147483647),nil
	case reflect.Float32, reflect.Float64:
		_, ok := value.(json.Number); return ok,nil
	default:
		return false,nil
	}
}

func decodeOperator(raw []byte, destination any, limit int) error {
	value, err := operatorJSONCandidate(raw, limit); if err != nil { return err }
	typ := reflect.TypeOf(destination)
	if typ == nil || typ.Kind() != reflect.Pointer { return &OperatorError{Stage: "response"} }
	shapeOK,shapeErr:=operatorShapeCause(value,typ.Elem());if shapeErr!=nil{return &OperatorError{Stage:"response",cause:shapeErr}};if !shapeOK||!operatorTypedStrings(value,typ.Elem()){return &OperatorError{Stage:"response"}}
	// Core accepts integer-valued exponent and decimal spellings. Normalize only
	// DTO integer members after the original spelling and closed shape pass.
	value,err = operatorIntegerSpellingsCause(value, typ.Elem());if err!=nil{return &OperatorError{Stage:"response",cause:err}}
	normalized, err := json.Marshal(value)
	if err != nil { return &OperatorError{Stage: "response",cause:err} }; if err:=json.Unmarshal(normalized,destination);err!=nil{return &OperatorError{Stage:"response",cause:err}}; return nil
}
func operatorPublicStrings(value any,limit int)bool{
	switch v:=value.(type){case string:return len(v)<=limit;case map[string]any:for _,child:=range v{if !operatorPublicStrings(child,limit){return false}};case []any:for _,child:=range v{if !operatorPublicStrings(child,limit){return false}}};return true
}
func operatorFileEntryStrings(value any)bool{
	entry,ok:=value.(map[string]any);if !ok{return value==nil};if len(entry)!=2{return false}
	target,targetOK:=entry["targetHandle"].(string);content,contentOK:=entry["content"].(string)
	return targetOK&&OperatorID(target)&&contentOK&&operatorFileContent(content,32768)
}
func operatorFileContent(value string,limit int)bool{if !utf8.ValidString(value)||len(value)>limit{return false};for _,r:=range value{if unicode.IsControl(r)&&r!='\t'&&r!='\n'&&r!='\r'{return false}};return true}
func operatorTypedStrings(value any,typ reflect.Type)bool{
	return operatorTypedStringsIn(value,typ,false)
}
func operatorTypedStringsIn(value any,typ reflect.Type,classified bool)bool{
	if value==nil{return true};for typ.Kind()==reflect.Pointer{typ=typ.Elem()}
	if typ==reflect.TypeOf(json.RawMessage{}){return operatorPublicStrings(value,4096)}
	switch typ.Kind(){
	case reflect.Struct:
		object:=value.(map[string]any)
		if typ==reflect.TypeOf(OperatorConfigForm{}){classified=true}
		for i:=0;i<typ.NumField();i++{field:=typ.Field(i);key:=strings.Split(field.Tag.Get("json"),",")[0];child,exists:=object[key];if !exists{continue}
			if classified&&typ==reflect.TypeOf(OperatorField{})&&key=="value"&&object["type"]=="file_entry"{if !operatorFileEntryStrings(child){return false};continue}
			if classified&&typ==reflect.TypeOf(OperatorCatalog{})&&key=="entries"&&object["type"]=="file_entry"{for _,entry:=range child.([]any){for member,item:=range entry.(map[string]any){if member=="value"{if !operatorFileEntryStrings(item){return false}}else if !operatorPublicStrings(item,4096){return false}}};continue}
			if !operatorTypedStringsIn(child,field.Type,classified){return false}
		};return true
	case reflect.Slice,reflect.Array:for _,child:=range value.([]any){if !operatorTypedStringsIn(child,typ.Elem(),classified){return false}};return true
	};return operatorPublicStrings(value,4096)
}
func operatorFieldJSON(kind string,raw []byte)(any,error){
	if kind!="file_entry"{return operatorJSON(raw,OperatorRequestBytes)}
	value,err:=operatorJSONCandidate(raw,OperatorRequestBytes);if err!=nil{return nil,err};if !operatorFileEntryStrings(value){return nil,&OperatorError{Stage:"response"}};return value,nil
}

func operatorIntegerSpellings(value any, typ reflect.Type) any {normalized,_:=operatorIntegerSpellingsCause(value,typ);return normalized}
func operatorIntegerSpellingsCause(value any, typ reflect.Type)(any,error) {
	if value == nil { return nil,nil }
	for typ.Kind() == reflect.Pointer { typ = typ.Elem() }
	if typ == reflect.TypeOf(json.RawMessage{}) { return value,nil }
	switch typ.Kind() {
	case reflect.Int, reflect.Int32, reflect.Int64:
		n,err := strconv.ParseFloat(string(value.(json.Number)), 64);if err!=nil{return nil,err}
		return json.Number(strconv.FormatFloat(n, 'f', 0, 64)),nil
	case reflect.Struct:
		object := value.(map[string]any)
		for i := 0; i < typ.NumField(); i++ {
			field := typ.Field(i); name := strings.Split(field.Tag.Get("json"), ",")[0]
			if child, exists := object[name]; exists { normalized,err:=operatorIntegerSpellingsCause(child,field.Type);if err!=nil{return nil,err};object[name]=normalized }
		}; return object,nil
	case reflect.Slice, reflect.Array:
		array := value.([]any); for i, child := range array { normalized,err:=operatorIntegerSpellingsCause(child,typ.Elem());if err!=nil{return nil,err};array[i]=normalized }; return array,nil
	}
	return value,nil
}

func OperatorID(value string) bool {
	if len(value) == 0 || len(value) > 128 || !utf8.ValidString(value) { return false }
	for _, r := range value { if unicode.IsControl(r) { return false } }; return true
}
var operatorServiceGrammar=regexp.MustCompile(`^@?[A-Za-z0-9][A-Za-z0-9._-]*$`)
var operatorReservedService=regexp.MustCompile(`(?i)^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(\.|$)`)
func OperatorServiceID(value string)bool{return OperatorID(value)&&operatorServiceGrammar.MatchString(value)&&!strings.HasSuffix(value,".")&&!operatorReservedService.MatchString(strings.TrimPrefix(value,"@"))}
func operatorServiceIDs(values []string,max int)bool{if values==nil||len(values)>max{return false};seen:=map[string]bool{};for _,id:=range values{if !OperatorServiceID(id)||seen[id]{return false};seen[id]=true};return true}

func operatorDisplay(value string, max int) bool {
	if len(value) > max || !utf8.ValidString(value) { return false }
	for _, r := range value { if unicode.IsControl(r) { return false } }; return true
}

func operatorEnum(value string, values ...string) bool { for _, candidate := range values { if value == candidate { return true } }; return false }
func operatorIDs(values []string, max int) bool {
	if values == nil || len(values) > max { return false }; seen := map[string]bool{}
	for _, value := range values { if !OperatorID(value) || seen[value] { return false }; seen[value] = true }; return true
}
