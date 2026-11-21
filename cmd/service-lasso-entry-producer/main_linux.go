//go:build linux

package main

import (
	"crypto/sha256"
	"encoding/base64"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"os"
	"path"
	"path/filepath"

	"github.com/service-lasso/service-lasso-tui/internal/coreentry"
)

type reference struct {
	Name   string `json:"name"`
	Sha256 string `json:"sha256"`
	Size   int64  `json:"size"`
}

type bundleRecord struct {
	Schema         string `json:"schema"`
	Classification string `json:"classification"`
	Platform       string `json:"platform"`
	Host           struct {
		Platform string `json:"platform"`
		Arch     string `json:"arch"`
	} `json:"host"`
	Profile  reference `json:"profile"`
	ABI      reference `json:"abi"`
	Geometry reference `json:"geometry"`
	Custody  struct {
		Entry    reference `json:"entry"`
		Provider reference `json:"provider"`
		Family   reference `json:"family"`
		Finance  reference `json:"finance"`
	} `json:"custody"`
	Toolchain struct {
		Go     reference `json:"go"`
		Node   reference `json:"node"`
		Python reference `json:"python"`
	} `json:"toolchain"`
	WindowsQueryProfile *reference `json:"windowsQueryProfile"`
}

type materializeRequest struct {
	Kind        string          `json:"kind"`
	HandoffLine string          `json:"handoffLine"`
	Observation json.RawMessage `json:"observation"`
}

func readOriginal(directory string, ref reference) ([]byte, error) {
	if path.Base(ref.Name) != ref.Name || ref.Name == "." || ref.Name == ".." {
		return nil, fmt.Errorf("bundle original name invalid")
	}
	file := filepath.Join(directory, ref.Name)
	bytes, err := os.ReadFile(file)
	if err != nil {
		return nil, err
	}
	if int64(len(bytes)) != ref.Size {
		return nil, fmt.Errorf("bundle original size mismatch")
	}
	sum := sha256.Sum256(bytes)
	if "sha256:"+hex.EncodeToString(sum[:]) != ref.Sha256 && hex.EncodeToString(sum[:]) != ref.Sha256 {
		return nil, fmt.Errorf("bundle original hash mismatch")
	}
	return bytes, nil
}

func readRecord(directory string) (bundleRecord, error) {
	var record bundleRecord
	bytes, err := os.ReadFile(filepath.Join(directory, "native-inputs.json"))
	if err != nil {
		return record, err
	}
	if err := json.Unmarshal(bytes, &record); err != nil {
		return record, err
	}
	if record.Schema != "service-lasso.tui-native-input-bundle.v1" ||
		record.Classification != "authorization" ||
		record.Platform != "linux" ||
		record.Host.Platform != "linux" ||
		record.Host.Arch != "amd64" {
		return record, fmt.Errorf("authorization bundle required")
	}
	return record, nil
}

func produce(bundleDirectory string, request materializeRequest) (image []byte, imageSha256 string, failure error) {
	if request.Kind != "materialize" || request.HandoffLine == "" {
		return nil, "", fmt.Errorf("materialize request invalid")
	}
	handoff, err := base64.StdEncoding.DecodeString(request.HandoffLine)
	if err != nil {
		return nil, "", fmt.Errorf("handoff line invalid")
	}
	record, err := readRecord(bundleDirectory)
	if err != nil {
		return nil, "", err
	}
	files := map[string][]byte{}
	for name, ref := range map[string]reference{
		"profile":          record.Profile,
		"abi":              record.ABI,
		"geometry":         record.Geometry,
		"custody.entry":    record.Custody.Entry,
		"custody.provider": record.Custody.Provider,
		"custody.family":   record.Custody.Family,
		"custody.finance":  record.Custody.Finance,
		"toolchain.go":     record.Toolchain.Go,
		"toolchain.node":   record.Toolchain.Node,
		"toolchain.python": record.Toolchain.Python,
	} {
		files[name], err = readOriginal(bundleDirectory, ref)
		if err != nil {
			return nil, "", err
		}
	}
	observation := request.Observation
	if len(observation) == 0 {
		observation = []byte("{}")
	}
	custody, err := json.Marshal(map[string]any{
		"toolchain": map[string]reference{
			"go": record.Toolchain.Go, "node": record.Toolchain.Node, "python": record.Toolchain.Python,
		},
		"windowsQueryProfile": record.WindowsQueryProfile,
	})
	if err != nil {
		return nil, "", err
	}
	sections := []coreentry.ImageSection{
		{Kind: coreentry.ViewCharge, Bytes: files["profile"]},
		{Kind: coreentry.ViewSource, Bytes: files["geometry"]},
		{Kind: coreentry.ViewDomain, Bytes: files["abi"]},
		{Kind: coreentry.ViewActors, Bytes: files["custody.entry"]},
		{Kind: coreentry.ViewBases, Bytes: files["custody.provider"]},
		{Kind: coreentry.ViewPublicInput, Bytes: observation},
		{Kind: coreentry.ViewDeadline, Bytes: files["custody.family"]},
		{Kind: coreentry.ViewIssuer, Bytes: files["custody.finance"]},
		{Kind: coreentry.ViewCustody, Bytes: custody},
	}
	image, imageSha256, err = coreentry.EncodeImage(handoff, sections)
	return image, imageSha256, err
}

func refuse(code string) {
	_ = json.NewEncoder(os.Stdout).Encode(map[string]any{"ok": false, "code": code})
	os.Exit(0)
}

func main() {
	if len(os.Args) != 2 {
		os.Exit(2)
	}
	var request materializeRequest
	if err := json.NewDecoder(os.Stdin).Decode(&request); err != nil {
		refuse("binder_message_invalid")
	}
	image, imageSha256, err := produce(os.Args[1], request)
	if err != nil {
		refuse("binder_originals_unavailable")
	}
	_ = json.NewEncoder(os.Stdout).Encode(map[string]any{
		"ok": true, "imageBytes": len(image), "imageSha256": imageSha256,
	})
	if _, err := os.Stdout.Write(image); err != nil {
		os.Exit(1)
	}
}
