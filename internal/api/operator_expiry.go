package api

import "time"

// This local stale-data guard never establishes source or mutation authority.
// The genuine server independently enforces its retained snapshot lifetime.
func OperatorSnapshotExpiredAt(expiry string,now time.Time)bool{
 if !OperatorTime(expiry){return true};deadline,err:=time.Parse("2006-01-02T15:04:05.000Z",expiry);return err!=nil||!deadline.After(now)
}

// A local deadline denial preserves the original preview. Only a deliberate
// new preview can supply another server binding; this check issues no grant.
func operatorPreviewExpiryAt(preview OperatorPreview,now time.Time)error{
 if preview.Confirmation.Required&&preview.Confirmation.ExpiresAt!=nil&&OperatorSnapshotExpiredAt(*preview.Confirmation.ExpiresAt,now){return &OperatorPublicError{Code:"confirmation_expired"}}
 if OperatorSnapshotExpiredAt(preview.ExpiresAt,now){return &OperatorPublicError{Code:"binding_stale"}}
 return nil
}
