package api

import (
 "github.com/service-lasso/service-lasso-tui/internal/lifetime"
	"bytes"
	"context"
	"crypto/rand"
	"encoding/json"
"errors"
	"fmt"
	"io"
	"net"
	"net/http"
	"net/url"
	"regexp"
	"strings"
	"time"
	"unicode"
	"unicode/utf16"
	"unicode/utf8"
)

const requestTimeout = 5 * time.Second

var serviceIDPattern = regexp.MustCompile(`^@?[A-Za-z0-9][A-Za-z0-9._-]{0,127}$`)

// Client consumes documented Service Lasso runtime HTTP contracts.
type Client struct {
 receiver *lifetime.Receiver
	baseURL       string
	http          *http.Client
	operatorToken string
	authMode      AuthMode
}

// AuthMode is explicit so an existing local-admin token is never silently
// reinterpreted as an OAuth bearer credential.
type AuthMode string

const (
	AuthModeLocalAdmin  AuthMode = "local-admin"
	AuthModeOAuthBearer AuthMode = "oauth-bearer"
)

// ConfigurationErrorKind classifies a rejected local API-client configuration
// without exposing the supplied URL or operator token in ordinary output.
type ConfigurationErrorKind string

const (
	ConfigurationErrorInvalidURL             ConfigurationErrorKind = "invalid_url"
	ConfigurationErrorUnsupportedScheme      ConfigurationErrorKind = "unsupported_scheme"
	ConfigurationErrorUserinfo               ConfigurationErrorKind = "userinfo"
	ConfigurationErrorQueryOrFragment        ConfigurationErrorKind = "query_or_fragment"
	ConfigurationErrorInsecureTokenTransport ConfigurationErrorKind = "insecure_token_transport"
	ConfigurationErrorRemoteProfileAdmission ConfigurationErrorKind = "remote_profile_admission"
	ConfigurationErrorOperatorReaderAdmission ConfigurationErrorKind = "operator_reader_admission"
)

// ConfigurationError reports only the closed validation class.
type ConfigurationError struct {
	Kind ConfigurationErrorKind
	cause error
	input *connectionInputObservation
}
func(e *ConfigurationError)Unwrap()error{return e.cause}

func (e *ConfigurationError) Error() string {
	switch e.Kind {
	case ConfigurationErrorInvalidURL:
		return "invalid Service Lasso API URL"
	case ConfigurationErrorUnsupportedScheme:
		return "Service Lasso API URL must use HTTP or HTTPS"
	case ConfigurationErrorUserinfo:
		return "Service Lasso API URL must not contain userinfo"
	case ConfigurationErrorQueryOrFragment:
		return "Service Lasso API URL must not contain a query or fragment"
	case ConfigurationErrorInsecureTokenTransport:
		return "operator token requires HTTPS for a non-loopback Service Lasso API URL"
	case ConfigurationErrorRemoteProfileAdmission:
		return "non-loopback Service Lasso API requires explicit oauth-bearer lifecycle scopes"
	case ConfigurationErrorOperatorReaderAdmission:
		return "non-loopback Service Lasso API requires explicit oauth-bearer general-read scope"
	default:
		return "invalid Service Lasso API configuration"
	}
}

type Health struct {
	Status string `json:"status"`
	API    struct {
		Status  string `json:"status"`
		Version string `json:"version"`
	} `json:"api"`
}

type Service struct {
	ID          string `json:"id"`
	Name        string `json:"name"`
	Description string `json:"description"`
	Enabled     bool   `json:"enabled"`
	Health      struct {
		Status string `json:"status"`
	} `json:"health"`
	Lifecycle struct {
		State string `json:"state"`
	} `json:"lifecycle"`
}

type servicesResponse struct {
	Services []Service `json:"services"`
}

// Dashboard surfaces contain only the bounded fields rendered by the TUI.
// The full Core payloads can contain paths, identities, and operator content
// which are not dashboard projections. Failed requests keep bounded originals
// privately as partial continuity, never as ordinary display or JSON output.
type Capabilities struct {
	RuntimeVersion  string
	ContractVersion string
}

