package api

import (
 "encoding/json"
 "fmt"
 "testing"
)

func operatorTemplateForm()OperatorConfigForm{
 ready:=OperatorAvailability{Supported:true,Available:true};constraints:=json.RawMessage(`{"catalogRevision":"catalog-r","maxEntries":128}`);value:=json.RawMessage(`{"sourceHandle":"source-a","targetHandle":"target-a"}`)
 field:=OperatorField{Selector:"config.templates[entry-a]",Type:"template_entry",Value:value,Constraints:constraints,Availability:ready}
 entry:=OperatorCatalogEntry{Selector:field.Selector,Value:value,EditTargets:[]OperatorCatalogTarget{{TargetHandle:"target-a",SourceHandles:[]string{"source-a","source-b"},Availability:ready},{TargetHandle:"target-b",SourceHandles:[]string{"source-c"},Availability:ready}},Availability:ready}
 catalog:=OperatorCatalog{Selector:"config.templates",Type:"template_entry",Constraints:constraints,CatalogRevision:"catalog-r",Entries:[]OperatorCatalogEntry{entry},Slots:[]OperatorCatalogSlot{},Availability:ready}
 return OperatorConfigForm{ServiceID:"service",DefinitionRevision:"definition",ModelEpoch:"epoch",Fields:[]OperatorField{field},Catalogs:[]OperatorCatalog{catalog},Availability:ready}
}
func TestOperatorTemplateReplacementUsesOwnEntryRoster(t *testing.T){
 form:=operatorTemplateForm();if !form.Valid(){t.Fatal("genuine writable entry with no removal handle rejected")}
 slotHandle:="insert-slot";form.Catalogs[0].Slots=[]OperatorCatalogSlot{{SlotHandle:&slotHandle,Targets:[]OperatorCatalogTarget{{TargetHandle:"slot-only",SourceHandles:[]string{"slot-source"},Availability:form.Availability}},Availability:form.Availability}};if !form.Valid(){t.Fatal("separate genuine insertion roster rejected")}
 operation:=OperatorConfigOperation{Op:"set",Selector:form.Fields[0].Selector,Value:json.RawMessage(`{"sourceHandle":"source-c","targetHandle":"target-b"}`)}
 if !form.ValidateOperations([]OperatorConfigOperation{operation}){t.Fatal("own-entry alternative replacement rejected")}
 operation.Value=json.RawMessage(`{"sourceHandle":"source-a","targetHandle":"target-b"}`);if form.ValidateOperations([]OperatorConfigOperation{operation}){t.Fatal("foreign target-source pairing accepted")}
 operation.Value=json.RawMessage(`{"sourceHandle":"slot-source","targetHandle":"slot-only"}`);if form.ValidateOperations([]OperatorConfigOperation{operation}){t.Fatal("insertion authority accepted as replacement")};insertion:=OperatorConfigOperation{Op:"add_entry",Selector:"config.templates",SlotHandle:&slotHandle,Value:operation.Value};if !form.ValidateOperations([]OperatorConfigOperation{insertion}){t.Fatal("genuine insertion role rejected")}
 form.Catalogs[0].Entries[0].EditTargets=nil;if form.Valid(){t.Fatal("missing mandatory replacement roster accepted")}
}
func TestOperatorReadonlyTemplateSourcesCountOnceWithEditRoster(t *testing.T){
 form:=operatorTemplateForm();ready:=form.Availability;constraints:=form.Catalogs[0].Constraints
 form.Catalogs[0].Entries[0].EditTargets=[]OperatorCatalogTarget{{TargetHandle:"target-a",SourceHandles:[]string{"source-a"},Availability:ready}}
 for i:=0;i<64;i++{selector:=fmt.Sprintf("config.templates[readonly-%d]",i);raw,_:=json.Marshal(map[string]string{"targetHandle":fmt.Sprintf("read-target-%d",i),"sourceHandle":fmt.Sprintf("read-source-%d",i)});form.Fields=append(form.Fields,OperatorField{Selector:selector,Type:"template_entry",Value:raw,Constraints:constraints,Readonly:true,Availability:ready});form.Catalogs[0].Entries=append(form.Catalogs[0].Entries,OperatorCatalogEntry{Selector:selector,Value:raw,EditTargets:[]OperatorCatalogTarget{},Readonly:true,Availability:ready})}
 for i:=1;i<64;i++{form.Catalogs[0].Entries[0].EditTargets[0].SourceHandles=append(form.Catalogs[0].Entries[0].EditTargets[0].SourceHandles,fmt.Sprintf("edit-source-%d",i))}
 if !form.Valid(){t.Fatal("exact128 original plus issued sources rejected or writable reference charged twice")}
 form.Catalogs[0].Entries[0].EditTargets[0].SourceHandles=append(form.Catalogs[0].Entries[0].EditTargets[0].SourceHandles,"source-over-budget");if form.Valid(){t.Fatal("129 original plus issued sources accepted")}
}
