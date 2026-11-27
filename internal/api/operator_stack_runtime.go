package api

import (
	"context"
	"net/url"
	"time"
)

type OperatorStackOperations struct {
	Scope string `json:"scope"`
	Count int64  `json:"count"`
}

type OperatorStackServiceRuntime struct {
	ServiceID    string `json:"serviceId"`
	UptimeMS     *int64 `json:"uptimeMs"`
	RestartCount *int64 `json:"restartCount"`
}

type OperatorStackIncident struct {
	At        string `json:"at"`
	ServiceID string `json:"serviceId"`
	Kind      string `json:"kind"`
}

type OperatorStackRuntimeFrame struct {
	ObservedAt       string                        `json:"observedAt"`
	CoreUptimeMS     *int64                        `json:"coreUptimeMs"`
	ActiveOperations *OperatorStackOperations      `json:"activeOperations"`
	Rows             []OperatorStackServiceRuntime `json:"rows"`
	Incidents        *[]OperatorStackIncident      `json:"incidents"`
}

func (c *Client) OperatorStackRuntime(ctx context.Context, serviceIDs []string) (OperatorStackRuntimeFrame, error) {
	invalid := &OperatorPublicError{Code: "invalid_request"}
	if len(serviceIDs) > 25 {
		return OperatorStackRuntimeFrame{}, invalid
	}
	query := url.Values{}
	requested := make(map[string]bool, len(serviceIDs))
	for _, id := range serviceIDs {
		if !OperatorServiceID(id) || requested[id] {
			return OperatorStackRuntimeFrame{}, invalid
		}
		requested[id] = true
		query.Add("serviceId", id)
	}
	path := "/stack-runtime"
	if len(serviceIDs) != 0 {
		path += "?" + query.Encode()
	}
	return operatorRequest(ctx, c, "GET", path, nil, func(frame OperatorStackRuntimeFrame) bool {
		return frame.valid(requested)
	}, 200)
}

func (frame OperatorStackRuntimeFrame) valid(requested map[string]bool) bool {
	if !OperatorTime(frame.ObservedAt) || frame.Rows == nil || len(frame.Rows) != len(requested) ||
		frame.CoreUptimeMS != nil && *frame.CoreUptimeMS < 0 ||
		frame.ActiveOperations != nil && (frame.ActiveOperations.Scope != "workspace" || frame.ActiveOperations.Count < 0) {
		return false
	}
	seen := make(map[string]bool, len(frame.Rows))
	for _, row := range frame.Rows {
		if !requested[row.ServiceID] || seen[row.ServiceID] ||
			row.UptimeMS != nil && *row.UptimeMS < 0 || row.RestartCount != nil && *row.RestartCount < 0 {
			return false
		}
		seen[row.ServiceID] = true
	}
	if frame.Incidents == nil {
		return true
	}
	if len(*frame.Incidents) > 12 {
		return false
	}
	latest, _ := time.Parse(operatorUTC, frame.ObservedAt)
	for _, incident := range *frame.Incidents {
		if !requested[incident.ServiceID] || !OperatorTime(incident.At) ||
			!operatorEnum(incident.Kind, "http_check_failed", "health_failed", "service_stopped", "operation_failed", "operation_active") {
			return false
		}
		at, _ := time.Parse(operatorUTC, incident.At)
		if at.After(latest) {
			return false
		}
		latest = at
	}
	return true
}
