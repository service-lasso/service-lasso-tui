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
	"strings"
	"time"
)

const requestTimeout = 5 * time.Second

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
	if parsed.User != nil {
		return nil, fmt.Errorf("Service Lasso API URL must not contain userinfo")
	}
	if operatorToken != "" && parsed.Scheme != "https" && !isLoopbackHost(parsed.Hostname()) {
		return nil, fmt.Errorf("operator token requires HTTPS for a non-loopback Service Lasso API URL")
	}
	if client == nil {
		client = &http.Client{Timeout: requestTimeout}
	}
	return &Client{baseURL: strings.TrimRight(parsed.String(), "/"), http: client, operatorToken: operatorToken}, nil
}

func isLoopbackHost(host string) bool {
	if host == "localhost" {
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

// Lifecycle asks Core to perform one documented lifecycle action. Core remains
// responsible for authorization, confirmation enforcement, auditing, and the
// actual mutation. This client never retries a mutation automatically.
func (c *Client) Lifecycle(ctx context.Context, serviceID, action string) (LifecycleResult, error) {
	var result LifecycleResult
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
		// Error bodies can contain server diagnostics or accidental sensitive
		// material. Keep them out of the UI and retained test evidence.
		return fmt.Errorf("request %s %s: runtime returned %s", method, path, response.Status)
	}
	if err := json.NewDecoder(response.Body).Decode(destination); err != nil {
		return fmt.Errorf("decode %s %s: %w", method, path, err)
	}
	return nil
}
