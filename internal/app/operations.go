package app

import (
	"encoding/json"
	"bytes"
	"io"
    "unicode/utf8"
	"fmt"
	"os"
	"path/filepath"
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

type fileOperationStore struct{ path string }

func defaultOperationStore() operationStore {
	dir, err := os.UserConfigDir()
	if err != nil {
		return nil
	}
	return fileOperationStore{path: filepath.Join(dir, "service-lasso-tui", "operation-reconciliation.json")}
}

func (s fileOperationStore) Load() (*persistedOperation, error) {
	    entry, err := os.Lstat(s.path)
    if os.IsNotExist(err) { return nil, nil }
    if err != nil || !entry.Mode().IsRegular() || entry.Size() <= 0 || entry.Size() > 4096 { return nil, fmt.Errorf("operation reconciliation metadata is invalid") }
    raw, err := os.ReadFile(s.path)
	if os.IsNotExist(err) {
		return nil, nil
	}
	if err != nil {
		return nil, fmt.Errorf("read operation reconciliation metadata: %w", err)
	}
	var value persistedOperation
	if err := decodeOperationMetadata(raw, &value); err != nil || value.Version != 2 || value.OperationID == "" || value.ConnectionName == "" || value.Binding == "" {
		return nil, fmt.Errorf("operation reconciliation metadata is invalid")
	}
	return &value, nil
}

func (s fileOperationStore) Save(value persistedOperation) error {
	if value.Version != 2 || value.OperationID == "" || value.ConnectionName == "" || value.Binding == "" {
		return fmt.Errorf("refuse unsafe operation reconciliation metadata")
	}
	if err := os.MkdirAll(filepath.Dir(s.path), 0700); err != nil {
		return err
	}
	    raw, err := json.Marshal(value)
    if err != nil || len(raw) > 4096 { return fmt.Errorf("operation reconciliation metadata is invalid") }
    // Sync the completed temporary body before the atomic replacement. A failed
    // write leaves the previous retained operation intact.
    temporary, err := os.CreateTemp(filepath.Dir(s.path), ".operation-reconciliation-*")
    if err != nil { return err }
    name := temporary.Name()
    defer os.Remove(name)
    if err = temporary.Chmod(0600); err == nil { _, err = temporary.Write(raw) }
    if err == nil { err = temporary.Sync() }
    closeErr := temporary.Close()
    if err != nil { return err }; if closeErr != nil { return closeErr }
    return os.Rename(name, s.path)
}

func (s fileOperationStore) Clear(expected persistedOperation) error {
	// Only the exact record this submission persisted may be removed. Invalid
	// metadata and another actor/connection's record remain recovery evidence.
	retained, err := s.Load()
	if err != nil {
		return err
	}
	if retained == nil {
		return nil
	}
	if *retained != expected {
		return fmt.Errorf("retained operation changed; reconciliation metadata was not removed")
	}
	if err := os.Remove(s.path); err != nil && !os.IsNotExist(err) {
		return err
	}
	return nil
}

func decodeOperationMetadata(raw []byte, destination *persistedOperation) error {
    if !utf8.Valid(raw) { return fmt.Errorf("invalid reconciliation encoding") }
    decoder := json.NewDecoder(bytes.NewReader(raw))
    token, err := decoder.Token()
    if err != nil || token != json.Delim('{') { return fmt.Errorf("invalid reconciliation object") }
    keys := map[string]bool{}
    for decoder.More() {
        token, err = decoder.Token(); key, ok := token.(string)
        if err != nil || !ok || keys[key] { return fmt.Errorf("duplicate reconciliation field") }
        keys[key] = true
        if key != "version" && key != "operationId" && key != "connectionName" && key != "reconciliationContext" { return fmt.Errorf("unknown reconciliation field") }
        var value json.RawMessage
        if decoder.Decode(&value) != nil { return fmt.Errorf("invalid reconciliation value") }
    }
    token, err = decoder.Token()
    if err != nil || token != json.Delim('}') || len(keys) != 4 { return fmt.Errorf("incomplete reconciliation object") }
    if _, err = decoder.Token(); err != io.EOF { return fmt.Errorf("trailing reconciliation data") }
    return json.Unmarshal(raw,destination)
}
