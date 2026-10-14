package api

import (
	"context"
	"encoding/json"
	"regexp"
	"strconv"
	"strings"
)

type operatorSelector struct{kind string;optional bool;minimum int}
var operatorScalars=map[string]operatorSelector{
	"enabled":{"boolean",true,0},"autostart":{"boolean",true,0},"serviceorder":{"integer",true,-9007199254740991},
	"monitoring.enabled":{"boolean",true,0},"monitoring.intervalSeconds":{"integer",true,1},"monitoring.unhealthyThreshold":{"integer",true,1},"monitoring.startupGraceSeconds":{"integer",true,0},
	"restartPolicy.enabled":{"boolean",true,0},"restartPolicy.onCrash":{"boolean",true,0},"restartPolicy.onUnhealthy":{"boolean",true,0},"restartPolicy.maxAttempts":{"integer",true,0},"restartPolicy.backoffSeconds":{"integer",true,0},
	"updates.enabled":{"boolean",true,0},"updates.mode":{"enum",true,0},"updates.track":{"string",true,0},"updates.checkIntervalSeconds":{"integer",true,60},"updates.runningService":{"enum",true,0},
	"updates.installWindow.days":{"string_array",true,0},"updates.installWindow.start":{"string",false,0},"updates.installWindow.end":{"string",false,0},"updates.installWindow.timezone":{"string",true,0},"files.enabled":{"boolean",true,0},
}
var operatorClock=regexp.MustCompile(`^(?:[01][0-9]|2[0-3]):[0-5][0-9]$`)
func operatorSelectorFor(selector string)(operatorSelector,bool){
	if value,ok:=operatorScalars[selector];ok{return value,true}
	for _,collection:=range []string{"config.files","config.templates","install.files","install.templates","files.roots"}{if operatorEntrySelector(collection,selector){return operatorSelector{operatorCatalogKind(collection),false,0},true}}
	return operatorSelector{},false
}
func operatorSourceValue(selector string,value any)bool{
	declaration,ok:=operatorSelectorFor(selector);if !ok||value==nil{return false}
	if declaration.kind=="integer"{if !operatorConstraintInteger(value,declaration.minimum,9007199254740991){return false}}
	switch selector{
	case "updates.mode":return operatorEnum(value.(string),"disabled","notify","download","install")
	case "updates.runningService":return operatorEnum(value.(string),"skip","require-stopped","stop-start","restart")
	case "updates.track":return strings.TrimSpace(value.(string))!=""
	case "updates.installWindow.days":seen:=map[string]bool{};for _,day:=range value.([]any){text:=day.(string);if !operatorEnum(text,"mon","tue","wed","thu","fri","sat","sun")||seen[text]{return false};seen[text]=true}
	case "updates.installWindow.start","updates.installWindow.end":return operatorClock.MatchString(value.(string))
	};return true
}
func OperatorEditableValue(field OperatorField,raw []byte)bool{
	declaration,ok:=operatorSelectorFor(field.Selector);if !ok||declaration.kind!=field.Type||field.Readonly||!field.Availability.Available{return false}
	copy:=field;copy.Value=raw;if !operatorFieldValid(copy){return false};value,err:=operatorFieldJSON(copy.Type,raw);return err==nil&&operatorSourceValue(field.Selector,value)
}

