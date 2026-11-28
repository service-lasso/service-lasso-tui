package api

import (
	"bytes"
	"context"
	"encoding/json"
	"net/http"
	"net/url"
	"time"
)

type PinnedReleaseCheck struct {
	ServiceID    string  `json:"serviceId"`
	CurrentTag   string  `json:"currentTag"`
	Status       string  `json:"status"`
	CandidateTag *string `json:"candidateTag"`
}
type PinnedReleaseInstall struct {
	ServiceID    string `json:"serviceId"`
	InstalledTag string `json:"installedTag"`
}

func pinnedService(id string) bool { return id == "@tui" || id == "@cli" }
func pinnedTag(tag string) bool    { return tag != "" && operatorDisplay(tag, 200) }
func (c *Client) CheckPinnedRelease(ctx context.Context, id string) (PinnedReleaseCheck, error) {
	var result PinnedReleaseCheck
	if !pinnedService(id) {
		return result, &OperatorPublicError{Code: "invalid_request"}
	}
	err := c.pinnedReleaseRequest(ctx, id, "check", struct{}{}, &result)
	if err == nil && (result.ServiceID != id || !pinnedTag(result.CurrentTag) || !operatorEnum(result.Status, "current", "update_available") || (result.Status == "update_available" && (result.CandidateTag == nil || !pinnedTag(*result.CandidateTag) || *result.CandidateTag == result.CurrentTag)) || (result.Status == "current" && result.CandidateTag != nil)) {
		err = &OperatorError{Stage: "response"}
	}
	return result, err
}
func (c *Client) InstallPinnedRelease(ctx context.Context, id, tag string) (PinnedReleaseInstall, error) {
	var result PinnedReleaseInstall
	if !pinnedService(id) || !pinnedTag(tag) {
		return result, &OperatorPublicError{Code: "invalid_request"}
	}
	err := c.pinnedReleaseRequest(ctx, id, "install", struct {
		Tag     string `json:"tag"`
		Confirm bool   `json:"confirm"`
	}{tag, true}, &result)
	if err == nil && (result.ServiceID != id || result.InstalledTag != tag) {
		err = &OperatorError{Stage: "response"}
	}
	return result, err
}
func (c *Client) pinnedReleaseRequest(ctx context.Context, id, action string, body, result any) (resultErr error) {
	path := "/api/services/" + url.PathEscape(id) + "/pinned-release/" + action
	occurrence, err := c.beginRequest(ctx, http.MethodPost, path, body)
	if err != nil {
		return err
	}
	defer func() { occurrence.Result = result; occurrence.Failure = resultErr }()
	raw, err := json.Marshal(body)
	if err != nil {
		return &OperatorError{Stage: "invalid_request", cause: err}
	}
	occurrence.Encoded = raw
	source := bytes.NewReader(raw)
	occurrence.Source = source
	request, err := http.NewRequestWithContext(ctx, http.MethodPost, c.baseURL+path, source)
	occurrence.Request = request
	if err != nil {
		return &OperatorError{Stage: "configuration", cause: err}
	}
	request.Header.Set("Content-Type", "application/json")
	if c.authMode == AuthModeLocalAdmin {
		request.Header.Set("x-service-lasso-admin-token", c.operatorToken)
	} else {
		request.Header.Set("Authorization", "Bearer "+c.operatorToken)
	}
	client := *c.http
	client.Timeout = 55 * time.Second
	if action == "install" {
		client.Timeout = 120 * time.Second
	}
	response, err := client.Do(request)
	occurrence.Response = response
	if err != nil {
		return operatorTransportFailure("transport", response, err)
	}
	occurrence.Observation = &operatorResponseObservation{}
	raw, err, observation := readObservedResponse(occurrence.Observation, response, OperatorResponseBytes)
	defer func() { resultErr = retainOperatorResponse(resultErr, observation) }()
	if err != nil {
		return &OperatorError{Stage: "response", cause: err}
	}
	if response.StatusCode == http.StatusNotFound || response.StatusCode == http.StatusMethodNotAllowed {
		return &OperatorPublicError{Code: "unsupported_capability"}
	}
	if response.StatusCode == http.StatusUnauthorized {
		return &OperatorPublicError{Code: "authentication_required"}
	}
	if response.StatusCode == http.StatusForbidden {
		return &OperatorPublicError{Code: "permission_denied"}
	}
	if response.StatusCode != http.StatusOK || len(raw) > OperatorResponseBytes {
		return &OperatorError{Stage: "response"}
	}
	return decodeOperator(raw, result, OperatorResponseBytes)
}
