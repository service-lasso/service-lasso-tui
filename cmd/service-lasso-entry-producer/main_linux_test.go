//go:build linux

package main

import (
	"crypto/sha256"
	"encoding/base64"
	"encoding/hex"
	"encoding/json"
	"os"
	"path/filepath"
	"testing"

	"github.com/service-lasso/service-lasso-tui/internal/coreentry"
)

func writeBundle(t *testing.T, classification string) string {
	t.Helper()
	directory := t.TempDir()
	values := map[string][]byte{
		"profile.bin":   []byte("profile-original"),
		"abi.json":      []byte(`{"goIntBits":64,"cSizeTBits":64}`),
		"geometry.json": []byte(`{"body_bytes":"1"}`),
		"entry.json":    []byte("entry-original"),
		"provider.json": []byte("provider-original"),
		"family.json":   []byte("family-original"),
		"finance.json":  []byte("finance-original"),
		"go":            []byte("go-original"),
		"node":          []byte("node-original"),
		"python":        []byte("python-original"),
	}
	ref := func(name string) reference {
		bytes := values[name]
		sum := sha256.Sum256(bytes)
		if err := os.WriteFile(filepath.Join(directory, name), bytes, 0o600); err != nil {
			t.Fatal(err)
		}
		return reference{Name: name, Sha256: hex.EncodeToString(sum[:]), Size: int64(len(bytes))}
	}
	record := bundleRecord{
		Schema: "service-lasso.tui-native-input-bundle.v1", Classification: classification,
		Platform: "linux",
	}
	record.Host.Platform, record.Host.Arch = "linux", "amd64"
	record.Profile = ref("profile.bin")
	record.ABI = ref("abi.json")
	record.Geometry = ref("geometry.json")
	record.Custody.Entry = ref("entry.json")
	record.Custody.Provider = ref("provider.json")
	record.Custody.Family = ref("family.json")
	record.Custody.Finance = ref("finance.json")
	record.Toolchain.Go = ref("go")
	record.Toolchain.Node = ref("node")
	record.Toolchain.Python = ref("python")
	bytes, err := json.Marshal(record)
	if err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(filepath.Join(directory, "native-inputs.json"), bytes, 0o600); err != nil {
		t.Fatal(err)
	}
	return directory
}

func TestProduceEmitsValidImage(t *testing.T) {
	directory := writeBundle(t, "authorization")
	handoff := []byte(`{"contract":"service-lasso.candidate-entry-handoff.v1","lease":{"leaseId":"lease-a"}}`)
	request := materializeRequest{
		Kind:        "materialize",
		HandoffLine: base64.StdEncoding.EncodeToString(handoff),
		Observation: json.RawMessage(`{"pid":7}`),
	}
	image, imageHash, err := produce(directory, request)
	if err != nil {
		t.Fatal(err)
	}
	decoded, err := coreentry.ValidateImage(image, handoff, imageHash, false)
	if err != nil {
		t.Fatal(err)
	}
	defer decoded.Close()
	if len(decoded.Views) != 9 || decoded.ImageSHA256 != imageHash {
		t.Fatalf("unexpected image: %s %d", decoded.ImageSHA256, len(decoded.Views))
	}
}

func TestProduceRefusesNonAuthorizationBundle(t *testing.T) {
	directory := writeBundle(t, "development-synthetic")
	request := materializeRequest{Kind: "materialize", HandoffLine: base64.StdEncoding.EncodeToString([]byte("{}"))}
	if _, _, err := produce(directory, request); err == nil {
		t.Fatal("development bundle admitted")
	}
}

func TestProduceRefusesMissingOriginal(t *testing.T) {
	directory := writeBundle(t, "authorization")
	if err := os.Remove(filepath.Join(directory, "finance.json")); err != nil {
		t.Fatal(err)
	}
	request := materializeRequest{Kind: "materialize", HandoffLine: base64.StdEncoding.EncodeToString([]byte("{}"))}
	if _, _, err := produce(directory, request); err == nil {
		t.Fatal("missing original admitted")
	}
}
