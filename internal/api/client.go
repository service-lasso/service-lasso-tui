package api

import (
	"bytes"
	"context"
	"crypto/rand"
	"encoding/json"
	"fmt"
	"io"
	"net"
	"net/http"
	"net/url"
	"regexp"
	"strconv"
	"strings"
	"time"
)

const requestTimeout = 5 * time.Second

var serviceIDPattern = regexp.MustCompile(`^@?[A-Za-z0-9][A-Za-z0-9._-]{0,127}$`)

// Client consumes documented Service Lasso runtime HTTP contracts.
type Client struct {
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
// without retaining the supplied URL or operator token.
type ConfigurationErrorKind string

const (
	ConfigurationErrorInvalidURL             ConfigurationErrorKind = "invalid_url"
	ConfigurationErrorUnsupportedScheme      ConfigurationErrorKind = "unsupported_scheme"
	ConfigurationErrorUserinfo               ConfigurationErrorKind = "userinfo"
	ConfigurationErrorQueryOrFragment        ConfigurationErrorKind = "query_or_fragment"
	ConfigurationErrorInsecureTokenTransport ConfigurationErrorKind = "insecure_token_transport"
	ConfigurationErrorRemoteProfileAdmission ConfigurationErrorKind = "remote_profile_admission"
)

// ConfigurationError reports only the closed validation class.
type ConfigurationError struct {
	Kind ConfigurationErrorKind
}

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
// which are deliberately not retained by this attached-terminal client.
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
}

func NewClient(baseURL string, client *http.Client, operatorToken string) (*Client, error) {
	return NewClientWithAuth(baseURL, client, operatorToken, AuthModeLocalAdmin)
}