type SetupStatus struct {
	State      string
	SetupMode  bool
	VaultReady bool
}

type RuntimeIdentity struct {
	Status string
	Phase  string
}

type InboxItem struct {
	ID        string `json:"id"`
	Title     string `json:"title"`
	Severity  string `json:"severity"`
	State     string `json:"state"`
	CreatedAt string `json:"createdAt"`
}

type Inbox struct {
	Items      []InboxItem
	Total      int
	NextCursor string
}

type HealthHistory struct {
	ServiceID string
	Entries   int
}

type LifecycleAvailability struct {
	Action               string
	Available            bool
	Reason               string
	Permission           string
	RequiresConfirmation bool
}

// LifecyclePreview contains the one immutable Core confirmation context. It is
// intentionally transient: only its resulting operation ID can be persisted.
type LifecyclePreview struct {
	ReconciliationContext string
	Action             string
	ServiceID          string
	ConfirmationID     string
	ConfirmationPhrase string
	Targets            []string
	Effects            []string
}

// Operation contains the closed, bounded readback fields that are safe to
// render. Core owns and redacts any durable detail omitted here.
type Operation struct {
	ID                    string
	Action                string
	Status                string
	Phase                 string
	Progress              int
	Outcome               string
	CancellationSupported bool
	Ownership             string
	Targets               []string
	CancellationResult    string
	CancellationTerminal  bool
}

func NewClient(baseURL string, client *http.Client, operatorToken string) (*Client, error) {
	return NewClientWithAuth(baseURL, client, operatorToken, AuthModeLocalAdmin)
}

// NewClientWithAuth admits a non-loopback runtime only when its caller
// explicitly declares the Core OAuth mode and both lifecycle scopes. The
// declaration is a local configuration guard; Core validates the bearer token
// and authorizes every request.
func NewClientWithAuth(baseURL string, client *http.Client, operatorToken string, authMode AuthMode, scopes ...string) (*Client, error) {
 return newClientWithAuth(baseURL,client,operatorToken,authMode,false,scopes...)
}

// V2 readers need declared general-read transport admission. These local
// profile declarations never validate a token or grant a server capability.
func NewOperatorClientWithAuth(baseURL string,client *http.Client,operatorToken string,authMode AuthMode,scopes ...string)(*Client,error){
 return newClientWithAuth(baseURL,client,operatorToken,authMode,true,scopes...)
}

func newClientWithAuth(baseURL string,client *http.Client,operatorToken string,authMode AuthMode,operatorV2 bool,scopes ...string)(*Client,error){
	parsed, err := parseBaseURL(baseURL)
	if err != nil {
		return nil, err
	}
	if authMode == "" {
		authMode = AuthModeLocalAdmin
	}
	if authMode != AuthModeLocalAdmin && authMode != AuthModeOAuthBearer {
		return nil, &ConfigurationError{Kind: ConfigurationErrorInvalidURL}
	}
	if !isLoopbackHost(parsed.Hostname()) && (authMode != AuthModeOAuthBearer || !hasConnectionScopes(scopes,operatorV2)) {
		if operatorV2 {
			return nil, &ConfigurationError{Kind: ConfigurationErrorOperatorReaderAdmission}
		}
		return nil, &ConfigurationError{Kind: ConfigurationErrorRemoteProfileAdmission}
	}
	if !isLoopbackHost(parsed.Hostname()) && parsed.Scheme != "https" {
		return nil, &ConfigurationError{Kind: ConfigurationErrorInsecureTokenTransport}
	}
	if client == nil {
		client = &http.Client{Timeout: requestTimeout}
	}
	isolatedClient := *client
	isolatedClient.CheckRedirect = func(_ *http.Request, _ []*http.Request) error {
		return http.ErrUseLastResponse
	}
	return &Client{baseURL: strings.TrimRight(parsed.String(), "/"), http: &isolatedClient, operatorToken: operatorToken, authMode: authMode}, nil
}

