package api

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"net/url"
	"strings"
	"time"
)

const requestTimeout = 5 * time.Second

// Client reads the stable runtime surfaces used by the first TUI foundation.
// Mutating runtime operations intentionally remain outside this first contract.
type Client struct {
	baseURL string
	http    *http.Client
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

func NewClient(baseURL string, client *http.Client) (*Client, error) {
	parsed, err := url.Parse(baseURL)
	if err != nil || parsed.Scheme == "" || parsed.Host == "" {
		return nil, fmt.Errorf("invalid Service Lasso API URL %q", baseURL)
	}
	if client == nil {
		client = &http.Client{Timeout: requestTimeout}
	}
	return &Client{baseURL: strings.TrimRight(parsed.String(), "/"), http: client}, nil
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

func (c *Client) get(ctx context.Context, path string, destination any) error {
	req, err := http.NewRequestWithContext(ctx, http.MethodGet, c.baseURL+path, nil)
	if err != nil {
		return err
	}
	response, err := c.http.Do(req)
	if err != nil {
		return fmt.Errorf("request %s: %w", path, err)
	}
	defer response.Body.Close()

	if response.StatusCode < http.StatusOK || response.StatusCode >= http.StatusMultipleChoices {
		body, _ := io.ReadAll(io.LimitReader(response.Body, 4096))
		return fmt.Errorf("request %s: runtime returned %s: %s", path, response.Status, strings.TrimSpace(string(body)))
	}
	if err := json.NewDecoder(response.Body).Decode(destination); err != nil {
		return fmt.Errorf("decode %s: %w", path, err)
	}
	return nil
}
