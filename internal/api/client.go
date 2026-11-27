package api

import (
	"net"
	"net/http"
	"net/url"
	"strings"
	"time"

	"github.com/service-lasso/service-lasso-tui/internal/lifetime"
)

const requestTimeout = 5 * time.Second

// Client consumes Service Lasso runtime HTTP contracts.
type Client struct {
	receiver      *lifetime.Receiver
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
	ConfigurationErrorMissingCredential       ConfigurationErrorKind = "missing_credential"
	ConfigurationErrorInvalidURL              ConfigurationErrorKind = "invalid_url"
	ConfigurationErrorUnsupportedScheme       ConfigurationErrorKind = "unsupported_scheme"
	ConfigurationErrorUserinfo                ConfigurationErrorKind = "userinfo"
	ConfigurationErrorQueryOrFragment         ConfigurationErrorKind = "query_or_fragment"
	ConfigurationErrorInsecureTokenTransport  ConfigurationErrorKind = "insecure_token_transport"
	ConfigurationErrorRemoteProfileAdmission  ConfigurationErrorKind = "remote_profile_admission"
	ConfigurationErrorOperatorReaderAdmission ConfigurationErrorKind = "operator_reader_admission"
)

type ConfigurationError struct {
	Kind  ConfigurationErrorKind
	cause error
	input *connectionInputObservation
}

func (e *ConfigurationError) Unwrap() error { return e.cause }

func (e *ConfigurationError) Error() string {
	switch e.Kind {
	case ConfigurationErrorMissingCredential:
		return "connection configuration unavailable; check profile settings. Load your local admin token in this terminal and retry."
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

func NewClient(baseURL string, client *http.Client, operatorToken string) (*Client, error) {
	return NewClientWithAuth(baseURL, client, operatorToken, AuthModeLocalAdmin)
}

func NewClientWithAuth(baseURL string, client *http.Client, operatorToken string, authMode AuthMode, scopes ...string) (*Client, error) {
	return newClientWithAuth(baseURL, client, operatorToken, authMode, false, scopes...)
}

func NewOperatorClientWithAuth(baseURL string, client *http.Client, operatorToken string, authMode AuthMode, scopes ...string) (*Client, error) {
	return newClientWithAuth(baseURL, client, operatorToken, authMode, true, scopes...)
}

func newClientWithAuth(baseURL string, client *http.Client, operatorToken string, authMode AuthMode, operatorV2 bool, scopes ...string) (*Client, error) {
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
	if !isLoopbackHost(parsed.Hostname()) && (authMode != AuthModeOAuthBearer || !hasConnectionScopes(scopes, operatorV2)) {
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
	if err != nil {
		return nil, &ConfigurationError{Kind: ConfigurationErrorInvalidURL, cause: err}
	}
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

// safeHTTPError intentionally excludes a runtime response body.
func safeHTTPError(method, path string, statusCode int) error {
	route := "runtime"
	switch {
	case strings.HasPrefix(path, "/api/operator/v2"):
		route = "operator"
	case strings.HasPrefix(path, "/api/services"):
		route = "services"
	case strings.HasPrefix(path, "/api/health"):
		route = "health"
	case strings.HasPrefix(path, "/api/operator/inbox"):
		route = "inbox"
	case strings.HasPrefix(path, "/api/setup/"):
		route = "setup"
	}
	return &OperatorError{Stage: "http", Status: statusCode, Method: method, Route: route}
}