func parseBaseURL(baseURL string) (*url.URL, error) {
	parsed, err := url.Parse(baseURL)
	if err != nil { return nil, &ConfigurationError{Kind:ConfigurationErrorInvalidURL,cause:err} }
	if parsed.Scheme == "" || parsed.Host == "" {
		return nil, &ConfigurationError{Kind: ConfigurationErrorInvalidURL}
	}
	if parsed.Scheme != "http" && parsed.Scheme != "https" {
		return nil, &ConfigurationError{Kind: ConfigurationErrorUnsupportedScheme}
	}
	if parsed.User != nil {
		return nil, &ConfigurationError{Kind: ConfigurationErrorUserinfo}
	}
	if parsed.RawQuery != "" || parsed.Fragment != "" {
		return nil, &ConfigurationError{Kind: ConfigurationErrorQueryOrFragment}
	}
	return parsed, nil
}

func isLoopbackHost(host string) bool {
	if strings.EqualFold(host, "localhost") {
		return true
	}
	ip := net.ParseIP(host)
	return ip != nil && ip.IsLoopback()
}

func (c *Client) Health(ctx context.Context) (Health, error) {
	var result Health
	if err := c.get(ctx, "/api/health", &result); err != nil { return Health{}, err }; return result, nil
}

func (c *Client) Services(ctx context.Context) (data []Service, resultErr error) {
	var observation *operatorResponseObservation;defer func(){resultErr=retainOperatorResponse(resultErr,observation)}()
	var result servicesResponse
	if err := c.getObserved(ctx, "/api/services", &result,&observation); err != nil {
		return nil, err
	}
	if result.Services == nil || len(result.Services) > 512 { return nil, &OperatorError{Stage: "response"} }
	seen := map[string]bool{}
	for _, service := range result.Services {
		if !serviceIDPattern.MatchString(service.ID) || seen[service.ID] { return nil, &OperatorError{Stage: "response"} }; seen[service.ID] = true
	}
	return result.Services, nil
}

func (c *Client) Capabilities(ctx context.Context) (Capabilities, error) {
	var result struct {
		Capabilities struct {
			Runtime struct {
				Version string `json:"version"`
			} `json:"runtime"`
			API struct {
				ContractVersion string `json:"contractVersion"`
			} `json:"api"`
		} `json:"capabilities"`
	}
	err := c.get(ctx, "/api/runtime/capabilities", &result)
	if err != nil { return Capabilities{}, err }
	return Capabilities{RuntimeVersion: result.Capabilities.Runtime.Version, ContractVersion: result.Capabilities.API.ContractVersion}, err
}

func (c *Client) SetupStatus(ctx context.Context) (SetupStatus, error) {
	var result struct {
		Setup struct {
			State     string `json:"state"`
			SetupMode bool   `json:"setupMode"`
			Vault     struct {
				Ready bool `json:"ready"`
			} `json:"vault"`
		} `json:"setup"`
	}
	err := c.get(ctx, "/api/setup/status", &result)
	if err != nil { return SetupStatus{}, err }
	return SetupStatus{State: result.Setup.State, SetupMode: result.Setup.SetupMode, VaultReady: result.Setup.Vault.Ready}, err
}

func (c *Client) RuntimeIdentity(ctx context.Context) (RuntimeIdentity, error) {
	var result struct {
		Instance *struct {
			Status string `json:"status"`
			Phase  string `json:"phase"`
		} `json:"instance"`
	}
	err := c.get(ctx, "/api/runtime/instance", &result)
	if err != nil { return RuntimeIdentity{}, err }
	if result.Instance == nil {
		return RuntimeIdentity{Status: "unknown", Phase: "unknown"}, err
	}
	return RuntimeIdentity{Status: result.Instance.Status, Phase: result.Instance.Phase}, err
}

