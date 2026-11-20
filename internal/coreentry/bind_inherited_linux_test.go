//go:build linux

package coreentry

import (
	"bufio"
	"bytes"
	"encoding/json"
	"fmt"
	"os"
	"os/exec"
	"testing"
)

func TestCoreEntryHelperProcess(t *testing.T) {
	if os.Getenv("GO_WANT_CORE_ENTRY_HELPER") != "1" {
		return
	}
	entry, err := BindInherited(true)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(2)
	}
	fmt.Println(entry.LeaseID)
	if err := entry.Release(); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(3)
	}
	if err := entry.Close(); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(4)
	}
	os.Exit(0)
}

func TestBindInheritedConsumesRealDescriptors(t *testing.T) {
	handoffLine := []byte(`{"contract":"service-lasso.candidate-entry-handoff.v1","lease":{"leaseId":"lease-a"}}`)
	imageBytes := testImage(t, handoffLine, requiredSections(), true)
	binding, err := json.Marshal(bindingEnvelope{
		Kind:        "binding",
		LeaseID:     "lease-a",
		ImageBytes:  uint64(len(imageBytes)),
		ImageSHA256: imageHash(imageBytes),
	})
	if err != nil {
		t.Fatal(err)
	}
	bound, err := json.Marshal(boundEnvelope{Kind: "bound", LeaseID: "lease-a"})
	if err != nil {
		t.Fatal(err)
	}

	coreToTUIReader, coreToTUIWriter, err := os.Pipe()
	if err != nil {
		t.Fatal(err)
	}
	defer coreToTUIWriter.Close()
	tuiToCoreReader, tuiToCoreWriter, err := os.Pipe()
	if err != nil {
		t.Fatal(err)
	}
	defer tuiToCoreReader.Close()

	command := exec.Command(os.Args[0], "-test.run=TestCoreEntryHelperProcess")
	command.Env = append(os.Environ(), "GO_WANT_CORE_ENTRY_HELPER=1")
	command.ExtraFiles = []*os.File{coreToTUIReader, tuiToCoreWriter}
	var stdout, stderr bytes.Buffer
	command.Stdout = &stdout
	command.Stderr = &stderr
	if err := command.Start(); err != nil {
		t.Fatal(err)
	}
	_ = coreToTUIReader.Close()
	_ = tuiToCoreWriter.Close()

	if _, err := coreToTUIWriter.Write(append(handoffLine, '\n')); err != nil {
		t.Fatal(err)
	}
	coreLines := bufio.NewReader(tuiToCoreReader)
	if _, err := readControlLine(coreLines); err != nil {
		t.Fatal(err)
	}
	if _, err := coreToTUIWriter.Write(append(binding, '\n')); err != nil {
		t.Fatal(err)
	}
	if _, err := coreToTUIWriter.Write(imageBytes); err != nil {
		t.Fatal(err)
	}
	if _, err := readControlLine(coreLines); err != nil {
		t.Fatal(err)
	}
	if _, err := coreToTUIWriter.Write(append(bound, '\n')); err != nil {
		t.Fatal(err)
	}
	release, err := readControlLine(coreLines)
	if err != nil {
		t.Fatal(err)
	}
	var control map[string]string
	if err := json.Unmarshal(release, &control); err != nil || control["kind"] != "release" || control["leaseId"] != "lease-a" {
		t.Fatalf("unexpected release: %v %#v", err, control)
	}
	if err := command.Wait(); err != nil {
		t.Fatalf("helper failed: %v stderr=%s", err, stderr.String())
	}
	if stdout.String() != "lease-a\n" {
		t.Fatalf("unexpected helper stdout: %q", stdout.String())
	}
}
