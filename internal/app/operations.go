package app

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
)

// persistedOperation is deliberately closed and metadata-only. In particular it
// excludes credentials, confirmation phrases, previews, request bodies, and
// idempotency keys. binding is an opaque local actor/connection binding.
type persistedOperation struct {
	Version        int    `json:"version"`
	OperationID    string `json:"operationId"`
	ConnectionName string `json:"connectionName"`
	Binding        string `json:"binding"`
}

type operationStore interface {
	Load() (*persistedOperation, error)
	Save(persistedOperation) error
	Clear() error
}

type fileOperationStore struct{ path string }

func defaultOperationStore() operationStore {
	dir, err := os.UserConfigDir()
	if err != nil {
		return nil
	}
	return fileOperationStore{path: filepath.Join(dir, "service-lasso-tui", "operation-reconciliation.json")}
}

func (s fileOperationStore) Load() (*persistedOperation, error) {
	bytes, err := os.ReadFile(s.path)
	if os.IsNotExist(err) {
		return nil, nil
	}
	if err != nil {
		return nil, fmt.Errorf("read operation reconciliation metadata: %w", err)
	}
	var value persistedOperation
	if err := json.Unmarshal(bytes, &value); err != nil || value.Version != 1 || value.OperationID == "" || value.ConnectionName == "" || value.Binding == "" {
		return nil, fmt.Errorf("operation reconciliation metadata is invalid")
	}
	return &value, nil
}

func (s fileOperationStore) Save(value persistedOperation) error {
	if value.Version != 1 || value.OperationID == "" || value.ConnectionName == "" || value.Binding == "" {
		return fmt.Errorf("refuse unsafe operation reconciliation metadata")
	}
	if err := os.MkdirAll(filepath.Dir(s.path), 0700); err != nil {
		return err
	}
	bytes, _ := json.Marshal(value)
	return os.WriteFile(s.path, bytes, 0600)
}

func (s fileOperationStore) Clear() error {
	if err := os.Remove(s.path); err != nil && !os.IsNotExist(err) {
		return err
	}
	return nil
}