func (c *Client) Inbox(ctx context.Context, cursor string) (data Inbox, resultErr error) {
	var observation *operatorResponseObservation;defer func(){resultErr=retainOperatorResponse(resultErr,observation)}()
	path := "/api/operator/inbox?limit=20"
	if cursor != "" {
		path += "&cursor=" + url.QueryEscape(cursor)
	}
	var result struct {
		Inbox struct {
			Items      []InboxItem `json:"items"`
			Pagination struct {
				Total      int     `json:"total"`
				NextCursor *string `json:"nextCursor"`
			} `json:"pagination"`
		} `json:"inbox"`
	}
	err := c.getObserved(ctx, path, &result,&observation)
	if err != nil { return Inbox{}, err }
	if result.Inbox.Items == nil || len(result.Inbox.Items) > 20 || result.Inbox.Pagination.Total < len(result.Inbox.Items) { return Inbox{}, &OperatorError{Stage: "response"} }
	next := ""
	if result.Inbox.Pagination.NextCursor != nil {
		next = *result.Inbox.Pagination.NextCursor
	}
	return Inbox{Items: result.Inbox.Items, Total: result.Inbox.Pagination.Total, NextCursor: next}, err
}

func (c *Client) HealthHistory(ctx context.Context, serviceID string) (data HealthHistory, resultErr error) {
	var observation *operatorResponseObservation;defer func(){resultErr=retainOperatorResponse(resultErr,observation)}()
	if !serviceIDPattern.MatchString(serviceID) {
		return HealthHistory{}, &OperatorError{Stage: "response"}
	}
	var result struct {
		ServiceID string `json:"serviceId"`
		History   struct {
			Transitions []json.RawMessage `json:"transitions"`
		} `json:"history"`
	}
	err := c.getObserved(ctx, "/api/services/"+url.PathEscape(serviceID)+"/health/history", &result,&observation)
	if err != nil { return HealthHistory{}, err }
	if result.ServiceID != serviceID || result.History.Transitions == nil || len(result.History.Transitions) > 256 { return HealthHistory{}, &OperatorError{Stage: "response"} }
	return HealthHistory{ServiceID: result.ServiceID, Entries: len(result.History.Transitions)}, err
}

func (c *Client) LifecycleAvailability(ctx context.Context, serviceID string) (data []LifecycleAvailability, resultErr error) {
	var observation *operatorResponseObservation;defer func(){resultErr=retainOperatorResponse(resultErr,observation)}()
	if !serviceIDPattern.MatchString(serviceID) {
		return nil, &OperatorError{Stage: "response"}
	}
	var result struct {
		ContractVersion string `json:"contractVersion"`
		ServiceID string `json:"serviceId"`
		Actions []struct {
			Action               string  `json:"action"`
			Available            *bool   `json:"available"`
			Reason               json.RawMessage `json:"reason"`
			Permission           string  `json:"permission"`
			RequiresConfirmation *bool   `json:"requiresConfirmation"`
		} `json:"actions"`
	}
	if err := c.requestObserved(ctx, http.MethodGet, "/api/operator/lifecycle/services/"+url.PathEscape(serviceID)+"/availability", nil, &result,&observation, 200); err != nil {
		return nil, err
	}
	if result.ContractVersion != "service-lasso-durable-lifecycle-operation.v1" || result.ServiceID != serviceID || (len(result.Actions) != 6 && len(result.Actions) != 8) { return nil, &OperatorError{Stage: "response"} }
	actions := make([]LifecycleAvailability, 0, len(result.Actions))
	seen := map[string]bool{}
	for _, entry := range result.Actions {
		extension := entry.Action == "update_check" || entry.Action == "update_download"
		if (!isDurableAction(entry.Action) && entry.Action != "reload" && !extension) || seen[entry.Action] || entry.Available == nil || entry.RequiresConfirmation == nil || len(entry.Reason) == 0 ||
			(entry.Action == "reload" && *entry.Available) ||
			entry.Permission != actionPermission(entry.Action) || *entry.RequiresConfirmation != (entry.Action != "update_check") {
			return nil, &OperatorError{Stage: "response"}
		}
		seen[entry.Action] = true
		reason := ""
		if string(entry.Reason) != "null" {
			if err:=json.Unmarshal(entry.Reason,&reason);err!=nil{return nil,&OperatorError{Stage:"response",cause:err}}
			if !safeIdentifier(reason, 96) {
				return nil, &OperatorError{Stage: "response"}
			}
		}
		if *entry.Available && reason != "" { return nil, &OperatorError{Stage: "response"} }
		// Never retain unknown reason identifiers as operator content. Availability
		// remains authoritative; unknown bounded reasons get a fixed safe label.
		if reason != "" && ReasonLabel(reason) == "Core action unavailable" { reason = "core_action_unavailable" }
		if !extension { actions = append(actions, LifecycleAvailability{Action: entry.Action, Available: *entry.Available, Reason: reason, Permission: entry.Permission, RequiresConfirmation: *entry.RequiresConfirmation}) }
	}
	for _, name := range []string{"install", "config", "start", "stop", "restart", "reload"} { if !seen[name] { return nil, &OperatorError{Stage: "response"} } }
	if seen["update_check"] != seen["update_download"] { return nil, &OperatorError{Stage: "response"} }
	return actions, nil
}

