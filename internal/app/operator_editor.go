package app

import (
 "crypto/rand"
 "encoding/hex"
 "encoding/json"
 "io"
 "strconv"
 "strings"
 "time"

 tea "github.com/charmbracelet/bubbletea"
 "github.com/rivo/uniseg"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

type operatorPreviewed struct{Ticket operatorTicket `json:"-"`;Preview api.OperatorPreview `json:"-"`;Err error `json:"-"`}
type operatorSubmitted struct{Epoch uint64;Binding api.OperatorContext `json:"-"`;Expected []byte `json:"-"`;Submission api.OperatorSubmission;Err error `json:"-"`;Ticket operatorTicket `json:"-"`}
type operatorCancelled struct{Epoch uint64;ID string;Response api.OperatorCancel;Err error `json:"-"`;Binding api.OperatorContext `json:"-"`;Sequence uint64;Dispatched bool}
type operatorDraftBinding struct{Ticket operatorTicket `json:"-"`;Action string;Availability api.OperatorLifecycleAvailability;Err error `json:"-"`}
// Local failures retain the original draft owner, copied input/value bytes and
// context privately. This is not durable native capture or finite-budget proof.
type operatorEditorFailure struct{Draft operatorDraft `json:"-"`;Candidate *operatorDraft `json:"-"`;ConfigSource *api.OperatorConfigForm `json:"-"`;ActionSource *api.OperatorActionForm `json:"-"`;PayloadValues *operatorPayloadNodeDraft `json:"-"`;CandidatePayloadValues *operatorPayloadNodeDraft `json:"-"`;Ticket operatorTicket `json:"-"`;Err error `json:"-"`}
// Preserve the original owner in Draft.Payload and separately capture its mutable
// values. Memoization preserves shared-node relationships without a policy cap.
func operatorPayloadEvidence(root *operatorPayloadNodeDraft)*operatorPayloadNodeDraft{
 seen:=map[*operatorPayloadNodeDraft]*operatorPayloadNodeDraft{}
 var copyNode func(*operatorPayloadNodeDraft)*operatorPayloadNodeDraft
 copyNode=func(node *operatorPayloadNodeDraft)*operatorPayloadNodeDraft{if node==nil{return nil};if prior:=seen[node];prior!=nil{return prior};value:=*node;value.EvidenceOwner=node;seen[node]=&value;if node.Properties!=nil{value.Properties=map[string]*operatorPayloadNodeDraft{};for name,child:=range node.Properties{value.Properties[name]=copyNode(child)}};if node.Items!=nil{value.Items=make([]*operatorPayloadNodeDraft,len(node.Items));for i,child:=range node.Items{value.Items[i]=copyNode(child)}};return &value}
 return copyNode(root)
}
func operatorEditorEvidenceDraft(draft operatorDraft)operatorDraft{
 draft.Input=append([]byte(nil),draft.Input...)
 fields:=append([]api.OperatorField(nil),draft.Fields...);for i:=range fields{fields[i].Value=append([]byte(nil),fields[i].Value...)};draft.Fields=fields
 if draft.Values!=nil{values:=map[string][]byte{};for selector,raw:=range draft.Values{values[selector]=append([]byte(nil),raw...)};draft.Values=values}
 operations:=append([]api.OperatorConfigOperation(nil),draft.ConfigOperations...);for i:=range operations{operations[i].Value=append([]byte(nil),operations[i].Value...)};draft.ConfigOperations=operations
 return draft
}
func(m *operatorModel)editorFailure(draft operatorDraft,err error)bool{
 if err==nil{return false}
 m.editorFailures=append(m.editorFailures,operatorEditorFailure{Draft:operatorEditorEvidenceDraft(draft),ConfigSource:m.config,ActionSource:m.actions,PayloadValues:operatorPayloadEvidence(draft.Payload),Ticket:m.ticket(),Err:err});m.errorText=api.SafeError(err);return false
}
func(m *operatorModel)editorCandidateFailure(original,candidate operatorDraft,err error)bool{
 m.editorFailure(original,err);if err!=nil{retained:=operatorEditorEvidenceDraft(candidate);failure:=&m.editorFailures[len(m.editorFailures)-1];failure.Candidate=&retained;failure.CandidatePayloadValues=operatorPayloadEvidence(candidate.Payload)};return false
}
func(m *operatorModel)loadDraftLifecycleBinding()tea.Cmd{
 if !m.draftSelectionCurrent(){m.errorText=api.SafeError(&api.OperatorError{Stage:"context"});return nil}
 m.request++;m.loading=true;ticket,client,ctx,action,service,binding:=m.ticket(),m.client,m.requestContext(),m.draft.Action,m.draft.Service,m.draft.Context
 return func()tea.Msg{if err:=operatorSameContext(ctx,client,binding);err!=nil{return operatorDraftBinding{Ticket:ticket,Action:action,Err:err}};current,err:=client.OperatorLifecycleAvailability(ctx,service);if err==nil{err=operatorSameContext(ctx,client,binding)};return operatorDraftBinding{ticket,action,current,err}}
}
func(m *operatorModel)beginEdit()tea.Cmd{
 m.expirePages(time.Now())
 if !m.pageSelectionCurrent()||m.formReadFailed(){return nil}
 if m.mutationBlocked()||m.draft.Sent||m.loading||m.discovery==nil||!m.binding.Valid()||m.operation!=nil&&!m.operation.Operation.CanClear(){return nil}
 original:=m.draft
 d:=operatorDraft{ReadConnection:m.epoch,ReadSelection:m.selection,Service:m.service,ReadService:m.service,ReadJourney:m.journey,ReadFilter:m.filter(),Context:m.binding,Dirty:true,Values:map[string][]byte{},RootMode:"read-only",RootProtected:true}
 switch m.journey{
 case "lifecycle":
  if m.lifecycle==nil||!m.lifecycle.Availability.Available||m.row>=len(m.lifecycle.Capabilities){return nil};cap:=m.lifecycle.Capabilities[m.row];if !cap.Availability.Available{return nil};d.Action=cap.ID;d.Definition=*m.lifecycle.DefinitionRevision;for _,binding:=range m.lifecycle.MutationBindings{if binding.ActionID==cap.ID&&binding.Availability.Available&&binding.ExpectedRevision!=nil{d.Expected=*binding.ExpectedRevision}};if d.Expected==""{return nil};d.Input=[]byte("{}")
 case "config":
  if m.config==nil||!m.config.Availability.Available{return nil};d.Action="config_patch";d.Definition=m.config.DefinitionRevision;d.Expected=d.Definition;d.ConfigModelEpoch=m.config.ModelEpoch
  if m.row<len(m.config.Fields){field:=m.config.Fields[m.row];if field.Readonly||!field.Availability.Available{return nil};d.Fields=[]api.OperatorField{field};d.Selector=field.Selector;text,err:=operatorFieldTextCause(field);if err!=nil{m.editorCandidateFailure(original,d,err);return nil};d.Text=text;d.Values[field.Selector]=append([]byte(nil),field.Value...)
  }else{index:=m.row-len(m.config.Fields);if index<0||index>=len(m.config.Catalogs){return nil};catalog:=m.config.Catalogs[index];if catalog.Readonly||!catalog.Availability.Available||len(catalog.Slots)==0{return nil};d.Catalog=&catalog;d.Selector=catalog.Selector;d.Text=""}
 case "setup":
  if m.setup==nil||!m.setup.Availability.Available||m.row>=len(m.setup.Steps){return nil};step:=m.setup.Steps[m.row];if !step.Availability.Available{return nil};d.Action="setup_step_run";d.Definition=m.setup.DefinitionRevision;if step.ExpectedRevision==nil{return nil};d.Expected=*step.ExpectedRevision;d.StepID=step.StepID;d.Fields=append([]api.OperatorField(nil),step.InputFields...)
 case "declared_actions":
  if m.actions==nil||!m.actions.Availability.Available||m.row>=len(m.actions.Actions){return nil};action:=m.actions.Actions[m.row];if !action.Availability.Available{return nil};d.Action="declared_action_run";d.Definition=m.actions.DefinitionRevision;if action.ExpectedRevision==nil{return nil};d.Expected=*action.ExpectedRevision;d.DeclaredID=action.ActionID;d.PayloadPolicy=action.PayloadPolicy;d.PayloadReferences=append([]api.OperatorPayloadReference(nil),action.PayloadReferences...);if action.InputFields!=nil{schema,ok,schemaErr:=action.InputFields.TreeCause();if schemaErr!=nil{m.editorCandidateFailure(original,d,api.OperatorInvalidRequestCause(schemaErr));return nil};if !ok{return nil};d.Payload=newOperatorPayloadNode(schema,true);d.InlinePayload=d.Payload!=nil;if d.Payload==nil{m.errorText="The declared payload minimum exceeds request limits; retained reference editing remains available when authorized."};d.Selector="/input/payload"}
 case "updates":
  if m.updates==nil||m.row>2{return nil};d.Definition=m.updates.DefinitionRevision;d.Action="update_check";if m.row==1{d.Action="update_download"};if m.row==2{if !m.updates.Availability.Available||m.updates.CandidateID==nil{return nil};if m.updates.ExpectedRevision==nil{return nil};d.Expected=*m.updates.ExpectedRevision;d.Action="update_install";d.Input,_=json.Marshal(api.OperatorUpdateInput{CandidateID:*m.updates.CandidateID})}else{d.Input=[]byte("{}")}
 case "removal":
  if m.removal==nil||!m.removal.Availability.Available{return nil};d.Action="service_remove_retain_data";d.Definition=m.removal.DefinitionRevision;if m.removal.ExpectedRevision==nil{return nil};d.Expected=*m.removal.ExpectedRevision;d.Input=[]byte(`{"retainData":true}`)
 case "queue":
  if m.queue.Stale||m.row>=len(m.queue.Rows){return nil};row:=m.queue.Rows[m.row];if row.Status!="open"||row.PlanID==nil{return nil};d.Service=row.ServiceID;d.Action="queue_plan_execute";d.Expected=m.queue.Revision;d.QueueRow=&row
 case "metadata":
  if m.meta==nil||!m.meta.Availability.Available||m.meta.SourceRevision==nil||m.row>1{return nil};observation,ok:=m.metadataCurrent[m.service];if !ok||observation.Err!=nil||observation.Key.Connection!=m.epoch||observation.Key.Binding!=m.binding||observation.Key.Generation==""{return nil};d.MetadataGeneration=observation.Key.Generation;d.Action="service_meta_patch";d.Expected=*m.meta.SourceRevision;d.Selector="favorite";d.Text="false";if m.meta.Favorite!=nil{d.Text=strconv.FormatBool(!*m.meta.Favorite)};if m.row==1{d.Selector="position";d.Text="automatic";if m.meta.Position!=nil{d.Text=strconv.Itoa(m.meta.Position.X)+","+strconv.Itoa(m.meta.Position.Y)}}
 case "inbox":
  if m.inbox.Stale||m.row>=len(m.inbox.Rows){return nil};row:=m.inbox.Rows[m.row];d.Service=row.ServiceID;d.Action="inbox_mark_read";d.Expected=m.inbox.Revision;d.InboxRow=&row;d.Text="mark read"
 default:return nil
 }
 if len(d.Fields)==1&&d.Fields[0].Type=="root_entry"{var root struct{Label,Mode string;Hidden,Protected bool};if err:=json.Unmarshal(d.Fields[0].Value,&root);err!=nil{m.editorCandidateFailure(original,d,api.OperatorInvalidRequestCause(err));return nil};d.Text=root.Label;d.RootMode=root.Mode;d.RootHidden=root.Hidden;d.RootProtected=root.Protected}
 if d.Action=="setup_step_run"||d.Action=="declared_action_run"{editable:=[]api.OperatorField{};for _,field:=range d.Fields{if !field.Readonly&&field.Availability.Available{editable=append(editable,field)}};d.Fields=editable}
 if len(d.Fields)>0&&d.Selector==""{for i,field:=range d.Fields{if !field.Readonly&&field.Availability.Available{d.FieldIndex=i;d.Selector=field.Selector;text,err:=operatorFieldTextCause(field);if err!=nil{m.editorCandidateFailure(original,d,err);return nil};d.Text=text;break}};for _,field:=range d.Fields{if !field.Readonly&&field.Availability.Available&&string(field.Value)!="null"{d.Values[field.Selector]=append([]byte(nil),field.Value...)}}}
 d.Cursor=len(d.Text);m.draft=d;if d.Action=="config_patch"&&len(d.Fields)==1&&!m.selectEntryControls(d.Fields[0]){m.draft=original;m.editorFailures[len(m.editorFailures)-1].Draft=operatorEditorEvidenceDraft(original);m.editorFailures[len(m.editorFailures)-1].PayloadValues=operatorPayloadEvidence(original.Payload);return nil};if d.Payload!=nil{m.showPayloadFocus()};if m.journey=="updates"&&d.Action!="update_install"{return m.loadDraftLifecycleBinding()};return nil
}
func operatorFieldText(field api.OperatorField)string{
 text,_:=operatorFieldTextCause(field);return text
}
func operatorFieldTextCause(field api.OperatorField)(string,error){
 var value any;if err:=json.Unmarshal(field.Value,&value);err!=nil{return "",api.OperatorInvalidRequestCause(err)};if value==nil{return "",nil}
 switch field.Type{case "string","enum","logical_handle":if text,ok:=value.(string);ok{return text,nil};case "file_entry","root_entry":key:="content";if field.Type=="root_entry"{key="label"};if object,ok:=value.(map[string]any);ok{if text,ok:=object[key].(string);ok{return text,nil}};case "string_array":if array,ok:=value.([]any);ok{values:=[]string{};for _,item:=range array{if text,ok:=item.(string);ok{values=append(values,text)}};return strings.Join(values,"\n"),nil}}
 if field.Type=="template_entry"{return "",nil};return string(field.Value),nil
}
func operatorEditedValue(field api.OperatorField,text string)([]byte,error){
 switch field.Type{
 case "string","enum","logical_handle":return json.Marshal(text)
 case "string_array":values:=[]string{};if text!=""{values=strings.Split(text,"\n")};return json.Marshal(values)
 case "file_entry":var object map[string]json.RawMessage;if err:=json.Unmarshal(field.Value,&object);err!=nil{return nil,api.OperatorInvalidRequestCause(err)};if object==nil{return nil,&api.OperatorPublicError{Code:"invalid_request"}};content,err:=json.Marshal(text);if err!=nil{return nil,api.OperatorInvalidRequestCause(err)};object["content"]=content;return json.Marshal(object)
 case "template_entry":return append([]byte(nil),field.Value...),nil
 default:return []byte(text),nil
 }
}
func(m *operatorModel)saveEditorField()bool{
 if m.draft.ReferenceMode{if !m.selectedPayloadReferenceValid(){m.errorText="Select an available payload reference for this action.";return false};return true}
 if m.draft.Payload!=nil{return m.savePayloadText()};if len(m.draft.Fields)==0{return true};field:=m.draft.Fields[m.draft.FieldIndex];if field.Readonly||!field.Availability.Available{return false}
 value,err:=operatorEditedValue(field,m.draft.Text);if err!=nil{return m.editorFailure(m.draft,err)}
 if field.Type=="root_entry"{var object map[string]json.RawMessage;if err:=json.Unmarshal(field.Value,&object);err!=nil{return m.editorFailure(m.draft,api.OperatorInvalidRequestCause(err))};if object==nil{m.errorText=api.OperatorMessage("invalid_request");return false};object["label"],_=json.Marshal(m.draft.Text);object["mode"],_=json.Marshal(m.draft.RootMode);object["hidden"],_=json.Marshal(m.draft.RootHidden);object["protected"],_=json.Marshal(m.draft.RootProtected);value,err=json.Marshal(object);if err!=nil{return m.editorFailure(m.draft,api.OperatorInvalidRequestCause(err))}}
 if m.draft.Action=="config_patch"&&(field.Type=="file_entry"||field.Type=="template_entry"||field.Type=="root_entry"){value,err=m.editedEntryValue(field,value);if err!=nil{m.editorFailure(m.draft,err);m.errorText="Select a replacement target and source issued for this entry.";return false}}
 if m.draft.Action=="config_patch"&&!api.OperatorEditableValue(field,value){m.errorText="Value does not meet the declared field constraints.";return false}
 m.draft.Values[field.Selector]=value;return true
}
func removeLastGrapheme(text string)string{graphemes:=uniseg.NewGraphemes(text);last:=0;for graphemes.Next(){start,_:=graphemes.Positions();last=start};return text[:last]}
func(m *operatorModel)editKey(msg tea.KeyMsg)tea.Cmd{
 if msg.String()=="esc"{if !m.draft.Sent{if m.cancel!=nil{m.cancel();m.cancel=nil};m.request++;m.loading=false;m.draft=operatorDraft{}};return nil}
 if m.draft.Sent{if msg.String()=="r"&&!m.loading{return m.load(nil)};return nil}
 if m.loading{return nil}
 if m.draft.Action=="setup_step_run"&&m.draft.Preview==nil{
  switch msg.String(){case "alt+pgdown":m.moveSetupReview(true);return nil;case "alt+pgup":m.moveSetupReview(false);return nil;case "alt+home":m.draft.SetupReview=false;return nil}
  if m.draft.SetupReview{switch msg.String(){case "ctrl+f":m.draft.Force=!m.draft.Force;case "alt+m":m.draft.IncludeManual=!m.draft.IncludeManual};return nil}
 }
 if msg.String()=="ctrl+v"&&m.quotedPayloadInputAllowed(){m.draft.LiteralNext=true;return nil}
 if m.draft.ReferenceMode&&m.draft.Preview==nil{switch msg.String(){case "tab","shift+tab","alt+n":m.pickPayloadReference(true);return nil;case "left","right","up","down","home","end","delete","backspace":return nil}}
 switch msg.String(){case "left":m.draft.moveCursor(false);return nil;case "right":m.draft.moveCursor(true);return nil;case "up":m.draft.moveLine(false);return nil;case "down":m.draft.moveLine(true);return nil;case "home":m.draft.lineBoundary(false);return nil;case "end":m.draft.lineBoundary(true);return nil;case "delete":m.draft.erase(false);return nil;case "backspace":m.draft.erase(true);return nil}
 if m.draft.Preview!=nil{
  switch msg.String(){case "alt+pgdown":m.movePreviewReview(true);case "alt+pgup":m.movePreviewReview(false);case "alt+home":m.draft.PreviewReview=false;case "enter":return m.submitPreview();case "ctrl+u":m.setEditorText("");default:if msg.Type==tea.KeyRunes{m.draft.insert(string(msg.Runes),128)}};return nil
 }
 if m.literalEditorInputAllowed(){switch msg.String(){case "ctrl+n":m.insertEditorText("\n");return nil;case "ctrl+t":m.insertEditorText("\t");return nil}}
 if m.payloadKey(msg){return nil}
 switch msg.String(){
 case "alt+w":if m.editingRoot(){if m.draft.RootMode=="read-only"{m.draft.RootMode="read-write"}else{m.draft.RootMode="read-only"}}
 case "alt+h":if m.editingRoot(){m.draft.RootHidden=!m.draft.RootHidden}
 case "alt+b":if m.editingRoot(){m.draft.RootProtected=!m.draft.RootProtected}
 case "alt+m":if m.draft.Action=="setup_step_run"{m.draft.IncludeManual=!m.draft.IncludeManual}else if m.draft.Action=="declared_action_run"&&!m.startPayloadOverlay(){m.errorText="Select an available reference and action with declared mixed payload fields first."}
 case "alt+p":if m.draft.Action=="declared_action_run"&&m.draft.PayloadPolicy.References{if !m.draft.ReferenceMode&&!m.saveEditorField(){return nil};m.pickPayloadReference(m.draft.ReferenceMode)}
 case "alt+n":if len(m.draft.Fields)>0&&!m.draft.ReferenceMode{choices:=api.OperatorConstraintChoices(m.draft.Fields[m.draft.FieldIndex]);if len(choices)>0{next:=0;for i,value:=range choices{if value==m.draft.Text{next=(i+1)%len(choices)}};m.setEditorText(choices[next])}}
 case "alt+i":if m.draft.Action=="declared_action_run"&&m.draft.Payload!=nil{m.draft.InlinePayload=!m.draft.InlinePayload}
 case "alt+o":if m.draft.Action=="declared_action_run"{if m.draft.Payload!=nil&&m.draft.Payload.OverlaySchema!=nil{m.errorText="This draft is a reference overlay. Start a new inline draft to omit the reference.";return nil};m.draft.PayloadRef=nil;m.draft.ReferenceMode=false;if m.draft.Payload!=nil{m.showPayloadFocus()}}
 case "alt+l":if m.draft.Action=="service_meta_patch"{m.draft.Selector="position";m.setEditorText("automatic")}
 case "alt+v":if m.draft.InboxRow!=nil{m.draft.Action="inbox_set_visibility";m.setEditorText("hidden");if m.draft.InboxRow.Visibility=="hidden"{m.setEditorText("visible")}}
 case "alt+a":if m.draft.QueueRow!=nil{m.draft.Action="queue_acknowledge";m.setEditorText("acknowledge")}
 case "alt+q":if m.draft.QueueRow!=nil{m.draft.Action="queue_defer";m.setEditorText("")}
 case "alt+s":m.cycleConfigSlot()
 case "alt+t":m.cycleConfigTarget()
 case "alt+r":m.cycleConfigSource()
 case "alt+enter":if m.draft.Action=="config_patch"{m.stageConfigField()}
 case "alt+f":if m.draft.Action=="config_patch"{m.chooseConfigField()}
 case "alt+k":if m.draft.Action=="config_patch"{m.chooseConfigCatalog()}
 case "alt+d":if m.draft.Action=="config_patch"{m.removeConfigEntry()}
 case "alt+u":if m.draft.Action=="config_patch"{m.unsetConfigField()}
 case "tab","shift+tab":
  if len(m.draft.Fields)>0&&!m.draft.ReferenceMode&&m.saveEditorField(){original:=m.draft;direction:=1;if msg.String()=="shift+tab"{direction=-1};for i:=0;i<len(m.draft.Fields);i++{m.draft.FieldIndex=(m.draft.FieldIndex+len(m.draft.Fields)+direction)%len(m.draft.Fields);field:=m.draft.Fields[m.draft.FieldIndex];if field.Readonly||!field.Availability.Available{continue};m.draft.Selector=field.Selector;raw:=m.draft.Values[field.Selector];if raw!=nil{field.Value=raw};text,err:=operatorFieldTextCause(field);if err!=nil{candidate:=m.draft;candidate.Fields=append([]api.OperatorField(nil),candidate.Fields...);candidate.Fields[candidate.FieldIndex]=field;m.draft=original;m.editorCandidateFailure(original,candidate,err);return nil};m.setEditorText(text);break}}
 case "ctrl+j":m.insertEditorText("\n")
 case "ctrl+t":m.insertEditorText("\t")
 case "ctrl+u":m.setEditorText("")
 case "ctrl+f":if m.draft.Action=="setup_step_run"{m.draft.Force=!m.draft.Force}
 case "enter":return m.previewDraft()
 default:if msg.Type==tea.KeyRunes{m.insertEditorText(string(msg.Runes))}
 };return nil
}
// This is the original admitted read selection, not a new service/context
// inferred from the current page or a server-held authorization lease.
func(m operatorModel)draftSelectionCurrent()bool{return api.OperatorServiceID(m.draft.Service)&&m.draft.ReadConnection==m.epoch&&m.draft.ReadSelection==m.selection&&m.draft.ReadService==m.service&&m.draft.ReadJourney==m.journey&&m.draft.ReadFilter==m.filter()&&m.draft.Context.Valid()&&m.draft.Context==m.binding&&m.pageSelectionCurrent()&&!m.formReadFailed()}
func(m *operatorModel)previewDraft()tea.Cmd{
 if m.mutationBlocked()||m.loading||m.draft.Expected==""{return nil};if !m.draftSelectionCurrent(){m.errorText=api.SafeError(&api.OperatorError{Stage:"context"});return nil}
 if m.draft.Action=="config_patch"&&(m.config==nil||m.config.ServiceID!=m.draft.Service||m.config.DefinitionRevision!=m.draft.Definition||m.config.DefinitionRevision!=m.draft.Expected||m.config.ModelEpoch!=m.draft.ConfigModelEpoch){m.errorText=api.OperatorMessage("binding_stale");return nil}
 if !m.saveEditorField(){return nil};d:=m.draft
 if d.Action=="service_meta_patch"||d.Action=="inbox_mark_read"||d.Action=="inbox_set_visibility"||d.Action=="queue_acknowledge"||d.Action=="queue_defer"{if !d.Review{m.draft.Review=true;return nil};return m.submitDirect()}
 switch d.Action{
 case "config_patch":
  if d.Catalog!=nil||len(d.Fields)==1{if !m.stageConfigField(){return nil};d=m.draft};if len(d.ConfigOperations)==0{return nil}
  if m.config==nil||!m.config.ValidateOperations(d.ConfigOperations){m.errorText="Configuration edit is not admitted by the current form.";return nil}
 case "setup_step_run":values:=map[string]json.RawMessage{};for key,value:=range d.Values{values[key]=value};input:=api.OperatorSetupInput{StepID:d.StepID,Values:values,IncludeManual:d.IncludeManual,Force:d.Force};if m.setup==nil||!api.OperatorSetupInputValid(*m.setup,input){m.errorText="Setup inputs do not meet the current declaration.";return nil};d.Input,_=json.Marshal(input)
 case "declared_action_run":input:=api.OperatorDeclaredInput{DeclaredActionID:d.DeclaredID,PayloadRef:d.PayloadRef};if d.InlinePayload{if d.Payload==nil{return nil};raw,ok,payloadErr:=d.Payload.valueCause();if payloadErr!=nil{m.editorFailure(d,api.OperatorInvalidRequestCause(payloadErr));return nil};if !ok{m.errorText="Complete the typed payload fields and required values.";return nil};input.Payload=raw};if m.actions==nil{m.errorText="Action payload does not meet the current declaration.";return nil};inputOK,inputErr:=api.OperatorDeclaredInputValidCause(*m.actions,input);if inputErr!=nil{m.editorFailure(d,api.OperatorInvalidRequestCause(inputErr));return nil};if !inputOK{m.errorText="Action payload does not meet the current declaration.";return nil};inputRaw,inputErr:=json.Marshal(input);if inputErr!=nil{m.editorFailure(d,api.OperatorInvalidRequestCause(inputErr));return nil};d.Input=inputRaw
 }
 m.draft=d;m.loading=true;m.request++;ticket,client,ctx:=m.ticket(),m.client,m.requestContext();form:=m.config;actions:=m.actions;service:=d.Service
 return func()tea.Msg{
  var p api.OperatorPreview;var err error
  if err=operatorSameContext(ctx,client,d.Context);err!=nil{return operatorPreviewed{Ticket:ticket,Err:err}}
  if d.Action=="config_patch"{p,err=client.OperatorConfigPreview(ctx,*form,d.ConfigOperations)}else if d.Action=="queue_plan_execute"{p,err=client.OperatorQueuePreview(ctx,*d.QueueRow,d.Expected)}else if d.Action=="declared_action_run"&&actions!=nil{p,err=client.OperatorDeclaredPreview(ctx,api.OperatorPreviewRequest{ServiceID:service,ActionID:d.Action,DefinitionRevision:d.Definition,ExpectedRevision:d.Expected,Input:d.Input},*actions)}else{p,err=client.OperatorPreview(ctx,api.OperatorPreviewRequest{ServiceID:service,ActionID:d.Action,DefinitionRevision:d.Definition,ExpectedRevision:d.Expected,Input:d.Input})}
  if err==nil{err=operatorSameContext(ctx,client,d.Context)};return operatorPreviewed{ticket,p,err}
 }
}
func(m *operatorModel)submitPreview()tea.Cmd{
 return m.submitPreviewWithEntropy(rand.Reader)
}
func(m *operatorModel)submitPreviewWithEntropy(entropy io.Reader)tea.Cmd{
 if m.mutationBlocked()||m.draft.Preview==nil||m.draft.Sent{return nil};if !m.draftSelectionCurrent()||m.draft.Preview.ServiceID!=m.draft.Service{m.errorText=api.SafeError(&api.OperatorError{Stage:"context"});return nil};p:=*m.draft.Preview
 if !api.OperatorConfirmationResponseValid(p,m.draft.Text){m.errorText="Enter the exact displayed confirmation phrase.";return nil}
 var nonce [24]byte;if _,err:=io.ReadFull(entropy,nonce[:]);err!=nil{m.editorFailure(m.draft,api.OperatorInvalidRequestCause(err));m.errorText="Unable to allocate an operation key.";return nil};key:=hex.EncodeToString(nonce[:]);m.draft.Idempotency=key
 request:=api.OperatorExecuteRequest{PlanID:p.PlanID,IdempotencyKey:key,ExpectedRevision:p.ExpectedRevision};if p.Confirmation.Required{response:=m.draft.Text;request.ConfirmationID=p.Confirmation.ID;request.ConfirmationResponse=&response}
 ticket:=m.ticket();m.draft.MutationTicket=&ticket;m.draft.Sent=true;m.loading=true;epoch,binding,expected,client,ctx:=m.epoch,m.draft.Context,append([]byte(nil),m.saved...),m.client,m.ctx;source:=api.OperatorExecutionSource{Input:append([]byte(nil),m.draft.Input...),ConfigOperations:m.draft.ConfigOperations,QueueRow:m.draft.QueueRow}
 return func()tea.Msg{submission,err:=client.OperatorExecute(ctx,request,p,binding,source);return operatorSubmitted{epoch,binding,expected,submission,err,ticket}}
}
// This session retains one original cancellation attempt after possible POST.
// It is not a durable server idempotency receipt or cross-restart authority.
func(m *operatorModel)cancelOperation()tea.Cmd{
 if m.mutationBlocked()||m.operation==nil||!m.operation.Operation.CancellationSupported||m.operation.Operation.Outcome!=nil{return nil};if m.cancelInFlight{m.warning="Cancellation request is pending; authoritative readback determines the outcome.";return nil};if m.cancelAttempted{m.warning="Cancellation was already requested for this operation; do not repeat it. Read authoritative operation state.";return nil};m.cancelSequence++;m.cancelInFlight=true;sequence:=m.cancelSequence;epoch,op,client,ctx,binding:=m.epoch,m.operation.Operation,m.client,m.requestContext(),m.binding
 return func()tea.Msg{
  result:=operatorCancelled{Epoch:epoch,ID:op.OperationID,Binding:binding,Sequence:sequence}
  current,err:=readOperatorOwn(ctx,client,binding,op);if err!=nil{result.Err=err;return result}
  if !current.Operation.CancellationSupported||current.Operation.Outcome!=nil{result.Err=&api.OperatorPublicError{Code:"unsupported_capability"};return result}
  if err:=operatorSameContext(ctx,client,binding);err!=nil{result.Err=err;return result}
  result.Dispatched=true;result.Response,result.Err=client.OperatorCancel(ctx,current.Operation);return result
 }
}
