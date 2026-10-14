package app

import("bytes";"encoding/json";"io";"reflect";"strconv";"strings";"unicode/utf8")

func recordArbiterShape(value any,typ reflect.Type)bool{ok,_:=recordArbiterShapeCause(value,typ);return ok}
func recordArbiterShapeCause(value any,typ reflect.Type)(bool,error){
 switch typ.Kind(){case reflect.Struct:object,ok:=value.(map[string]any);if !ok{return false,nil};expected:=0;for i:=0;i<typ.NumField();i++{field:=typ.Field(i);if field.PkgPath!=""{continue};name:=strings.Split(field.Tag.Get("json"),",")[0];if name=="-"{continue};if name==""{name=field.Name};expected++;child,present:=object[name];if !present{return false,nil};admitted,err:=recordArbiterShapeCause(child,field.Type);if err!=nil{return false,err};if !admitted{return false,nil}};return len(object)==expected,nil
 case reflect.Slice:if value==nil{return true,nil};if typ.Elem().Kind()==reflect.Uint8{_,ok:=value.(string);return ok,nil};array,ok:=value.([]any);if !ok{return false,nil};for _,child:=range array{admitted,err:=recordArbiterShapeCause(child,typ.Elem());if err!=nil{return false,err};if !admitted{return false,nil}};return true,nil
 case reflect.String:_,ok:=value.(string);return ok,nil
 case reflect.Bool:_,ok:=value.(bool);return ok,nil
 case reflect.Int,reflect.Int32,reflect.Int64:number,ok:=value.(json.Number);if !ok{return false,nil};_,err:=strconv.ParseInt(string(number),10,typ.Bits());return err==nil,err
 case reflect.Uint,reflect.Uint32,reflect.Uint64:number,ok:=value.(json.Number);if !ok{return false,nil};_,err:=strconv.ParseUint(string(number),10,typ.Bits());return err==nil,err
 default:return false,nil
 }
}

// Private native identities are exact uint64/int64 facts. They must not pass
// through the public DTO's JavaScript-safe numeric range or rounded floats.
func recordArbiterJSON(raw []byte)(any,error){
 if len(raw)==0||len(raw)>recordArbiterSlotBytes-recordArbiterHeaderBytes||!utf8.Valid(raw){return nil,storageError()}
 for i:=0;i<len(raw);i++{if raw[i]!='"'{continue};i++;for ;i<len(raw)&&raw[i]!='"';i++{if raw[i]!='\\'{continue};i++;if i>=len(raw){return nil,storageError()};if raw[i]!='u'{continue};if i+4>=len(raw){return nil,storageError()};scalar,err:=strconv.ParseUint(string(raw[i+1:i+5]),16,16);if err!=nil{return nil,storageErrorCause(err)};i+=4;if scalar>=0xdc00&&scalar<=0xdfff{return nil,storageError()};if scalar>=0xd800&&scalar<=0xdbff{if i+6>=len(raw)||raw[i+1]!='\\'||raw[i+2]!='u'{return nil,storageError()};low,err:=strconv.ParseUint(string(raw[i+3:i+7]),16,16);if err!=nil{return nil,storageErrorCause(err)};if low<0xdc00||low>0xdfff{return nil,storageError()};i+=6}}}
 decoder:=json.NewDecoder(bytes.NewReader(raw));decoder.UseNumber();values:=0
 var read func(int)(any,error);read=func(depth int)(any,error){if depth>16||values>=8192{return nil,storageError()};values++;token,err:=decoder.Token();if err!=nil{return nil,err};switch value:=token.(type){
 case json.Delim:
  switch value{case '{':object:=map[string]any{};for decoder.More(){if len(object)>=128{return nil,storageError()};keyToken,err:=decoder.Token();if err!=nil{return nil,err};key,ok:=keyToken.(string);if !ok||len(key)>128{return nil,storageError()};if _,duplicate:=object[key];duplicate{return nil,storageError()};child,err:=read(depth+1);if err!=nil{return nil,err};object[key]=child};end,err:=decoder.Token();if err!=nil{return nil,storageErrorCause(err)};if end!=json.Delim('}'){return nil,storageError()};return object,nil
  case '[':array:=[]any{};for decoder.More(){if len(array)>=128{return nil,storageError()};child,err:=read(depth+1);if err!=nil{return nil,err};array=append(array,child)};end,err:=decoder.Token();if err!=nil{return nil,storageErrorCause(err)};if end!=json.Delim(']'){return nil,storageError()};return array,nil};return nil,storageError()
 case json.Number:spelling:=string(value);if strings.ContainsAny(spelling,".eE+"){return nil,storageError()};if strings.HasPrefix(spelling,"-"){if _,err:=strconv.ParseInt(spelling,10,64);err!=nil{return nil,err}}else if _,err:=strconv.ParseUint(spelling,10,64);err!=nil{return nil,err};return value,nil
 case string:if len(value)>16384{return nil,storageError()};return value,nil
 case bool,nil:return value,nil
 default:return nil,storageError()
 }}
 value,err:=read(0);if err!=nil{return nil,err};if _,err:=decoder.Token();err!=io.EOF{if err!=nil{return nil,storageErrorCause(err)};return nil,storageError()};return value,nil
}