func (c *Client) PreviewLifecycle(ctx context.Context, serviceID, action string) (data LifecyclePreview, resultErr error) {
	var observation *operatorResponseObservation;defer func(){resultErr=retainOperatorResponse(resultErr,observation)}()
	if !serviceIDPattern.MatchString(serviceID) || !isDurableAction(action) {
		return LifecyclePreview{}, &OperatorError{Stage: "response"}
	}
	body, _ := json.Marshal(map[string]string{"action": action, "serviceId": serviceID})
	var result struct {
		ContractVersion string `json:"contractVersion"`
		Status string `json:"status"`
		OK *bool `json:"ok"`
		Action    string `json:"action"`
		Preflight struct {
			Executable *bool `json:"executable"`
			Targets []string `json:"targets"`
			Effects []string `json:"effects"`
		} `json:"preflight"`
		Confirmation struct {
			Required *bool `json:"required"`
			ID                 string `json:"id"`
			Status             string `json:"status"`
			ConfirmationPhrase string `json:"confirmationPhrase"`
		} `json:"confirmation"`
	}
	if err := c.requestObserved(ctx, http.MethodPost, "/api/operator/lifecycle/operations", bytes.NewReader(body), &result,&observation, 200); err != nil {
		return LifecyclePreview{}, err
	}
	expected := "service_" + action
	if action == "config" {
		expected = "service_configure"
	}
	if result.ContractVersion != "service-lasso-mcp-guarded-action.v1" || result.Status != "preflight" || result.OK == nil || !*result.OK || result.Preflight.Executable == nil || !*result.Preflight.Executable || result.Confirmation.Required == nil || !*result.Confirmation.Required || result.Action != expected || result.Confirmation.Status != "pending" || !safeOpaque(result.Confirmation.ID, "mcp-confirmation-", 80) || !safePhrase(result.Confirmation.ConfirmationPhrase) || !safeTargetEffects(result.Preflight.Targets, result.Preflight.Effects) || !containsTarget(result.Preflight.Targets, serviceID) {
		return LifecyclePreview{}, &OperatorError{Stage: "response"}
	}
	return LifecyclePreview{Action: action, ServiceID: serviceID, ConfirmationID: result.Confirmation.ID, ConfirmationPhrase: result.Confirmation.ConfirmationPhrase, Targets: result.Preflight.Targets, Effects: result.Preflight.Effects}, nil
}

func (c *Client) SubmitLifecycle(ctx context.Context, preview LifecyclePreview, idempotencyKey string) (data Operation, resultErr error) {
	var observation *operatorResponseObservation;defer func(){resultErr=retainOperatorResponse(resultErr,observation)}()
	if !serviceIDPattern.MatchString(preview.ServiceID) || !isDurableAction(preview.Action) || !safeOpaque(preview.ConfirmationID, "mcp-confirmation-", 80) || !safePhrase(preview.ConfirmationPhrase) || !safeOpaque(idempotencyKey, "tui-", 80) || !safeTargetEffects(preview.Targets, preview.Effects) || !containsTarget(preview.Targets, preview.ServiceID) {
		return Operation{}, &OperatorError{Stage: "response"}
	}
	body, _ := json.Marshal(map[string]any{"action": preview.Action, "serviceId": preview.ServiceID, "execute": true, "idempotencyKey": idempotencyKey, "confirmationId": preview.ConfirmationID, "confirmationPhrase": preview.ConfirmationPhrase})
	var result struct {
		ContractVersion string `json:"contractVersion"`
		Accepted  bool            `json:"accepted"`
		Operation json.RawMessage `json:"operation"`
	}
	if err := c.requestObserved(ctx, http.MethodPost, "/api/operator/lifecycle/operations", bytes.NewReader(body), &result,&observation, 202); err != nil {
		return Operation{}, err
	}
	if !result.Accepted || result.ContractVersion != "service-lasso-mcp-operation-accepted.v1" {
		return Operation{}, &OperatorError{Stage: "response"}
	}
	operation, err := parseOperation(result.Operation)
	if err != nil { return Operation{}, err }
	if !MatchOperation(operation, preview) || (!containsTarget(operation.Targets, preview.ServiceID) && operation.Status != "skipped") { return Operation{}, &OperatorError{Stage: "response"} }
	return operation, nil
}

