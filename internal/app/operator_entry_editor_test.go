package app

import (
 "encoding/json"
 "testing"

 "github.com/service-lasso/service-lasso-tui/internal/api"
)

func TestOperatorTemplateEditorRetainsIssuedReplacementChoices(t *testing.T){
 ready:=api.OperatorAvailability{Supported:true,Available:true};value:=json.RawMessage(`{"sourceHandle":"original-source","targetHandle":"original-target"}`)
 field:=api.OperatorField{Selector:"config.templates[entry]",Type:"template_entry",Value:value,Constraints:json.RawMessage(`{"catalogRevision":"revision","maxEntries":128}`),Availability:ready}
 entry:=api.OperatorCatalogEntry{Selector:field.Selector,Value:value,Availability:ready,EditTargets:[]api.OperatorCatalogTarget{{TargetHandle:"alternative-target",SourceHandles:[]string{"alternative-source"},Availability:ready},{TargetHandle:"original-target",SourceHandles:[]string{"original-source","second-source"},Availability:ready}}}
 form:=api.OperatorConfigForm{ServiceID:"service",DefinitionRevision:"definition",ModelEpoch:"epoch",Availability:ready,Fields:[]api.OperatorField{field},Catalogs:[]api.OperatorCatalog{{Selector:"config.templates",Type:"template_entry",Constraints:field.Constraints,CatalogRevision:"revision",Entries:[]api.OperatorCatalogEntry{entry},Slots:[]api.OperatorCatalogSlot{},Availability:ready}}};if !form.Valid(){t.Fatal("complete classified form fixture invalid")}
 model:=operatorModel{config:&form,draft:operatorDraft{Action:"config_patch",Fields:[]api.OperatorField{field},Values:map[string][]byte{}}};model.selectEntryControls(field)
 if model.draft.TargetIndex!=1||model.draft.SourceIndex!=0||operatorFieldText(field)!=""{t.Fatal("original issued choice not selected or raw JSON exposed")}
 model.insertEditorText(`{"targetHandle":"guessed"}`);if model.draft.Text!=""{t.Fatal("template raw JSON input accepted")}
 if !model.saveEditorField(){t.Fatal("original template replacement failed")};var original map[string]string;json.Unmarshal(model.draft.Values[field.Selector],&original);if original["targetHandle"]!="original-target"||original["sourceHandle"]!="original-source"{t.Fatal("opening editor changed original choice")}
 model.draft.TargetIndex=0;model.draft.SourceIndex=0;if !model.saveEditorField(){t.Fatal("typed alternative replacement failed")};var alternative map[string]string;json.Unmarshal(model.draft.Values[field.Selector],&alternative);if alternative["targetHandle"]!="alternative-target"||alternative["sourceHandle"]!="alternative-source"{t.Fatal("typed selection did not serialize exact issued handles")}
 model.draft.EditEntry=nil;if model.saveEditorField(){t.Fatal("missing own-entry authority admitted")}
}