// NewClientWithAuth admits a non-loopback runtime only when its caller
// explicitly declares the Core OAuth mode and both lifecycle scopes. The
// declaration is a local configuration guard; Core validates the bearer token
// and authorizes every request.
func NewClientWithAuth(baseURL string, client *http.Client, operatorToken string, authMode AuthMode, scopes ...string) (*Client, error) {
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
	if !isLoopbackHost(parsed.Hostname()) && (authMode != AuthModeOAuthBearer || !hasRequiredScopes(scopes)) {
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
	if err != nil || parsed.Scheme == "" || parsed.Host == "" {
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
	return result, c.get(ctx, "/api/health", &result)
}

func (c *Client) Services(ctx context.Context) ([]Service, error) {
	var result servicesResponse
	if err := c.get(ctx, "/api/services", &result); err != nil {
		return nil, err
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
	if result.Instance == nil {
		return RuntimeIdentity{Status: "unknown", Phase: "unknown"}, err
	}
	return RuntimeIdentity{Status: result.Instance.Status, Phase: result.Instance.Phase}, err
}

func (c *Client) Inbox(ctx context.Context, cursor string) (Inbox, error) {
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
	err := c.get(ctx, path, &result)
	next := ""
	if result.Inbox.Pagination.NextCursor != nil {
		next = *result.Inbox.Pagination.NextCursor
	}
	return Inbox{Items: result.Inbox.Items, Total: result.Inbox.Pagination.Total, NextCursor: next}, err
}

func (c *Client) HealthHistory(ctx context.Context, serviceID string) (HealthHistory, error) {
	if !serviceIDPattern.MatchString(serviceID) {
		return HealthHistory{}, fmt.Errorf("invalid service ID")
	}
	var result struct {
		ServiceID string `json:"serviceId"`
		History   struct {
			Transitions []json.RawMessage `json:"transitions"`
		} `json:"history"`
	}
	err := c.get(ctx, "/api/services/"+url.PathEscape(serviceID)+"/health/history", &result)
	return HealthHistory{ServiceID: result.ServiceID, Entries: len(result.History.Transitions)}, err
}

func (c *Client) LifecycleAvailability(ctx context.Context, serviceID string) ([]LifecycleAvailability, error) {
	if !serviceIDPattern.MatchString(serviceID) {
		return nil, fmt.Errorf("invalid service ID")
	}
	var result struct {
		Actions []struct {
			Action               string  `json:"action"`
			Available            bool    `json:"available"`
			Reason               *string `json:"reason"`
			Permission           string  `json:"permission"`
			RequiresConfirmation bool    `json:"requiresConfirmation"`
		} `json:"actions"`
	}
	if err := c.get(ctx, "/api/operator/lifecycle/services/"+url.PathEscape(serviceID)+"/availability", &result); err != nil {
		return nil, err
	}
	actions := make([]LifecycleAvailability, 0, len(result.Actions))
	for _, entry := range result.Actions {
		if !isDurableAction(entry.Action) || !safeIdentifier(entry.Permission, 96) {
			return nil, fmt.Errorf("invalid lifecycle availability response")
		}
		reason := ""
		if entry.Reason != nil {
			if !safeIdentifier(*entry.Reason, 96) {
				return nil, fmt.Errorf("invalid lifecycle availability response")
			}
			reason = *entry.Reason
		}
		actions = append(actions, LifecycleAvailability{Action: entry.Action, Available: entry.Available, Reason: reason, Permission: entry.Permission, RequiresConfirmation: entry.RequiresConfirmation})
	}
	return actions, nil
}

func (c *Client) PreviewLifecycle(ctx context.Context, serviceID, action string) (LifecyclePreview, error) {
	if !serviceIDPattern.MatchString(serviceID) || !isDurableAction(action) {
		return LifecyclePreview{}, fmt.Errorf("invalid lifecycle preview")
	}
	body, _ := json.Marshal(map[string]string{"action": action, "serviceId": serviceID})
	var result struct {
		Action    string `json:"action"`
		Preflight struct {
			Targets []string `json:"targets"`
			Effects []string `json:"effects"`
		} `json:"preflight"`
		Confirmation struct {
			ID                 string `json:"id"`
			Status             string `json:"status"`
			ConfirmationPhrase string `json:"confirmationPhrase"`
		} `json:"confirmation"`
	}
	if err := c.request(ctx, http.MethodPost, "/api/operator/lifecycle/operations", bytes.NewReader(body), &result); err != nil {
		return LifecyclePreview{}, err
	}
	expected := "service_" + action
	if action == "config" {
		expected = "service_configure"
	}
	if result.Action != expected || result.Confirmation.Status != "pending" || !safeOpaque(result.Confirmation.ID, "mcp-confirmation-", 80) || !safePhrase(result.Confirmation.ConfirmationPhrase) || !safeTargetEffects(result.Preflight.Targets, result.Preflight.Effects) {
		return LifecyclePreview{}, fmt.Errorf("invalid lifecycle preview response")
	}
	return LifecyclePreview{Action: action, ServiceID: serviceID, ConfirmationID: result.Confirmation.ID, ConfirmationPhrase: result.Confirmation.ConfirmationPhrase, Targets: result.Preflight.Targets, Effects: result.Preflight.Effects}, nil
}

func (c *Client) SubmitLifecycle(ctx context.Context, preview LifecyclePreview, idempotencyKey string) (Operation, error) {
	if !serviceIDPattern.MatchString(preview.ServiceID) || !isDurableAction(preview.Action) || !safeOpaque(preview.ConfirmationID, "mcp-confirmation-", 80) || !safePhrase(preview.ConfirmationPhrase) || !safeOpaque(idempotencyKey, "tui-", 80) {
		return Operation{}, fmt.Errorf("invalid frozen lifecycle submission")
	}
	body, _ := json.Marshal(map[string]any{"action": preview.Action, "serviceId": preview.ServiceID, "execute": true, "idempotencyKey": idempotencyKey, "confirmationId": preview.ConfirmationID, "confirmationPhrase": preview.ConfirmationPhrase})
	var result struct {
		Accepted  bool            `json:"accepted"`
		Operation json.RawMessage `json:"operation"`
	}
	if err := c.request(ctx, http.MethodPost, "/api/operator/lifecycle/operations", bytes.NewReader(body), &result); err != nil {
		return Operation{}, err
	}
	if !result.Accepted {
		return Operation{}, fmt.Errorf("invalid lifecycle submission response")
	}
	return parseOperation(result.Operation)
}

func (c *Client) Operation(ctx context.Context, operationID string) (Operation, error) {
	if !safeOpaque(operationID, "mcp-operation-", 80) {
		return Operation{}, fmt.Errorf("invalid operation ID")
	}
	var result struct {
		Operation json.RawMessage `json:"operation"`
	}
	if err := c.get(ctx, "/api/operator/lifecycle/operations/"+url.PathEscape(operationID), &result); err != nil {
		return Operation{}, err
	}
	return parseOperation(result.Operation)
}

func (c *Client) CancelOperation(ctx context.Context, operationID string) (Operation, error) {
	if !safeOpaque(operationID, "mcp-operation-", 80) {
		return Operation{}, fmt.Errorf("invalid operation ID")
	}
	var result struct {
		Cancellation struct {
			Operation json.RawMessage `json:"operation"`
		} `json:"cancellation"`
	}
	if err := c.request(ctx, http.MethodPost, "/api/operator/lifecycle/operations/"+url.PathEscape(operationID)+"/cancel", bytes.NewBufferString("{}"), &result); err != nil {
		return Operation{}, err
	}
	return parseOperation(result.Cancellation.Operation)
}

func NewIdempotencyKey() (string, error) {
	var value [16]byte
	if _, err := rand.Read(value[:]); err != nil {
		return "", fmt.Errorf("create idempotency key: %w", err)
	}
	return fmt.Sprintf("tui-%x", value), nil
}

func isDurableAction(value string) bool {
	return value == "install" || value == "config" || value == "start" || value == "stop" || value == "restart"
}
func safeOpaque(value, prefix string, limit int) bool {
	return strings.HasPrefix(value, prefix) && safeIdentifier(value, limit)
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
	if len(targets) == 0 || len(targets) > 100 || len(effects) > 100 {
		return false
	}
	for _, value := range append(append([]string{}, targets...), effects...) {
		if !safeIdentifier(value, 128) {
			return false
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
		Progress              int     `json:"progress"`
		Outcome               *string `json:"outcome"`
		CancellationSupported bool    `json:"cancellationSupported"`
		Ownership             string  `json:"ownership"`
	}
	if len(raw) == 0 || json.Unmarshal(raw, &value) != nil || !safeOpaque(value.OperationID, "mcp-operation-", 80) || !safeIdentifier(value.Action, 96) || !safeIdentifier(value.Status, 96) || !safeIdentifier(value.Phase, 96) || value.Progress < 0 || value.Progress > 100 || (value.Outcome != nil && !safeIdentifier(*value.Outcome, 96)) || (value.Ownership != "" && value.Ownership != "own" && value.Ownership != "other") {
		return Operation{}, fmt.Errorf("invalid lifecycle operation response")
	}
	outcome := ""
	if value.Outcome != nil {
		outcome = *value.Outcome
	}
	return Operation{ID: value.OperationID, Action: value.Action, Status: value.Status, Phase: value.Phase, Progress: value.Progress, Outcome: outcome, CancellationSupported: value.CancellationSupported, Ownership: value.Ownership}, nil
}

func (c *Client) get(ctx context.Context, path string, destination any) error {
	return c.request(ctx, http.MethodGet, path, nil, destination)
}

func (c *Client) request(ctx context.Context, method, path string, body io.Reader, destination any) error {
	req, err := http.NewRequestWithContext(ctx, method, c.baseURL+path, body)
	if err != nil {
		return err
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
	if err != nil {
		return fmt.Errorf("request %s %s: %w", method, path, err)
	}
	defer response.Body.Close()

	if response.StatusCode < http.StatusOK || response.StatusCode >= http.StatusMultipleChoices {
		return safeHTTPError(method, path, response.StatusCode)
	}
	if err := json.NewDecoder(response.Body).Decode(destination); err != nil {
		return fmt.Errorf("decode %s %s: %w", method, path, err)
	}
	return nil
}

// safeHTTPError intentionally excludes a runtime response body. Core error
// payloads and reason phrases can include diagnostics or accidental sensitive
// material, while the request method, path, and canonical HTTP status remain
// useful to the operator.
func safeHTTPError(method, path string, statusCode int) error {
	status := strconv.Itoa(statusCode)
	if text := http.StatusText(statusCode); text != "" {
		status += " " + text
	}
	return fmt.Errorf("request %s %s: runtime returned %s", method, path, status)
}