func (c *Client) Operation(ctx context.Context, operationID string) (data Operation, resultErr error) {
	var observation *operatorResponseObservation;defer func(){resultErr=retainOperatorResponse(resultErr,observation)}()
	if !safeOpaque(operationID, "mcp-operation-", 80) {
		return Operation{}, &OperatorError{Stage: "response"}
	}
	var result struct {
		ContractVersion string `json:"contractVersion"`
		Operation json.RawMessage `json:"operation"`
	}
	if err := c.requestObserved(ctx, http.MethodGet, "/api/operator/lifecycle/operations/"+url.PathEscape(operationID), nil, &result,&observation, 200); err != nil {
		return Operation{}, err
	}
	if result.ContractVersion != "service-lasso-mcp-operation.v1" { return Operation{}, &OperatorError{Stage: "response"} }
	operation, err := parseOperation(result.Operation)
	if err == nil && operation.ID != operationID { return Operation{}, &OperatorError{Stage: "response"} }
	return operation, err
}

func (c *Client) CancelOperation(ctx context.Context, operationID string) (data Operation, resultErr error) {
	var observation *operatorResponseObservation;defer func(){resultErr=retainOperatorResponse(resultErr,observation)}()
	if !safeOpaque(operationID, "mcp-operation-", 80) {
		return Operation{}, &OperatorError{Stage: "response"}
	}
	var result struct {
		ContractVersion string `json:"contractVersion"`
		Operation json.RawMessage `json:"operation"`
		Cancellation struct {
			Result string `json:"result"`
			Terminal *bool `json:"terminal"`
		} `json:"cancellation"`
	}
	if err := c.requestObserved(ctx, http.MethodPost, "/api/operator/lifecycle/operations/"+url.PathEscape(operationID)+"/cancel", bytes.NewBufferString("{}"), &result,&observation, 200); err != nil {
		return Operation{}, err
	}
	if result.ContractVersion != "service-lasso-mcp-operation.v1" || result.Cancellation.Terminal == nil { return Operation{}, &OperatorError{Stage: "response"} }
	operation, err := parseOperation(result.Operation)
	if err != nil { return Operation{}, err }
	resultName := result.Cancellation.Result
	if operation.ID != operationID || (resultName != "requested" && resultName != "unsupported" && resultName != "too_late") || *result.Cancellation.Terminal != terminalStatus(operation.Status) || (resultName == "requested" && (!operation.CancellationSupported || (operation.Status != "cancelling" && operation.Status != "cancelled"))) { return Operation{}, &OperatorError{Stage: "response"} }
	operation.CancellationResult, operation.CancellationTerminal = resultName, *result.Cancellation.Terminal
	return operation, nil
}

func NewIdempotencyKey() (string, error) {
	var value [16]byte
	if _, err := rand.Read(value[:]); err != nil {
		return "", &OperatorError{Stage: "submission", cause: err}
	}
	return fmt.Sprintf("tui-%x", value), nil
}

