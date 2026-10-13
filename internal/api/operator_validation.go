package api

import (
	"encoding/json"
	"math"
	"reflect"
	"strconv"
	"strings"

)

func operatorUniqueEnum(values []string, max int, enums []string) bool { if values == nil || len(values) > max { return false }; seen := map[string]bool{}; for _, value := range values { if seen[value] || !operatorEnum(value,enums...) { return false }; seen[value] = true }; return true }
func (a OperatorLifecycleAvailability) Valid() bool {
 if !OperatorServiceID(a.ServiceID)||!a.Availability.Valid()||a.Capabilities==nil||a.MutationBindings==nil{return false}
 if !a.Availability.Available{return a.DefinitionRevision==nil&&a.Generation==nil&&a.ModelEpoch==nil&&len(a.Capabilities)==0&&len(a.MutationBindings)==0}
 if a.DefinitionRevision==nil||a.Generation==nil||a.ModelEpoch==nil||!OperatorID(*a.DefinitionRevision)||!OperatorID(*a.Generation)||!OperatorID(*a.ModelEpoch)||len(a.Capabilities)!=8||len(a.MutationBindings)!=8{return false}
 for i,b:=range a.MutationBindings{c:=a.Capabilities[i];if b.ActionID!=c.ID||b.Availability.Supported!=c.Availability.Supported||!operatorRevisionGrant(b.ExpectedRevision,b.Availability)||b.Availability.Available&&!c.Availability.Available{return false};if b.ActionID=="service_reload"&&(b.Availability.Supported||b.ExpectedRevision!=nil){return false}}
	seen:=map[string]bool{};for _,c:=range a.Capabilities{
		if !operatorEnum(c.ID,"service_install","service_configure","service_start","service_stop","service_restart","update_check","update_download","service_reload")||seen[c.ID]||!operatorCapabilityValid(c){return false};seen[c.ID]=true
		if c.ID=="service_reload"{if c.Availability.Supported||c.Availability.Available||c.Availability.ReasonCode==nil||*c.Availability.ReasonCode!="unsupported_capability"{return false};continue}
		if !c.Availability.Supported||!operatorEnum("service-lasso:read",c.RequiredScopes...){return false}
		for _,binding:=range []string{"definition_revision","runtime_generation","model_epoch"}{if !operatorEnum(binding,c.BindingRequirements...){return false}}
	};return true
}
func (a OperatorLifecycleAvailability) Admits(p OperatorPreview) bool {
	if !a.Valid()||!a.Availability.Available||a.ServiceID!=p.ServiceID||*a.DefinitionRevision!=p.DefinitionRevision||*a.Generation!=p.Generation||*a.ModelEpoch!=p.ModelEpoch{return false}
 for _,b:=range a.MutationBindings{if b.ActionID==p.ActionID{return b.Availability.Available&&b.ExpectedRevision!=nil&&*b.ExpectedRevision==p.ExpectedRevision}};return false
}
func operatorRevisionGrant(revision *string,availability OperatorAvailability)bool{if !availability.Valid(){return false};if availability.Available{return revision!=nil&&OperatorID(*revision)};return revision==nil}
func operatorCapabilityValid(c OperatorCapability) bool {
	if !operatorEnum(c.ID,OperatorCapabilityIDs...) || !c.Availability.Valid() || !operatorEnum(c.RequiredProfile,"observer","operator","maintainer","administrator") || !operatorUniqueEnum(c.RequiredScopes,7,OperatorScopes) || !operatorEnum(c.ConfirmationPolicy,"none","required") || !operatorUniqueEnum(c.BindingRequirements,12,[]string{"metadata_revision","graph_revision","definition_revision","manifest_revision","runtime_generation","model_epoch","provider_revision","artifact_revision","setup_revision","payload_revision","registry_revision","allocation_revision"}) { return false }
	if c.ID == "service_reload" && c.Availability.Supported { return false }
	if c.ID == "read_logs" || c.ID == "read_audit" {
		special := "service-lasso:logs:read"; if c.ID == "read_audit" { special = "service-lasso:audit:read" }
		if !operatorEnum("service-lasso:read",c.RequiredScopes...) || !operatorEnum(special,c.RequiredScopes...) { return false }
	}
	return true
}
func (d OperatorDiscovery) Valid() bool {
	if !OperatorID(d.MetadataRevision) || !OperatorID(d.ModelEpoch) || !operatorUniqueEnum(d.SupportedJourneys,15,OperatorJourneys) || !operatorEnum(d.MutationSemantics,"all_writer_atomic_revision","legacy_serialized_current_model","unavailable") || len(d.Capabilities) > 128 || d.Capabilities == nil { return false }
	if d.Limits != (OperatorLimits{65536,262144,8192,16,8192,128,4096,128,512,256,1024,100,4096}) { return false }
	seen := map[string]bool{}; for _, c := range d.Capabilities { if seen[c.ID] || !operatorCapabilityValid(c) { return false }; seen[c.ID] = true
		if c.Availability.Available && !strings.HasPrefix(c.ID,"read_") && c.ID != "service_meta_read" && d.MutationSemantics != "all_writer_atomic_revision" { return false }
	}; return true
}
func (s OperatorService) Valid() bool {
	if !OperatorServiceID(s.ServiceID) || !operatorDisplay(s.DisplayName,256) || !operatorDisplay(s.Description,1024) || !OperatorID(s.DefinitionRevision) || !OperatorID(s.Generation) || !OperatorID(s.ModelEpoch) || !operatorEnum(s.Installed,"yes","no","unknown") || !operatorEnum(s.Configured,"yes","no","unknown") || !operatorEnum(s.Running,"yes","no","unknown") || !operatorEnum(s.HealthStatus,"healthy","unhealthy","unknown","not_configured") || !operatorEnum(s.UpdateStatus,"pinned","latest","update_available","unavailable","check_failed","not_checked","installing","installed","install_failed","unknown") || !operatorUniqueEnum(s.CapabilityIDs,128,OperatorCapabilityIDs) { return false }
	for _, n := range []*int{s.InboxUnread,s.QueuePending} { if n != nil && (*n < 0 || *n > 1000000) { return false } }; return true
}
func operatorEdgeValid(e OperatorEdge) bool { return OperatorServiceID(e.FromServiceID) && OperatorServiceID(e.ToServiceID) && operatorEnum(e.Kind,"service","provider") && operatorEnum(e.State,"satisfied","blocked","unknown") }
func operatorNodeValid(n OperatorNode) bool {
	if !OperatorServiceID(n.ServiceID) || !operatorEnum(n.Role,"service","provider","unknown") || !operatorEnum(n.Presence,"present","missing") || !n.Availability.Valid() { return false }
	if n.Presence == "missing" { return n.Role == "unknown" && n.Availability.Supported && !n.Availability.Available && n.Availability.ReasonCode != nil && *n.Availability.ReasonCode == "dependency_blocked" }; return true
}
func operatorEndpointValid(e OperatorEndpoint) bool { return operatorDisplay(e.Label,256) && operatorEnum(e.Protocol,"http","https","tcp","udp") && (e.Port == nil || *e.Port >= 1 && *e.Port <= 65535) && e.Availability.Valid() }
func operatorVariableValid(v OperatorVariable) bool { return operatorDisplay(v.Name,128) && v.Name != "" && operatorEnum(v.Scope,"manifest","global","runtime","derived") && operatorEnum(v.Resolution,"configured","resolved","unresolved","unavailable") }
func (m OperatorMeta) Valid() bool {
	if !OperatorServiceID(m.ServiceID) || !m.Availability.Valid() { return false }
	if m.Availability.Available { return m.SourceRevision != nil && OperatorID(*m.SourceRevision) && m.Favorite != nil && (m.Position == nil || m.Position.X >= -100000 && m.Position.X <= 100000 && m.Position.Y >= -100000 && m.Position.Y <= 100000) }
	return m.Availability.Supported && m.Availability.ReasonCode != nil && operatorEnum(*m.Availability.ReasonCode,"source_unavailable","source_corrupt") && m.SourceRevision == nil && m.Favorite == nil && m.Position == nil
}
func (p OperatorPage[T]) ValidHeader(max int) bool { return OperatorID(p.SnapshotID) && OperatorID(p.SourceRevision) && p.Rows != nil && len(p.Rows) <= max && OperatorTime(p.ExpiresAt) && (p.NextCursor == nil || OperatorID(*p.NextCursor)) }
func operatorFieldsValid(fields []OperatorField) bool { if fields == nil || len(fields) > 128 { return false }; seen := map[string]bool{}; for _, f := range fields { if seen[f.Selector] || !operatorFieldValid(f) { return false }; seen[f.Selector] = true }; return true }
func operatorFieldValid(f OperatorField) bool {
	if !operatorDisplay(f.Selector,128) || f.Selector == "" || !f.Availability.Valid() { return false }
	value, err := operatorFieldJSON(f.Type,f.Value); if err != nil { return false }
	if !f.Availability.Available&&value!=nil{return false}
	constraints, err := operatorJSON(f.Constraints,OperatorErrorBytes); if err != nil { return false }
	object, ok := constraints.(map[string]any); if !ok { return false }
	allowed := []string{}
	switch f.Type {
	case "boolean": if value != nil { if _,ok := value.(bool); !ok { return false } }
	case "integer","number": allowed = []string{"minimum","maximum"}; if value != nil { if _,ok := value.(json.Number); !ok { return false }; if f.Type == "integer" && !operatorShape(value,reflect.TypeOf(int(0))) { return false } }
	case "string": allowed = []string{"maxBytes"}; if value != nil { if _,ok := value.(string); !ok { return false } }
	case "enum": allowed = []string{"values"}; if value != nil { if _,ok := value.(string); !ok { return false } }
	case "string_array": allowed = []string{"maxItems","maxItemBytes","unique"}; if value != nil { if !operatorShape(value,reflect.TypeOf([]string{})) { return false } }
	case "logical_handle": allowed = []string{"catalogRevision"}; if value != nil { handle,ok := value.(string); if !ok || !OperatorID(handle) { return false } }
	case "file_entry","template_entry","root_entry": allowed = []string{"catalogRevision","maxEntries","maxContentBytes"}; if value != nil { if !operatorEntryValue(f.Type,value) { return false } }
	default: return false
	}
	for key := range object { if !operatorEnum(key,allowed...) { return false } }
	return operatorConstraintsValid(f.Type,object,value)
}
func operatorConstraintInteger(value any,min,max int)bool{
	if !operatorShape(value,reflect.TypeOf(int(0))){return false};number,_:=strconv.ParseFloat(string(value.(json.Number)),64);return number>=float64(min)&&number<=float64(max)
}
func operatorConstraintInt(value any)int{number,_:=strconv.ParseFloat(string(value.(json.Number)),64);return int(number)}
func operatorConstraintsValid(kind string,c map[string]any,value any)bool{
	switch kind{
	case "boolean":return len(c)==0
	case "integer","number":
		if len(c)!=2{return false};if _,exists:=c["minimum"];!exists{return false};if _,exists:=c["maximum"];!exists{return false}
		for _,key:=range []string{"minimum","maximum"}{if bound,exists:=c[key];exists{if _,ok:=bound.(json.Number);!ok{return false};if kind=="integer"&&!operatorShape(bound,reflect.TypeOf(int(0))){return false}}}
		minimum,maximum:=math.Inf(-1),math.Inf(1)
		if bound,exists:=c["minimum"];exists{minimum,_=strconv.ParseFloat(string(bound.(json.Number)),64)};if bound,exists:=c["maximum"];exists{maximum,_=strconv.ParseFloat(string(bound.(json.Number)),64)}
		if minimum>maximum{return false};if value!=nil{number,_:=strconv.ParseFloat(string(value.(json.Number)),64);return number>=minimum&&number<=maximum};return true
	case "string":
		if len(c)!=1||!operatorConstraintInteger(c["maxBytes"],0,4096){return false};if value==nil{return true};maximum:=operatorConstraintInt(c["maxBytes"]);return len(value.(string))<=maximum&&operatorDisplay(value.(string),maximum)
	case "enum":
		values,ok:=c["values"].([]any);if len(c)!=1||!ok||len(values)==0||len(values)>32{return false};seen:=map[string]bool{};matched:=value==nil
		for _,candidate:=range values{text,ok:=candidate.(string);if !ok||!operatorDisplay(text,4096)||seen[text]{return false};seen[text]=true;if value==text{matched=true}};return matched
	case "string_array":
		if len(c)!=3||!operatorConstraintInteger(c["maxItems"],0,128)||!operatorConstraintInteger(c["maxItemBytes"],0,4096){return false};unique,ok:=c["unique"].(bool);if !ok{return false};if value==nil{return true}
		maxItems:=operatorConstraintInt(c["maxItems"]);maxBytes:=operatorConstraintInt(c["maxItemBytes"]);array:=value.([]any);if len(array)>maxItems{return false};seen:=map[string]bool{};for _,item:=range array{text:=item.(string);if !operatorDisplay(text,maxBytes)||unique&&seen[text]{return false};seen[text]=true};return true
	case "logical_handle":revision,ok:=c["catalogRevision"].(string);return len(c)==1&&ok&&OperatorID(revision)
	case "file_entry","template_entry","root_entry":
		revision,ok:=c["catalogRevision"].(string);if !ok||!OperatorID(revision)||!operatorConstraintInteger(c["maxEntries"],0,128){return false}
		if kind=="file_entry"{maximum:=32768;if bound,exists:=c["maxContentBytes"];exists{if len(c)!=3||!operatorConstraintInteger(bound,0,32768){return false};maximum=operatorConstraintInt(bound)}else if len(c)!=2{return false};if value!=nil{entry:=value.(map[string]any);content:=entry["content"].(string);if !operatorFileContent(content,maximum){return false}}}else if len(c)!=2{return false};return true
	};return false
}
func operatorEntryValue(kind string, value any) bool {
	object, ok := value.(map[string]any); if !ok { return false }
	keys := []string{}; switch kind { case "file_entry":keys=[]string{"targetHandle","content"};case "template_entry":keys=[]string{"sourceHandle","targetHandle"};case "root_entry":keys=[]string{"label","mode","hidden","protected","pathHandle"} }
	if len(object) != len(keys) { return false }; for key, child := range object { if !operatorEnum(key,keys...) { return false }; switch key { case "hidden","protected": if _,ok:=child.(bool); !ok { return false };case "mode": s,ok:=child.(string);if !ok || !operatorEnum(s,"read-only","read-write"){return false};case "label":s,ok:=child.(string);if !ok || !operatorDisplay(s,256){return false};case "content":s,ok:=child.(string);if !ok || !operatorFileContent(s,32768){return false};default:s,ok:=child.(string);if !ok || !OperatorID(s){return false} } }; return true
}
func operatorSetupValid(s OperatorSetupStep) bool { return OperatorID(s.StepID) && operatorDisplay(s.Description,1024) && operatorEnum(s.Rerun,"manual","ifMissing","always","ifChanged") && OperatorID(s.DefinitionRevision) && s.InputFields!=nil && len(s.InputFields)==0 && (s.Status==nil||operatorEnum(*s.Status,"succeeded","failed","timeout","skipped")) && operatorRevisionGrant(s.ExpectedRevision,s.Availability) }
func operatorActionValid(a OperatorAction) bool { return OperatorID(a.ActionID) && operatorDisplay(a.Description,1024) && OperatorID(a.DefinitionRevision) && operatorActionReferencesValid(a) && operatorActionSchemaValid(a) && operatorEnum(a.RequiredProfile,"observer","operator","maintainer","administrator") && operatorUniqueEnum(a.RequiredScopes,7,OperatorScopes) && operatorDisplay(a.DeclaredPermission,128) && operatorRevisionGrant(a.ExpectedRevision,a.Availability) && (!a.PayloadPolicy.AllowMixed || a.PayloadPolicy.Inline && a.PayloadPolicy.References) && (!a.PayloadPolicy.Required || a.PayloadPolicy.Inline || a.PayloadPolicy.References) }
func (d OperatorDetail) Valid() bool {
	if !d.Summary.Valid() || !operatorEnum(d.Provider.Role,"provider","service","unknown") || !d.Provider.Availability.Valid() || d.Dependencies == nil || len(d.Dependencies)>256 || d.Endpoints == nil || len(d.Endpoints)>128 || d.VariableMetadata==nil || len(d.VariableMetadata)>128 || d.SetupMetadata==nil || len(d.SetupMetadata)>128 || d.ActionMetadata==nil || len(d.ActionMetadata)>128 { return false }
	for _,e:=range d.Dependencies{if !operatorEdgeValid(e)||e.ToServiceID!=d.Summary.ServiceID{return false}};for _,e:=range d.Endpoints{if !operatorEndpointValid(e){return false}};for _,v:=range d.VariableMetadata{if !operatorVariableValid(v){return false}};for _,s:=range d.SetupMetadata{if !operatorSetupValid(s){return false}};if !operatorActionRosterValid(d.ActionMetadata){return false};for _,action:=range d.ActionMetadata{if action.DefinitionRevision!=d.Summary.DefinitionRevision{return false}};return true
}
func (o OperatorOperation) Valid() bool {
	if !OperatorID(o.OperationID)||!OperatorServiceID(o.ServiceID)||!OperatorID(o.CorrelationID)||!operatorEnum(o.ActionID,"service_start","service_stop","service_restart","service_install","service_configure","update_check","update_download","setup_step_run","update_install","declared_action_run","service_remove_retain_data","queue_plan_execute","config_patch")||!operatorServiceIDs(o.Targets,256)||!OperatorTime(o.CreatedAt)||!OperatorTime(o.UpdatedAt)||!OperatorTime(o.ExpiresAt)||o.UpdatedAt<o.CreatedAt||o.ExpiresAt<o.CreatedAt{return false}
	if operatorEnum(o.ActionID,"service_start","service_stop","service_restart","service_install","service_configure") && o.CancellationSupported{return false}
	return operatorLegalOperationState(o.Status,o.Phase,o.Outcome)
}
func operatorLegalOperationState(status,phase string,outcome *string) bool {
	if operatorEnum(status,"queued","running","cancelling"){return outcome==nil&&operatorEnum(phase,"preflight","preflight_complete","executing","checking_update","downloading_update","verifying_update","finalizing","detached","cancellation_requested")}
	if outcome==nil||*outcome!=status{return false}
	switch status{case "succeeded":return operatorEnum(phase,"completed","guarded_result_reconciled");case "failed":return operatorEnum(phase,"failed","interrupted","guarded_result_reconciled");case "cancelled":return phase=="cancelled";case "skipped":return operatorEnum(phase,"skipped","replayed","guarded_result_reconciled");case "unknown_after_crash":return phase=="unknown_after_crash"};return false
}
func (o OperatorOperation) CanClear() bool { return o.Valid() && o.Outcome != nil && o.Status != "unknown_after_crash" && o.Phase != "interrupted" }
func (o OperatorOperation) SameRoot(other OperatorOperation) bool { return o.OperationID==other.OperationID && o.ActionID==other.ActionID && o.ServiceID==other.ServiceID && o.CorrelationID==other.CorrelationID }
func operatorPreviewValid(p OperatorPreview) bool {
	if !OperatorID(p.PlanID)||!OperatorServiceID(p.ServiceID)||!operatorEnum(p.ActionID,"service_start","service_stop","service_restart","service_install","service_configure","update_check","update_download","setup_step_run","update_install","declared_action_run","service_remove_retain_data","queue_plan_execute","config_patch")||!OperatorID(p.DefinitionRevision)||!OperatorID(p.ExpectedRevision)||!OperatorID(p.Generation)||!OperatorID(p.ModelEpoch)||!operatorServiceIDs(p.Targets,256)||!operatorUniqueEnum(p.Effects,128,OperatorEffects)||!p.Availability.Valid()||!OperatorTime(p.ExpiresAt){return false}
	if p.Confirmation.Required{return p.Confirmation.ID!=nil&&OperatorID(*p.Confirmation.ID)&&p.Confirmation.PhraseKey!=nil&&operatorPhrase(p.ActionID,*p.Confirmation.PhraseKey)&&p.Confirmation.ExpiresAt!=nil&&OperatorTime(*p.Confirmation.ExpiresAt)&&*p.Confirmation.ExpiresAt<=p.ExpiresAt}
	return p.Confirmation.ID==nil&&p.Confirmation.PhraseKey==nil&&p.Confirmation.ExpiresAt==nil
}
func operatorPhrase(action,phrase string)bool{
	prefix:="confirm "+strings.ReplaceAll(action,"_","-")+" ";if !operatorDisplay(phrase,128)||!strings.HasPrefix(phrase,prefix){return false};nonce:=strings.TrimPrefix(phrase,prefix);if len(nonce)!=32{return false};for _,r:=range nonce{if !(r>='0'&&r<='9'||r>='a'&&r<='f'){return false}};return true
}
func OperatorConfirmationResponseValid(preview OperatorPreview,response string)bool{return operatorPreviewValid(preview)&&(!preview.Confirmation.Required&&response==""||preview.Confirmation.Required&&preview.Confirmation.PhraseKey!=nil&&response==*preview.Confirmation.PhraseKey&&operatorDisplay(response,128))}
