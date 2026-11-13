package app

import (
 "encoding/json"

 "github.com/service-lasso/service-lasso-tui/internal/api"
)

func(m *operatorModel)selectEntryControls(field api.OperatorField)bool{
 original,candidate:=m.draft,m.draft;candidate.EditEntry=nil;candidate.TargetIndex=0;candidate.SourceIndex=0
 if m.config==nil{m.draft=candidate;return true};for _,catalog:=range m.config.Catalogs{for _,entry:=range catalog.Entries{if entry.Selector!=field.Selector{continue};candidate.EditEntry=&entry;var value map[string]json.RawMessage;if err:=json.Unmarshal(field.Value,&value);err!=nil{return m.editorCandidateFailure(original,candidate,api.OperatorInvalidRequestCause(err))};if value==nil{return m.editorCandidateFailure(original,candidate,&api.OperatorPublicError{Code:"invalid_request"})};key:="targetHandle";if field.Type=="root_entry"{key="pathHandle"};var currentTarget,currentSource string;if err:=json.Unmarshal(value[key],&currentTarget);err!=nil{return m.editorCandidateFailure(original,candidate,api.OperatorInvalidRequestCause(err))};if field.Type=="template_entry"{if err:=json.Unmarshal(value["sourceHandle"],&currentSource);err!=nil{return m.editorCandidateFailure(original,candidate,api.OperatorInvalidRequestCause(err))}};for i,target:=range entry.EditTargets{if target.TargetHandle!=currentTarget{continue};candidate.TargetIndex=i;for j,source:=range target.SourceHandles{if source==currentSource{candidate.SourceIndex=j}}};m.draft=candidate;return true}};m.draft=candidate;return true
}
func(m *operatorModel)editedEntryValue(field api.OperatorField,raw []byte)([]byte,error){
 entry:=m.draft.EditEntry;if entry==nil||entry.Selector!=field.Selector||entry.Readonly||!entry.Availability.Available||len(entry.EditTargets)==0{return nil,&api.OperatorPublicError{Code:"invalid_request"}}
 target:=entry.EditTargets[m.draft.TargetIndex%len(entry.EditTargets)];if !target.Availability.Available{return nil,&api.OperatorPublicError{Code:"invalid_request"}}
 var value map[string]json.RawMessage;if err:=json.Unmarshal(raw,&value);err!=nil{return nil,api.OperatorInvalidRequestCause(err)};if value==nil{return nil,&api.OperatorPublicError{Code:"invalid_request"}};key:="targetHandle";if field.Type=="root_entry"{key="pathHandle"};encoded,err:=json.Marshal(target.TargetHandle);if err!=nil{return nil,api.OperatorInvalidRequestCause(err)};value[key]=encoded
 if field.Type=="template_entry"{if len(target.SourceHandles)==0{return nil,&api.OperatorPublicError{Code:"invalid_request"}};encoded,err:=json.Marshal(target.SourceHandles[m.draft.SourceIndex%len(target.SourceHandles)]);if err!=nil{return nil,api.OperatorInvalidRequestCause(err)};value["sourceHandle"]=encoded}
 return json.Marshal(value)
}
func(m *operatorModel)cycleConfigSlot(){if catalog:=m.draft.Catalog;catalog!=nil&&len(catalog.Slots)>0{m.draft.SlotIndex=(m.draft.SlotIndex%len(catalog.Slots)+1)%len(catalog.Slots);m.draft.TargetIndex=0;m.draft.SourceIndex=0}}
func(m *operatorModel)configTargetChoices()[]api.OperatorCatalogTarget{if entry:=m.draft.EditEntry;entry!=nil{return entry.EditTargets};if catalog:=m.draft.Catalog;catalog!=nil&&len(catalog.Slots)>0{return catalog.Slots[m.draft.SlotIndex%len(catalog.Slots)].Targets};return nil}
func(m *operatorModel)cycleConfigTarget(){choices:=m.configTargetChoices();if len(choices)>0{m.draft.TargetIndex=(m.draft.TargetIndex%len(choices)+1)%len(choices);m.draft.SourceIndex=0}}
func(m *operatorModel)cycleConfigSource(){choices:=m.configTargetChoices();if len(choices)==0{return};sources:=choices[m.draft.TargetIndex%len(choices)].SourceHandles;if len(sources)>0{m.draft.SourceIndex=(m.draft.SourceIndex%len(sources)+1)%len(sources)}}
