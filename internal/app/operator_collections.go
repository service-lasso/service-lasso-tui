package app

import (
 "encoding/json"

 "github.com/service-lasso/service-lasso-tui/internal/api"
)
func(m operatorModel)editingRoot()bool{return m.draft.Catalog!=nil&&m.draft.Catalog.Type=="root_entry"||len(m.draft.Fields)>0&&m.draft.Fields[m.draft.FieldIndex].Type=="root_entry"}

func(m *operatorModel)buildCatalogOperation()bool{
 catalog:=m.draft.Catalog;if catalog==nil||catalog.Readonly||!catalog.Availability.Available||len(catalog.Slots)==0{return false}
 index:=m.draft.SlotIndex%len(catalog.Slots);slot:=catalog.Slots[index]
 if slot.SlotHandle==nil||slot.Readonly||!slot.Availability.Available||len(slot.Targets)==0{m.errorText="Select an available issued slot.";return false}
 target:=slot.Targets[m.draft.TargetIndex%len(slot.Targets)];if !target.Availability.Available{m.errorText="Select an available logical target.";return false}
 value:=map[string]any{}
 switch catalog.Type{
 case "root_entry":value["pathHandle"]=target.TargetHandle;value["label"]=m.draft.Text;value["mode"]=m.draft.RootMode;value["hidden"]=m.draft.RootHidden;value["protected"]=m.draft.RootProtected
 case "file_entry":value["targetHandle"]=target.TargetHandle;value["content"]=m.draft.Text
 case "template_entry":if len(target.SourceHandles)==0{m.errorText="Select an issued template source.";return false};value["targetHandle"]=target.TargetHandle;value["sourceHandle"]=target.SourceHandles[m.draft.SourceIndex%len(target.SourceHandles)]
 default:return false
 }
 raw,err:=json.Marshal(value);if err!=nil{return false};handle:=*slot.SlotHandle
 if !m.stageConfigOperation(api.OperatorConfigOperation{Op:"add_entry",Selector:catalog.Selector,SlotHandle:&handle,Value:raw}){return false};m.draft.Catalog=nil;m.draft.Fields=nil;return true
}
func(m *operatorModel)removeConfigEntry()bool{
 if m.config==nil||len(m.draft.Fields)!=1{return false};selector:=m.draft.Fields[0].Selector
 for _,catalog:=range m.config.Catalogs{for _,entry:=range catalog.Entries{if entry.Selector==selector&&entry.EntryHandle!=nil{handle:=*entry.EntryHandle;if !m.stageConfigOperation(api.OperatorConfigOperation{Op:"remove_entry",Selector:catalog.Selector,EntryHandle:&handle}){return false};m.draft.Fields=nil;m.setEditorText("Entry removal staged with the declared recovery policy.");return true}}};return false
}
func(m *operatorModel)unsetConfigField()bool{
 if m.config==nil||len(m.draft.Fields)!=1{return false};if !m.stageConfigOperation(api.OperatorConfigOperation{Op:"unset",Selector:m.draft.Fields[0].Selector}){return false};m.draft.Fields=nil;m.setEditorText("Optional declaration removal staged.");return true
}