func isDurableAction(value string) bool {
	return value == "install" || value == "config" || value == "start" || value == "stop" || value == "restart"
}
func safeOpaque(value, prefix string, limit int) bool {
	if !strings.HasPrefix(value, prefix) || len(value) > limit { return false }
	suffix := strings.TrimPrefix(value, prefix)
	if prefix == "tui-" { return regexp.MustCompile(`^[a-f0-9]{32}$`).MatchString(suffix) }
	return regexp.MustCompile(`^[a-f0-9]{8}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{12}$`).MatchString(suffix)
}
func safeIdentifier(value string, limit int) bool {
	if value == "" || len(value) > limit {
		return false
	}
	for _, r := range value {
		if !(r >= 'a' && r <= 'z' || r >= 'A' && r <= 'Z' || r >= '0' && r <= '9' || r == '-' || r == '_' || r == ':' || r == '.') {
			return false
		}
	}
	return true
}
func safePhrase(value string) bool {
	return len(value) >= 10 && len(value) <= 200 && !strings.ContainsAny(value, "\r\n\x1b")
}
func safeTargetEffects(targets, effects []string) bool {
	if len(targets) == 0 || len(targets) > 100 || len(effects) == 0 || len(effects) > 100 {
		return false
	}
	seen := map[string]bool{}
	for _, value := range targets {
		if !serviceIDPattern.MatchString(value) || seen[value] {
			return false
		}
		seen[value] = true
	}
	for _, effect := range effects {
		// Core's preflight effects are operator-facing descriptions, not
		// identifiers. Core admits 300 UTF-16 units, bounded by 1,200 UTF-8
		// bytes. Preserve its Unicode descriptions and reject terminal controls.
		if len(effect) == 0 || len(effect) > 1200 || !utf8.ValidString(effect) {
			return false
		}
		units:=0
		for _, character := range effect {
			units+=utf16.RuneLen(character)
			if unicode.IsControl(character) || units>300 {
				return false
			}
		}
	}
	return true
}
func parseOperation(raw json.RawMessage) (Operation, error) {
	var value struct {
		OperationID           string  `json:"operationId"`
		Action                string  `json:"action"`
		Status                string  `json:"status"`
		Phase                 string  `json:"phase"`
		Progress              *int    `json:"progress"`
		Outcome               json.RawMessage `json:"outcome"`
		CancellationSupported *bool   `json:"cancellationSupported"`
		Ownership             string  `json:"ownership"`
		Targets               []string `json:"targetIds"`
	}
	if err:=decodeResponse(raw,&value);err!=nil{return Operation{},err}
	if !safeOpaque(value.OperationID, "mcp-operation-", 80) || !knownOperationAction(value.Action) || !knownStatus(value.Status) || !regexp.MustCompile(`^[a-z][a-z0-9_]{0,63}$`).MatchString(value.Phase) || value.Progress == nil || *value.Progress < 0 || *value.Progress > 100 || len(value.Outcome) == 0 || value.CancellationSupported == nil || value.Ownership != "own" || !validTargets(value.Targets) {
		return Operation{}, &OperatorError{Stage: "response"}
	}
	outcome := ""
	if string(value.Outcome) != "null" {
		if err:=json.Unmarshal(value.Outcome,&outcome);err!=nil{return Operation{},&OperatorError{Stage:"response",cause:err}}
		if !terminalStatus(outcome) { return Operation{}, &OperatorError{Stage: "response"} }
	}
	if terminalStatus(value.Status) != (outcome != "") || (outcome != "" && (value.Status != outcome || *value.Progress != 100)) { return Operation{}, &OperatorError{Stage: "response"} }
	return Operation{ID: value.OperationID, Action: value.Action, Status: value.Status, Phase: value.Phase, Progress: *value.Progress, Outcome: outcome, CancellationSupported: *value.CancellationSupported, Ownership: value.Ownership, Targets: value.Targets}, nil
}

func containsTarget(targets []string, id string) bool { for _, target := range targets { if target == id { return true } }; return false }
func validTargets(targets []string) bool { if targets == nil || len(targets) > 100 { return false }; seen := map[string]bool{}; for _, id := range targets { if !serviceIDPattern.MatchString(id) || seen[id] { return false }; seen[id] = true }; return true }
func knownOperationAction(action string) bool { for _, name := range []string{"install", "config", "start", "stop", "restart"} { if action == coreAction(name) { return true } }; return action == "update_check" || action == "update_download" }
func knownStatus(status string) bool { return terminalStatus(status) || status == "queued" || status == "running" || status == "cancelling" }
func actionPermission(action string) string { switch action { case "install", "config": return "service-lasso:config:write"; case "update_check", "update_download": return "service-lasso:update:write"; default: return "service-lasso:lifecycle:write" } }

