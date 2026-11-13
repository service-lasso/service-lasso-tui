package app

import (
	"encoding/json"
		"os"
	"path/filepath"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// persistedOperation is deliberately closed and metadata-only. In particular it
// excludes credentials, confirmation phrases, previews, request bodies, and
// idempotency keys. Binding is supplied by Core as an opaque validated
// actor/client/instance context; it is never derived from a URL or credential.
type persistedOperation struct {
	Version        int    `json:"version"`
	OperationID    string `json:"operationId"`
	ConnectionName string `json:"connectionName"`
	Binding        string `json:"reconciliationContext"`
}

type operationStore interface {
	Load() (*persistedOperation, error)
	Save(persistedOperation) error
	Clear(persistedOperation) error
}

type fileOperationStore struct{ path string; held heldOperationStore }

func defaultOperationStore() operationStore {
	dir, err := os.UserConfigDir()
	if err != nil {
		return nil
	}
	return fileOperationStore{path: filepath.Join(dir, "service-lasso-tui", "operation-reconciliation.json")}
}

func (s fileOperationStore) Load() (*persistedOperation, error) {
	raw,err:=s.readRaw();if err!=nil{return nil,err};if raw==nil{return nil,nil}
	var value persistedOperation
	if err := decodeOperationMetadata(raw, &value); err != nil { return nil,storageErrorCause(err) }
	if value.Version != 2 || value.OperationID == "" || value.ConnectionName == "" || value.Binding == "" {
		return nil, &api.OperatorError{Stage: "storage"}
	}
	return &value, nil
}

func (s fileOperationStore) Save(value persistedOperation) error {
	if value.Version != 2 || value.OperationID == "" || value.ConnectionName == "" || value.Binding == "" {
		return &api.OperatorError{Stage: "storage"}
	}
	raw, err := json.Marshal(value)
	if err != nil { return storageErrorCause(err) }
	if len(raw) > 4096 { return storageError() }
	if _,err:=api.DecodeOperatorSavedV2(raw);err!=nil{return storageErrorCause(err)}
	return s.publication(func(s fileOperationStore)error{
		current,err:=s.readRaw();if err!=nil{return err}
		// Readbacks also call Save. A late A response must not replace B before
		// Clear(A) compares ownership. Only absence can publish a new legacy ID;
		// the identical owner is a no-op that preserves its exact original bytes.
		if current!=nil{
			var retained persistedOperation
			if err:=decodeOperationMetadata(current,&retained);err!=nil{return storageErrorCause(err)}
			if retained!=value{return storageError()}
			return nil
		}
		return s.publishRaw(raw)
	})
}

func (s fileOperationStore) Clear(expected persistedOperation) error {
	// Only the exact record this submission persisted may be removed. Invalid
	// metadata and another actor/connection's record remain recovery evidence.
	return s.publication(func(s fileOperationStore)error{
	retained, err := s.Load()
	if err != nil {
		return err
	}
	if retained == nil {
		return nil
	}
	if *retained != expected {
		return &api.OperatorError{Stage: "storage"}
	}
	if err := s.held.remove(); err != nil {
		return err
	}
	return nil
	})
}

func decodeOperationMetadata(raw []byte, destination *persistedOperation) error {
	value,err:=api.DecodeOperatorSavedV2(raw);if err!=nil{return err}
	*destination=persistedOperation{Version:value.Version,OperationID:value.OperationID,ConnectionName:value.ConnectionName,Binding:value.ReconciliationContext};return nil
}