type OperatorConfigOperation struct{Op string;Selector string;Value json.RawMessage;SlotHandle *string;EntryHandle *string}
func(o OperatorConfigOperation)MarshalJSON()([]byte,error){
	body:=map[string]any{"op":o.Op,"selector":o.Selector}
	switch o.Op{case "set":body["value"]=o.Value;case "unset":case "add_entry":body["slotHandle"]=o.SlotHandle;body["value"]=o.Value;case "remove_entry":body["entryHandle"]=o.EntryHandle;default:return nil,&OperatorPublicError{Code:"invalid_request"}}
	return json.Marshal(body)
}
func(f OperatorConfigForm)ValidateOperations(operations []OperatorConfigOperation)bool{
	if !f.Valid()||!f.Availability.Available||len(operations)==0||len(operations)>128{return false}
	fields:=map[string]OperatorField{};for _,field:=range f.Fields{fields[field.Selector]=field}
	catalogs:=map[string]OperatorCatalog{};for _,catalog:=range f.Catalogs{catalogs[catalog.Selector]=catalog}
	seenFields,seenHandles:=map[string]bool{},map[string]bool{};delta:=map[string]int{}
	for _,operation:=range operations{
		switch operation.Op{
		case "set","unset":
			field,ok:=fields[operation.Selector];if !ok||seenFields[operation.Selector]||field.Readonly||!field.Availability.Available||operation.SlotHandle!=nil||operation.EntryHandle!=nil{return false};seenFields[operation.Selector]=true
			declaration,ok:=operatorSelectorFor(field.Selector);if !ok||declaration.kind!=field.Type{return false}
			for _,catalog:=range f.Catalogs{for _,entry:=range catalog.Entries{if entry.Selector==field.Selector&&(catalog.Readonly||!catalog.Availability.Available||entry.Readonly||!entry.Availability.Available){return false}}}
			if operation.Op=="unset"{if !declaration.optional||len(operation.Value)!=0{return false}}else if !OperatorEditableValue(field,operation.Value){return false}
			if operation.Op=="set"&&(field.Type=="file_entry"||field.Type=="template_entry"||field.Type=="root_entry"){value,err:=operatorFieldJSON(field.Type,operation.Value);if err!=nil{return false};admitted:=false;for _,catalog:=range f.Catalogs{for _,entry:=range catalog.Entries{if entry.Selector==field.Selector&&operatorEntryEditAdmits(field.Type,entry,value){admitted=true}}};if !admitted{return false}}
		case "add_entry":
			catalog,ok:=catalogs[operation.Selector];if !ok||catalog.Readonly||!catalog.Availability.Available||operation.SlotHandle==nil||operation.EntryHandle!=nil||seenHandles["slot:"+*operation.SlotHandle]{return false};seenHandles["slot:"+*operation.SlotHandle]=true
			value,err:=operatorFieldJSON(catalog.Type,operation.Value);if err!=nil||!operatorEntryValue(catalog.Type,value){return false};constraints,err:=operatorJSON(catalog.Constraints,OperatorErrorBytes);if err!=nil||!operatorConstraintsValid(catalog.Type,constraints.(map[string]any),value){return false}
			entry:=value.(map[string]any);targetKey:="targetHandle";if catalog.Type=="root_entry"{targetKey="pathHandle"};target:=entry[targetKey].(string);admitted:=false
			for _,slot:=range catalog.Slots{if slot.SlotHandle==nil||*slot.SlotHandle!=*operation.SlotHandle||slot.Readonly||!slot.Availability.Available{continue};for _,candidate:=range slot.Targets{if candidate.TargetHandle!=target||!candidate.Availability.Available{continue};if catalog.Type=="template_entry"&&!operatorEnum(entry["sourceHandle"].(string),candidate.SourceHandles...){continue};admitted=true}}
			if !admitted{return false};maximum:=operatorConstraintInt(constraints.(map[string]any)["maxEntries"]);if len(catalog.Entries)+delta[catalog.Selector]>=maximum{return false};delta[catalog.Selector]++
		case "remove_entry":
			catalog,ok:=catalogs[operation.Selector];if !ok||catalog.Readonly||!catalog.Availability.Available||operation.EntryHandle==nil||operation.SlotHandle!=nil||len(operation.Value)!=0||seenHandles["entry:"+*operation.EntryHandle]{return false};seenHandles["entry:"+*operation.EntryHandle]=true;admitted:=false
			for _,entry:=range catalog.Entries{field,exists:=fields[entry.Selector];if exists&&!field.Readonly&&field.Availability.Available&&entry.EntryHandle!=nil&&*entry.EntryHandle==*operation.EntryHandle&&!entry.Readonly&&entry.Availability.Available&&!seenFields[entry.Selector]{admitted=true;seenFields[entry.Selector]=true}}
			if !admitted||len(catalog.Entries)+delta[catalog.Selector]<=0{return false};delta[catalog.Selector]--
		default:return false
		}
	}
	for selector,change:=range delta{catalog:=catalogs[selector];constraints,_:=operatorJSON(catalog.Constraints,OperatorErrorBytes);maximum:=operatorConstraintInt(constraints.(map[string]any)["maxEntries"]);if len(catalog.Entries)+change<0||len(catalog.Entries)+change>maximum{return false}}
	return true
}
type operatorConfigPreviewBody struct{ExpectedRevision string `json:"expectedRevision"`;Operations []OperatorConfigOperation `json:"operations"`}
func(b operatorConfigPreviewBody)operatorErrorSelectors()[]string{selectors:=[]string{"expectedRevision","operations"};for _,operation:=range b.Operations{selectors=append(selectors,operation.Selector)};return selectors}
func(c *Client)OperatorConfigPreview(ctx context.Context,form OperatorConfigForm,operations []OperatorConfigOperation)(OperatorPreview,error){
	if !form.ValidateOperations(operations){return OperatorPreview{},&OperatorPublicError{Code:"invalid_request"}}
	body:=operatorConfigPreviewBody{form.DefinitionRevision,operations}
	return operatorRequest(ctx,c,"POST","/services/"+form.ServiceID+"/config-preview",body,func(p OperatorPreview)bool{return operatorPreviewValid(p)&&p.ServiceID==form.ServiceID&&p.ActionID=="config_patch"&&p.DefinitionRevision==form.DefinitionRevision&&p.ExpectedRevision==form.DefinitionRevision&&p.ModelEpoch==form.ModelEpoch},200)
}
func OperatorConstraintChoices(field OperatorField)[]string{
	object,err:=operatorJSON(field.Constraints,OperatorErrorBytes);if err!=nil{return nil};constraints:=object.(map[string]any)
	if field.Type=="enum"{var choices []string;for _,value:=range constraints["values"].([]any){choices=append(choices,value.(string))};return choices}
	if field.Type=="boolean"{return []string{"false","true"}}
	return nil
}
func OperatorIntegerText(text string)bool{value,err:=operatorJSON([]byte(text),128);return err==nil&&operatorConstraintInteger(value,-9007199254740991,9007199254740991)}
func OperatorNumberText(text string)bool{value,err:=operatorJSON([]byte(text),128);_,ok:=value.(json.Number);return err==nil&&ok}
func OperatorNumberValue(raw json.RawMessage)string{var value json.Number;if json.Unmarshal(raw,&value)!=nil{return ""};number,err:=strconv.ParseFloat(string(value),64);if err!=nil{return ""};return strconv.FormatFloat(number,'g',-1,64)}
