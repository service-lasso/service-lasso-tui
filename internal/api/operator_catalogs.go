package api

import (
	"encoding/json"
	"reflect"
	"strconv"
	"strings"
)

func operatorCatalogKind(selector string)string{
	switch selector{case "config.files","install.files":return "file_entry";case "config.templates","install.templates":return "template_entry";case "files.roots":return "root_entry"};return ""
}
func operatorEntrySelector(collection,selector string)bool{
	prefix:=collection+"[";if !strings.HasPrefix(selector,prefix)||!strings.HasSuffix(selector,"]"){return false};return OperatorID(strings.TrimSuffix(strings.TrimPrefix(selector,prefix),"]"))
}
func operatorEquivalentJSON(left,right []byte,kind string)bool{
	l,err:=operatorFieldJSON(kind,left);if err!=nil{return false};r,err:=operatorFieldJSON(kind,right);if err!=nil{return false};return operatorSameJSON(l,r)
}
func operatorSameJSON(left,right any)bool{
	switch l:=left.(type){
	case json.Number:r,ok:=right.(json.Number);if !ok{return false};a,_:=strconv.ParseFloat(string(l),64);b,_:=strconv.ParseFloat(string(r),64);return a==b
	case map[string]any:r,ok:=right.(map[string]any);if !ok||len(l)!=len(r){return false};for key,value:=range l{other,exists:=r[key];if !exists||!operatorSameJSON(value,other){return false}};return true
	case []any:r,ok:=right.([]any);if !ok||len(l)!=len(r){return false};for i,value:=range l{if !operatorSameJSON(value,r[i]){return false}};return true
	};return reflect.DeepEqual(left,right)
}
func(f OperatorConfigForm)Valid()bool{
	if !OperatorServiceID(f.ServiceID)||!OperatorID(f.DefinitionRevision)||!OperatorID(f.ModelEpoch)||!f.Availability.Valid()||!operatorFieldsValid(f.Fields)||f.Catalogs==nil||len(f.Catalogs)>128{return false}
	fields:=map[string]OperatorField{};for _,field:=range f.Fields{fields[field.Selector]=field}
	seenCatalogs,seenEntries:=map[string]bool{},map[string]bool{}
	handles:=map[string]map[string]bool{"entry":{},"slot":{},"target":{},"source":{},"path":{}}
	addHandle:=func(kind,id string)bool{if !OperatorID(id){return false};for _,roster:=range handles{if roster[id]{return false}};handles[kind][id]=true;return true}
	entryCount,slotCount,targetCount,sourceCount:=0,0,0,0
	for _,catalog:=range f.Catalogs{
		if operatorCatalogKind(catalog.Selector)!=catalog.Type||seenCatalogs[catalog.Selector]||!OperatorID(catalog.CatalogRevision)||!catalog.Availability.Valid()||catalog.Entries==nil||catalog.Slots==nil||len(catalog.Entries)>128||len(catalog.Slots)>128{return false};seenCatalogs[catalog.Selector]=true
		constraints,err:=operatorJSON(catalog.Constraints,OperatorErrorBytes);if err!=nil{return false};object,ok:=constraints.(map[string]any);if !ok||!operatorConstraintsValid(catalog.Type,object,nil)||object["catalogRevision"]!=catalog.CatalogRevision{return false}
		if !operatorConstraintInteger(object["maxEntries"],1,128){return false};if len(catalog.Entries)>operatorConstraintInt(object["maxEntries"]){return false}
		if !catalog.Availability.Available&&(len(catalog.Entries)!=0||len(catalog.Slots)!=0){return false}
		for _,entry:=range catalog.Entries{
			field,exists:=fields[entry.Selector];if !exists||field.Type!=catalog.Type||field.Readonly!=entry.Readonly||field.Availability.Supported!=entry.Availability.Supported||field.Availability.Available!=entry.Availability.Available||!reflect.DeepEqual(field.Availability.ReasonCode,entry.Availability.ReasonCode)||seenEntries[entry.Selector]||!operatorEntrySelector(catalog.Selector,entry.Selector)||!operatorEquivalentJSON(field.Constraints,catalog.Constraints,"")||!operatorEquivalentJSON(field.Value,entry.Value,catalog.Type)||!entry.Availability.Valid(){return false};seenEntries[entry.Selector]=true
			if entry.EntryHandle!=nil&&!addHandle("entry",*entry.EntryHandle){return false}
			if catalog.Readonly&&!entry.Readonly||entry.EditTargets==nil||len(entry.EditTargets)>128||(!entry.Availability.Available||entry.Readonly)&&len(entry.EditTargets)!=0{return false}
			value,err:=operatorFieldJSON(catalog.Type,entry.Value);if err!=nil{return false};if entry.Availability.Available&&value==nil||!entry.Availability.Available&&value!=nil{return false};if value!=nil&&!operatorEntryValue(catalog.Type,value){return false}
			for _,target:=range entry.EditTargets{if !addHandle("target",target.TargetHandle)||target.SourceHandles==nil||len(target.SourceHandles)>128||!target.Availability.Valid()||catalog.Type!="template_entry"&&len(target.SourceHandles)!=0{return false};for _,source:=range target.SourceHandles{if !addHandle("source",source){return false};sourceCount++};targetCount++}
			if entry.Availability.Available&&!entry.Readonly{if !operatorEntryEditAdmits(catalog.Type,entry,value){return false}}else if value!=nil{object:=value.(map[string]any);key:="targetHandle";if catalog.Type=="root_entry"{key="pathHandle"};if !addHandle("target",object[key].(string)){return false};if catalog.Type=="template_entry"{if !addHandle("source",object["sourceHandle"].(string)){return false};sourceCount++}}
			entryCount++
		}
		for _,slot:=range catalog.Slots{
			if slot.Targets==nil||len(slot.Targets)>128||!slot.Availability.Valid(){return false};if slot.SlotHandle!=nil&&!addHandle("slot",*slot.SlotHandle){return false}
			if !slot.Availability.Available&&len(slot.Targets)!=0{return false}
			if catalog.Readonly&&!slot.Readonly||slot.Availability.Available&&!slot.Readonly&&(slot.SlotHandle==nil||len(slot.Targets)==0){return false}
			for _,target:=range slot.Targets{
				kind:="target"
				if !addHandle(kind,target.TargetHandle)||target.SourceHandles==nil||len(target.SourceHandles)>128||!target.Availability.Valid()||catalog.Type!="template_entry"&&len(target.SourceHandles)!=0{return false}
				for _,source:=range target.SourceHandles{if !addHandle("source",source){return false};sourceCount++};targetCount++
			};slotCount++
		}
		if entryCount>128||slotCount>128||targetCount>128||sourceCount>128{return false}
	}
	for left:=range fields{for right:=range fields{if left!=right&&(strings.HasPrefix(right,left+".")||strings.HasPrefix(right,left+"[")){return false}}}
	return true
}
func operatorEntryEditAdmits(kind string,entry OperatorCatalogEntry,value any)bool{
 object,ok:=value.(map[string]any);if !ok{return false};key:="targetHandle";if kind=="root_entry"{key="pathHandle"};target,ok:=object[key].(string);if !ok{return false}
 for _,row:=range entry.EditTargets{if row.TargetHandle!=target||!row.Availability.Available{continue};if kind=="template_entry"{source,ok:=object["sourceHandle"].(string);if !ok||!operatorEnum(source,row.SourceHandles...){continue}};return true};return false
}
