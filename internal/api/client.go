package api

import (
	"bytes"
	"context"
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

// LifecycleResult is the durable result returned by Core after a lifecycle
// request completes. The detailed state remains Core-owned.
type LifecycleResult struct {
	OK        bool   `json:"ok"`
	Action    string `json:"action"`
	ServiceID string `json:"serviceId"`
	Message   string `json:"message"`
}

func NewClient(baseURL string, client *http.Client, operatorToken string) (*Client, error) {
	parsed, err := url.Parse(baseURL)
	if err != nil || parsed.Scheme == "" || parsed.Host == "" {
		return nil, fmt.Errorf("invalid Service Lasso API URL %q", baseURL)
	}
	if parsed.Scheme != "http" && parsed.Scheme != "https" {
		return nil, fmt.Errorf("Service Lasso API URL must use HTTP or HTTPS")
	}
	if parsed.User != nil {
		return nil, fmt.Errorf("Service Lasso API URL must not contain userinfo")
	}
	if parsed.RawQuery != "" || parsed.Fragment != "" {
		return nil, fmt.Errorf("Service Lasso API URL must not contain a query or fragment")
	}
	if operatorToken != "" && parsed.Scheme != "https" && !isLoopbackHost(parsed.Hostname()) {
		return nil, fmt.Errorf("operator token requires HTTPS for a non-loopback Service Lasso API URL")
	}
	if client == nil {
		client = &http.Client{Timeout: requestTimeout}
	}
	isolatedClient := *client
	isolatedClient.CheckRedirect = func(_ *http.Request, _ []*http.Request) error {
		return http.ErrUseLastResponse
	}
	return &Client{baseURL: strings.TrimRight(parsed.String(), "/"), http: &isolatedClient, operatorToken: operatorToken}, nil
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

// Lifecycle asks Core to perform one documented lifecycle action. Core remains
// responsible for authorization, confirmation enforcement, auditing, and the
// actual mutation. This client never retries a mutation automatically.
func (c *Client) Lifecycle(ctx context.Context, serviceID, action string) (LifecycleResult, error) {
	var result LifecycleResult
	if !serviceIDPattern.MatchString(serviceID) {
		return result, fmt.Errorf("invalid service ID")
	}
	allowed := map[string]bool{
		"install": true, "config": true, "start": true, "stop": true, "restart": true, "reload": true,
	}
	if !allowed[action] {
		return result, fmt.Errorf("unsupported lifecycle action %q", action)
	}
	body, err := json.Marshal(struct {
		Confirm bool `json:"confirm"`
	}{Confirm: true})
	if err != nil {
		return result, fmt.Errorf("encode lifecycle confirmation: %w", err)
	}
	path := "/api/services/" + url.PathEscape(serviceID) + "/" + action
	return result, c.request(ctx, http.MethodPost, path, bytes.NewReader(body), &result)
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
	if c.operatorToken != "" {
		req.Header.Set("x-service-lasso-admin-token", c.operatorToken)
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
