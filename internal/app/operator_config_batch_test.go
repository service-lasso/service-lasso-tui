package app

import (
 "encoding/json"
 "bytes"
 "testing"

 "github.com/service-lasso/service-lasso-tui/internal/api"
)

func TestOperatorConfigurationStagesOneAtomicFieldBatch(t *testing.T){
 ready:=api.OperatorAvailability{Supported:true,Available:true};fields:=[]api.OperatorField{{Selector:"enabled",Type:"boolean",Value:json.RawMessage(`false`),Constraints:json.RawMessage(`{}`),Availability:ready},{Selector:"autostart",Type:"boolean",Value:json.RawMessage(`false`),Constraints:json.RawMessage(`{}`),Availability:ready}}
 form:=api.OperatorConfigForm{ServiceID:"service",DefinitionRevision:"definition",ModelEpoch:"epoch",Fields:fields,Catalogs:[]api.OperatorCatalog{},Availability:ready}
 model:=operatorModel{config:&form,draft:operatorDraft{Action:"config_patch",Expected:"definition",Fields:fields[:1],Selector:"enabled",Text:"true",Values:map[string][]byte{}}}
 if !model.stageConfigField()||!model.chooseConfigField(){t.Fatal("first change could not stage and select next field")};model.setEditorText("true");if !model.stageConfigField(){t.Fatal("second field change rejected")}
 if len(model.draft.ConfigOperations)!=2||model.draft.ConfigOperations[0].Selector!="enabled"||model.draft.ConfigOperations[1].Selector!="autostart"{t.Fatal("field selection replaced previous staged change")}
 model.setEditorText("false");if !model.stageConfigField()||len(model.draft.ConfigOperations)!=2{t.Fatal("revising one staged field duplicated or erased batch")};if string(model.draft.ConfigOperations[1].Value)!="false"{t.Fatal("latest typed field value lost")}
 original,_:=json.Marshal(model.draft.ConfigOperations);if model.stageConfigOperation(api.OperatorConfigOperation{Op:"set",Selector:"private.guessed.path",Value:json.RawMessage(`true`)}){t.Fatal("foreign selector staged")};retained,_:=json.Marshal(model.draft.ConfigOperations);if !bytes.Equal(original,retained){t.Fatal("denied staging changed exact existing batch")}
 envelope,_:=json.Marshal(struct{ExpectedRevision string `json:"expectedRevision"`;Operations []api.OperatorConfigOperation `json:"operations"`}{model.draft.Expected,model.draft.ConfigOperations});var decoded struct{ExpectedRevision string `json:"expectedRevision"`;Operations []api.OperatorConfigOperation `json:"operations"`};if json.Unmarshal(envelope,&decoded)!=nil||decoded.ExpectedRevision!="definition"||len(decoded.Operations)!=2{t.Fatal("batch revision or operations changed in request serialization")}
}