func (c *Client) get(ctx context.Context, path string, destination any) error {
	return c.request(ctx, http.MethodGet, path, nil, destination)
}
func (c *Client) getObserved(ctx context.Context,path string,destination any,observed **operatorResponseObservation)error{
 return c.requestObserved(ctx,http.MethodGet,path,nil,destination,observed)
}

func (c *Client) request(ctx context.Context,method,path string,body io.Reader,destination any,expectedStatus ...int)error{
 return c.requestObserved(ctx,method,path,body,destination,nil,expectedStatus...)
}
func (c *Client) requestObserved(ctx context.Context, method, path string, body io.Reader, destination any, observed **operatorResponseObservation, expectedStatus ...int) (resultErr error) {
	occurrence,enrollmentErr:=c.beginRequest(ctx,method,path,body);if enrollmentErr!=nil{return enrollmentErr};occurrence.Source=body;occurrence.Destination=destination;defer func(){occurrence.Result=destination;occurrence.Failure=resultErr}()
 if observed!=nil{*observed=nil}
	req, err := http.NewRequestWithContext(ctx, method, c.baseURL+path, body)
 occurrence.Request=req
	if err != nil {
		return &OperatorError{Stage: "configuration", cause: err}
	}
	if body != nil {
		req.Header.Set("Content-Type", "application/json")
	}
	if c.operatorToken != "" && c.authMode == AuthModeLocalAdmin {
		req.Header.Set("x-service-lasso-admin-token", c.operatorToken)
	} else if c.operatorToken != "" && c.authMode == AuthModeOAuthBearer {
		req.Header.Set("Authorization", "Bearer "+c.operatorToken)
	}
	response, err := c.http.Do(req)
 occurrence.Response=response
	if err != nil {
		return operatorTransportFailure("transport",response,err)
	}
	observation:=&operatorResponseObservation{Response:response,Body:response.Body,Status:response.StatusCode,Limit:maxResponseBytes}
	occurrence.Observation=observation
 if observed!=nil{*observed=observation}
	defer func(){observation.CloseAttempted=true;closeErr:=response.Body.Close();observation.CloseError=closeErr;if closeErr!=nil{resultErr=&OperatorError{Stage:"response",cause:errors.Join(resultErr,closeErr)}};resultErr=retainOperatorResponse(resultErr,observation)}()
	observation.ReadAttempted=true;raw,readErr:=io.ReadAll(io.LimitReader(response.Body,maxResponseBytes+1));observation.Raw=raw;observation.ReadError=readErr

	if response.StatusCode < http.StatusOK || response.StatusCode >= http.StatusMultipleChoices {
		failure:=safeHTTPError(method,path,response.StatusCode)
		if readErr!=nil{return &OperatorError{Stage:"response",cause:errors.Join(failure,readErr)}}
		return failure
	}
	if readErr!=nil{return &OperatorError{Stage:"response",cause:readErr}}
	if len(expectedStatus) > 0 && response.StatusCode != expectedStatus[0] { return &OperatorError{Stage: "response"} }
	return decodeResponse(raw, destination)
}

// safeHTTPError intentionally excludes a runtime response body. Core error
// payloads and reason phrases can include diagnostics or accidental sensitive
// material, while the request method, path, and canonical HTTP status remain
// useful to the operator.
func safeHTTPError(method, path string, statusCode int) error {
	route := "runtime"
	switch { case strings.HasPrefix(path,"/api/operator/lifecycle/"): route="lifecycle"; case strings.HasPrefix(path,"/api/services"): route="services"; case strings.HasPrefix(path,"/api/health"):route="health"; case strings.HasPrefix(path,"/api/operator/inbox"):route="inbox";case strings.HasPrefix(path,"/api/setup/"):route="setup" }
	return &OperatorError{Stage: "http", Status: statusCode, Method:method, Route:route}
}
