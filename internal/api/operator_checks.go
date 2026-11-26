package api

import (
	"context"
	"net/url"
	"time"
)

const OperatorCheckWindow = 15 * time.Minute

type OperatorCheckSample struct {
	At         string `json:"at"`
	Passed     bool   `json:"passed"`
	DurationMS int64  `json:"durationMs"`
}

type OperatorServiceChecks struct {
	ServiceID string                `json:"serviceId"`
	State     string                `json:"state"`
	Samples   []OperatorCheckSample `json:"samples"`
}

type OperatorCheckFrame struct {
	ObservedAt    string                  `json:"observedAt"`
	WindowSeconds int                     `json:"windowSeconds"`
	Rows          []OperatorServiceChecks `json:"rows"`
}

func (c *Client) OperatorHealthChecks(ctx context.Context, serviceIDs []string) (OperatorCheckFrame, error) {
	invalid := &OperatorPublicError{Code: "invalid_request"}
	if len(serviceIDs) == 0 || len(serviceIDs) > 25 {
		return OperatorCheckFrame{}, invalid
	}
	query := url.Values{}
	requested := make(map[string]bool, len(serviceIDs))
	for _, id := range serviceIDs {
		if !OperatorServiceID(id) || requested[id] {
			return OperatorCheckFrame{}, invalid
		}
		requested[id] = true
		query.Add("serviceId", id)
	}
	return operatorRequest(ctx, c, "GET", "/health-check-samples?"+query.Encode(), nil, func(frame OperatorCheckFrame) bool {
		if !OperatorTime(frame.ObservedAt) || frame.WindowSeconds != int(OperatorCheckWindow/time.Second) || len(frame.Rows) != len(requested) {
			return false
		}
		observed, _ := time.Parse(operatorUTC, frame.ObservedAt)
		oldest := observed.Add(-OperatorCheckWindow)
		seen := make(map[string]bool, len(frame.Rows))
		for _, row := range frame.Rows {
			if !requested[row.ServiceID] || seen[row.ServiceID] || !operatorEnum(row.State, "configured", "not_configured") ||
				row.Samples == nil || len(row.Samples) > 60 || row.State == "not_configured" && len(row.Samples) != 0 {
				return false
			}
			seen[row.ServiceID] = true
			before := oldest
			for _, sample := range row.Samples {
				if !OperatorTime(sample.At) || sample.DurationMS < 0 {
					return false
				}
				at, _ := time.Parse(operatorUTC, sample.At)
				if at.Before(before) || at.After(observed) {
					return false
				}
				before = at
			}
		}
		return true
	}, 200)
}
