package api

import (
	"encoding/json"
	"regexp"
	"time"
)

type OperatorEnvelope[T any] struct {
	ContractVersion string `json:"contractVersion"`
	RequestID string `json:"requestId"`
	GeneratedAt string `json:"generatedAt"`
	Data T `json:"data"`
}
type OperatorAvailability struct {
	Supported bool `json:"supported"`
	Available bool `json:"available"`
	ReasonCode *string `json:"reasonCode"`
}
func (a OperatorAvailability) Valid() bool {
	if a.Available { return a.Supported && a.ReasonCode == nil }
	if a.ReasonCode == nil { return false }
	if !a.Supported { return *a.ReasonCode == "unsupported_capability" }
	return operatorEnum(*a.ReasonCode,"authentication_required","permission_denied","confirmation_required","binding_stale","revision_conflict","source_unavailable","source_corrupt","dependency_blocked","ownership_unknown","storage_unavailable","no_mutation_required")
}
type OperatorLifecycleAvailability struct {
	ServiceID string `json:"serviceId"`
	DefinitionRevision *string `json:"definitionRevision"`
	Generation *string `json:"generation"`
	ModelEpoch *string `json:"modelEpoch"`
	Capabilities []OperatorCapability `json:"capabilities"`
	MutationBindings []OperatorMutationBinding `json:"mutationBindings"`
	Availability OperatorAvailability `json:"availability"`
}
type OperatorMutationBinding struct{ActionID string `json:"actionId"`;ExpectedRevision *string `json:"expectedRevision"`;Availability OperatorAvailability `json:"availability"`}
type OperatorCapability struct {
	ID string `json:"id"`
	Availability OperatorAvailability `json:"availability"`
	RequiredProfile string `json:"requiredProfile"`
	RequiredScopes []string `json:"requiredScopes"`
	ConfirmationPolicy string `json:"confirmationPolicy"`
	BindingRequirements []string `json:"bindingRequirements"`
}
var OperatorScopes = []string{"service-lasso:read","service-lasso:logs:read","service-lasso:audit:read","service-lasso:lifecycle:write","service-lasso:config:write","service-lasso:update:write","service-lasso:runtime:admin"}
var OperatorJourneys = []string{"overview","detail","graph","variables","endpoints","lifecycle","config","setup","declared_actions","updates","health_history","inbox","queue","audit","logs"}
var OperatorCapabilityIDs = []string{"read_overview","read_detail","read_graph","read_variables","read_endpoints","read_health_history","read_inbox","read_queue","read_audit","read_logs","config_patch","setup_step_run","declared_action_run","update_check","update_download","update_install","service_start","service_stop","service_restart","service_install","service_configure","service_remove_retain_data","inbox_mark_read","inbox_set_visibility","queue_acknowledge","queue_defer","queue_plan_execute","service_meta_read","service_meta_patch","service_reload"}
var OperatorEffects = []string{"install_runtime","materialize_config","start_runtime","stop_runtime","restart_runtime","check_update","download_update","install_update","run_setup","run_declared_action","acknowledge_queue","mark_inbox_read","change_visibility","update_metadata","detach_registration","remove_eligible_artifact","retain_data","retain_recovery"}
type OperatorLimits struct {
	RequestBytes int `json:"requestBytes"`
	ResponseBytes int `json:"responseBytes"`
	ErrorBytes int `json:"errorBytes"`
	Depth int `json:"depth"`
	TotalValues int `json:"totalValues"`
	PropertyNameBytes int `json:"propertyNameBytes"`
	PublicStringBytes int `json:"publicStringBytes"`
	IDBytes int `json:"idBytes"`
	SummaryBytes int `json:"summaryBytes"`
	DisplayNameBytes int `json:"displayNameBytes"`
	DescriptionBytes int `json:"descriptionBytes"`
	PageRows int `json:"pageRows"`
	ContextBytes int `json:"contextBytes"`
}
type OperatorDiscovery struct {
	MetadataRevision string `json:"metadataRevision"`
	ModelEpoch string `json:"modelEpoch"`
	SupportedJourneys []string `json:"supportedJourneys"`
	MutationSemantics string `json:"mutationSemantics"`
	Limits OperatorLimits `json:"limits"`
	Capabilities []OperatorCapability `json:"capabilities"`
}
type OperatorService struct {
	ServiceID string `json:"serviceId"`
	DisplayName string `json:"displayName"`
	Description string `json:"description"`
	DefinitionRevision string `json:"definitionRevision"`
	Generation string `json:"generation"`
	ModelEpoch string `json:"modelEpoch"`
	Installed string `json:"installed"`
	Configured string `json:"configured"`
	Running string `json:"running"`
	HealthStatus string `json:"healthStatus"`
	UpdateStatus string `json:"updateStatus"`
	InboxUnread *int `json:"inboxUnread"`
	QueuePending *int `json:"queuePending"`
	CapabilityIDs []string `json:"capabilityIds"`
}
type OperatorProvider struct { Role string `json:"role"`; Availability OperatorAvailability `json:"availability"` }
type OperatorEndpoint struct { Label string `json:"label"`; Protocol string `json:"protocol"`; Port *int `json:"port"`; Availability OperatorAvailability `json:"availability"` }
type OperatorVariable struct { Name string `json:"name"`; Scope string `json:"scope"`; Resolution string `json:"resolution"` }
type OperatorDetail struct {
	Summary OperatorService `json:"summary"`
	Provider OperatorProvider `json:"provider"`
	Dependencies []OperatorEdge `json:"dependencies"`
	Endpoints []OperatorEndpoint `json:"endpoints"`
	VariableMetadata []OperatorVariable `json:"variableMetadata"`
	SetupMetadata []OperatorSetupStep `json:"setupMetadata"`
	ActionMetadata []OperatorAction `json:"actionMetadata"`
}
type OperatorPage[T any] struct {
	SnapshotID string `json:"snapshotId"`
	SourceRevision string `json:"sourceRevision"`
	Rows []T `json:"rows"`
	NextCursor *string `json:"nextCursor"`
	ExpiresAt string `json:"expiresAt"`
	Truncated bool `json:"truncated"`
}
type OperatorNode struct { ServiceID string `json:"serviceId"`; Role string `json:"role"`; Presence string `json:"presence"`; Availability OperatorAvailability `json:"availability"` }
type OperatorEdge struct { FromServiceID string `json:"fromServiceId"`; ToServiceID string `json:"toServiceId"`; Kind string `json:"kind"`; State string `json:"state"` }
type OperatorGraphGroup struct { Node OperatorNode `json:"node"`; Edges []OperatorEdge `json:"edges"`; EdgeOffset int `json:"edgeOffset"`; EdgeTotal int `json:"edgeTotal"`; Complete bool `json:"complete"` }
type OperatorPosition struct { X int `json:"x"`; Y int `json:"y"` }
type OperatorMeta struct {
	ServiceID string `json:"serviceId"`
	SourceRevision *string `json:"sourceRevision"`
	Favorite *bool `json:"favorite"`
	Position *OperatorPosition `json:"position"`
	Availability OperatorAvailability `json:"availability"`
}
type OperatorInboxRow struct { RowID string `json:"rowId"`; ServiceID string `json:"serviceId"`; Kind string `json:"kind"`; Source string `json:"source"`; Severity string `json:"severity"`; Status string `json:"status"`; Visibility string `json:"visibility"`; CreatedAt string `json:"createdAt"`; Summary string `json:"summary"`; CapabilityIDs []string `json:"capabilityIds"` }
type OperatorQueueRow struct { RowID string `json:"rowId"`; ServiceID string `json:"serviceId"`; Kind string `json:"kind"`; Status string `json:"status"`; Severity string `json:"severity"`; CreatedAt string `json:"createdAt"`; Summary string `json:"summary"`; PlanID *string `json:"planId"` }
type OperatorAuditRow struct { RowID string `json:"rowId"`; ServiceID string `json:"serviceId"`; Actor string `json:"actor"`; Action string `json:"action"`; Status string `json:"status"`; Time string `json:"time"`; Summary string `json:"summary"`; Integrity string `json:"integrity"` }
type OperatorHistoryRow struct { RowID string `json:"rowId"`; ServiceID string `json:"serviceId"`; Kind string `json:"kind"`; Status string `json:"status"`; Phase string `json:"phase"`; Outcome *string `json:"outcome"`; StartedAt *string `json:"startedAt"`; EndedAt *string `json:"endedAt"`; Summary string `json:"summary"` }
type OperatorLogRow struct { RowID string `json:"rowId"`; ServiceID string `json:"serviceId"`; Stream string `json:"stream"`; RunID string `json:"runId"`; Time string `json:"time"`; Text string `json:"text"`; Truncated bool `json:"truncated"` }
type OperatorField struct {
	Selector string `json:"selector"`
	Type string `json:"type"`
	Value json.RawMessage `json:"value"`
	Constraints json.RawMessage `json:"constraints"`
	Readonly bool `json:"readonly"`
	Availability OperatorAvailability `json:"availability"`
}
type OperatorConfigForm struct { ServiceID string `json:"serviceId"`; DefinitionRevision string `json:"definitionRevision"`; ModelEpoch string `json:"modelEpoch"`; Fields []OperatorField `json:"fields"`; Catalogs []OperatorCatalog `json:"catalogs"`; Availability OperatorAvailability `json:"availability"` }
type OperatorCatalog struct{Selector string `json:"selector"`;Type string `json:"type"`;Constraints json.RawMessage `json:"constraints"`;CatalogRevision string `json:"catalogRevision"`;Entries []OperatorCatalogEntry `json:"entries"`;Slots []OperatorCatalogSlot `json:"slots"`;Readonly bool `json:"readonly"`;Availability OperatorAvailability `json:"availability"`}
type OperatorCatalogEntry struct{EntryHandle *string `json:"entryHandle"`;Selector string `json:"selector"`;Value json.RawMessage `json:"value"`;EditTargets []OperatorCatalogTarget `json:"editTargets"`;Readonly bool `json:"readonly"`;Availability OperatorAvailability `json:"availability"`}
type OperatorCatalogSlot struct{SlotHandle *string `json:"slotHandle"`;Targets []OperatorCatalogTarget `json:"targets"`;Readonly bool `json:"readonly"`;Availability OperatorAvailability `json:"availability"`}
type OperatorCatalogTarget struct{TargetHandle string `json:"targetHandle"`;SourceHandles []string `json:"sourceHandles"`;Availability OperatorAvailability `json:"availability"`}
type OperatorSetupStep struct { StepID string `json:"stepId"`; Description string `json:"description"`; Rerun string `json:"rerun"`; DefinitionRevision string `json:"definitionRevision"`; InputFields []OperatorField `json:"inputFields"`; Status *string `json:"status"`; Availability OperatorAvailability `json:"availability"` ;ExpectedRevision *string `json:"expectedRevision"` }
type OperatorSetupForm struct { ServiceID string `json:"serviceId"`; DefinitionRevision string `json:"definitionRevision"`; Steps []OperatorSetupStep `json:"steps"`; Availability OperatorAvailability `json:"availability"` }
type OperatorPayloadPolicy struct { Inline bool `json:"inline"`; References bool `json:"references"`; AllowMixed bool `json:"allowMixed"`; Required bool `json:"required"` }
type OperatorPayloadReference struct { PayloadRef *string `json:"payloadRef"`; Label string `json:"label"`; Availability OperatorAvailability `json:"availability"` }
type OperatorAction struct { ActionID string `json:"actionId"`; Description string `json:"description"`; DefinitionRevision string `json:"definitionRevision"`; PayloadPolicy OperatorPayloadPolicy `json:"payloadPolicy"`; InputFields *OperatorPayloadTable `json:"inputFields"`; PayloadReferences []OperatorPayloadReference `json:"payloadReferences"`; RequiredProfile string `json:"requiredProfile"`; RequiredScopes []string `json:"requiredScopes"`; DeclaredPermission string `json:"declaredPermission"`; Availability OperatorAvailability `json:"availability"` ;ExpectedRevision *string `json:"expectedRevision"` }
type OperatorActionForm struct { ServiceID string `json:"serviceId"`; DefinitionRevision string `json:"definitionRevision"`; Actions []OperatorAction `json:"actions"`; Availability OperatorAvailability `json:"availability"` }
type OperatorUpdateForm struct { ServiceID string `json:"serviceId"`; DefinitionRevision string `json:"definitionRevision"`; Mode string `json:"mode"`; RunningService string `json:"runningService"`; CandidateID *string `json:"candidateId"`; CheckStatus string `json:"checkStatus"`; Availability OperatorAvailability `json:"availability"` ;ExpectedRevision *string `json:"expectedRevision"` }
type OperatorRemovalCategory struct { Code string `json:"code"`; Disposition string `json:"disposition"` }
type OperatorRemovalForm struct { ServiceID string `json:"serviceId"`; DefinitionRevision string `json:"definitionRevision"`; Generation string `json:"generation"`; Categories []OperatorRemovalCategory `json:"categories"`; Availability OperatorAvailability `json:"availability"` ;ExpectedRevision *string `json:"expectedRevision"` }
type OperatorConfirmation struct { Required bool `json:"required"`; ID *string `json:"id"`; PhraseKey *string `json:"phraseKey"`; ExpiresAt *string `json:"expiresAt"` }
type OperatorPreview struct {
	PlanID string `json:"planId"`
	ServiceID string `json:"serviceId"`
	ActionID string `json:"actionId"`
	DefinitionRevision string `json:"definitionRevision"`
	ExpectedRevision string `json:"expectedRevision"`
	Generation string `json:"generation"`
	ModelEpoch string `json:"modelEpoch"`
	Targets []string `json:"targets"`
	Effects []string `json:"effects"`
	Availability OperatorAvailability `json:"availability"`
	Confirmation OperatorConfirmation `json:"confirmation"`
	ExpiresAt string `json:"expiresAt"`
}
type OperatorOperation struct {
	OperationID string `json:"operationId"`
	ActionID string `json:"actionId"`
	ServiceID string `json:"serviceId"`
	Targets []string `json:"targets"`
	Status string `json:"status"`
	Phase string `json:"phase"`
	Outcome *string `json:"outcome"`
	CorrelationID string `json:"correlationId"`
	CreatedAt string `json:"createdAt"`
	UpdatedAt string `json:"updatedAt"`
	ExpiresAt string `json:"expiresAt"`
	CancellationSupported bool `json:"cancellationSupported"`
}
type OperatorResult struct { OperationID string `json:"operationId"`; ActionID string `json:"actionId"`; ServiceID string `json:"serviceId"`; Targets []string `json:"targets"`; Effects []string `json:"effects"`; Summary string `json:"summary"`; ResultingState []OperatorResultState `json:"resultingState"` }
type OperatorSubmission struct { Kind string `json:"kind"`; Operation OperatorOperation `json:"operation"`; Result *OperatorResult `json:"result,omitempty"` }
type OperatorReadback struct { Operation OperatorOperation `json:"operation"`; Result *OperatorResult `json:"result,omitempty"` }
type OperatorCancel struct { OperationID string `json:"operationId"`; ActionID string `json:"actionId"`; Status string `json:"status"` }
type OperatorContext struct { InstanceBinding string `json:"instanceBinding"`; WorkspaceBinding string `json:"workspaceBinding"`; ActorBinding string `json:"actorBinding"`; ClientBinding string `json:"clientBinding"` }

const operatorUTC = "2006-01-02T15:04:05.000Z"
func OperatorTime(value string) bool { parsed, err := time.Parse(operatorUTC,value); return err == nil && len(value) == 24 && parsed.Location() == time.UTC && parsed.Format(operatorUTC) == value }
var opaqueContextPattern = regexp.MustCompile(`^slrc_[a-f0-9]{64}$`)
func (c OperatorContext) Valid() bool { for _, binding := range []string{c.InstanceBinding,c.WorkspaceBinding,c.ActorBinding,c.ClientBinding} { if !opaqueContextPattern.MatchString(binding) { return false } }; return true }

type OperatorResultState struct { ServiceID string `json:"serviceId"`; Installed bool `json:"installed"`; Configured bool `json:"configured"`; Running bool `json:"running"` }
