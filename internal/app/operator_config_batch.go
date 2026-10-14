package app

import (
 "encoding/json"

 "github.com/service-lasso/service-lasso-tui/internal/api"
)

func(m *operatorModel)stageConfigOperation(operation api.OperatorConfigOperation)bool{
 if m.config==nil{return false};operations:=append([]api.OperatorConfigOperation(nil),m.draft.ConfigOperations...)
 if operation.Op=="set"||operation.Op=="unset"{kept:=operations[:0];for _,old:=range operations{if (old.Op=="set"||old.Op=="unset")&&old.Selector==operation.Selector{continue};kept=append(kept,old)};operations=kept}
 operations=append(operations,operation)
 if !m.config.ValidateOperations(operations){m.errorText="These staged changes conflict with the current configuration form.";return false}
 body:=struct{ExpectedRevision string `json:"expectedRevision"`;Operations []api.OperatorConfigOperation `json:"operations"`}{m.draft.Expected,operations};raw,err:=json.Marshal(body)
 if err!=nil{candidate:=m.draft;candidate.ConfigOperations=operations;return m.editorCandidateFailure(m.draft,candidate,api.OperatorInvalidRequestCause(err))};if len(raw)>api.OperatorRequestBytes{m.errorText="The staged configuration exceeds the request byte limit.";return false};m.draft.ConfigOperations=operations;return true
}
func(m *operatorModel)stageConfigField()bool{
 if m.draft.Catalog!=nil{return m.buildCatalogOperation()};if len(m.draft.Fields)!=1||!m.saveEditorField(){return false};field:=m.draft.Fields[0]
 return m.stageConfigOperation(api.OperatorConfigOperation{Op:"set",Selector:field.Selector,Value:append(json.RawMessage(nil),m.draft.Values[field.Selector]...)})
}
func(m *operatorModel)chooseConfigField()bool{
 if m.config==nil{return false};current:=m.draft.Selector;start:=-1;for i,field:=range m.config.Fields{if field.Selector==current{start=i;break}}
 for distance:=1;distance<=len(m.config.Fields);distance++{field:=m.config.Fields[(start+distance)%len(m.config.Fields)];if field.Readonly||!field.Availability.Available{continue};candidate:=*m;candidate.draft.Catalog=nil;candidate.draft.Fields=[]api.OperatorField{field};candidate.draft.FieldIndex=0;candidate.draft.Selector=field.Selector;if prior:=m.draft.Values[field.Selector];prior!=nil{field.Value=prior};candidate.draft.Fields[0]=field;text,err:=operatorFieldTextCause(field);if err!=nil{return m.editorCandidateFailure(m.draft,candidate.draft,err)};candidate.setEditorText(text);if !candidate.loadConfigFieldControls(field){failure:=candidate.editorFailures[len(candidate.editorFailures)-1];rejected:=candidate.draft;if failure.Candidate!=nil{rejected=*failure.Candidate};return m.editorCandidateFailure(m.draft,rejected,failure.Err)};m.draft=candidate.draft;return true};return false
}
func(m *operatorModel)loadConfigFieldControls(field api.OperatorField)bool{
 candidate:=*m;if !candidate.selectEntryControls(field){failure:=candidate.editorFailures[len(candidate.editorFailures)-1];rejected:=candidate.draft;if failure.Candidate!=nil{rejected=*failure.Candidate};return m.editorCandidateFailure(m.draft,rejected,failure.Err)}
 if field.Type!="root_entry"{m.draft=candidate.draft;return true};var value struct{Label,Mode string;Hidden,Protected bool;PathHandle string};if err:=json.Unmarshal(field.Value,&value);err!=nil{return m.editorCandidateFailure(m.draft,candidate.draft,api.OperatorInvalidRequestCause(err))};candidate.draft.RootMode=value.Mode;candidate.draft.RootHidden=value.Hidden;candidate.draft.RootProtected=value.Protected;m.draft=candidate.draft;return true
}
func(m *operatorModel)chooseConfigCatalog()bool{
 if m.config==nil{return false};start:=-1;if m.draft.Catalog!=nil{for i,catalog:=range m.config.Catalogs{if catalog.Selector==m.draft.Catalog.Selector{start=i;break}}}
 for distance:=1;distance<=len(m.config.Catalogs);distance++{catalog:=m.config.Catalogs[(start+distance)%len(m.config.Catalogs)];if catalog.Readonly||!catalog.Availability.Available||len(catalog.Slots)==0{continue};m.draft.Catalog=&catalog;m.draft.EditEntry=nil;m.draft.Fields=nil;m.draft.Selector=catalog.Selector;m.draft.SlotIndex=0;m.draft.TargetIndex=0;m.draft.SourceIndex=0;m.draft.RootMode="read-only";m.draft.RootHidden=false;m.draft.RootProtected=true;m.setEditorText("");return true};return false
}
