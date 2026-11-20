//go:build linux

package coreentry

import (
	"bufio"
	"bytes"
	"crypto/sha256"
	"encoding/binary"
	"encoding/hex"
	"encoding/json"
	"sort"
	"testing"
)

func testImage(t *testing.T, handoff []byte, sections map[uint32][]byte, synthetic bool) []byte {
	t.Helper()
	kinds := make([]uint32, 0, len(sections))
	for kind := range sections {
		kinds = append(kinds, kind)
	}
	sort.Slice(kinds, func(i, j int) bool { return kinds[i] < kinds[j] })
	tableBytes := len(kinds) * binderSectionBytes
	cursor := (binderHeaderBytes + tableBytes + 7) &^ 7
	type placement struct {
		kind   uint32
		bytes  []byte
		offset int
	}
	placements := make([]placement, 0, len(kinds))
	for _, kind := range kinds {
		cursor = (cursor + 7) &^ 7
		placements = append(placements, placement{kind: kind, bytes: sections[kind], offset: cursor})
		cursor += len(sections[kind])
	}
	total := (cursor + 7) &^ 7
	image := make([]byte, total)
	copy(image[:8], []byte(binderMagic))
	binary.LittleEndian.PutUint16(image[8:10], binderVersion)
	binary.LittleEndian.PutUint16(image[10:12], binderHeaderBytes)
	if synthetic {
		binary.LittleEndian.PutUint32(image[12:16], binderSyntheticBit)
	}
	binary.LittleEndian.PutUint64(image[16:24], uint64(total))
	binary.LittleEndian.PutUint64(image[24:32], uint64(len(kinds)))
	binary.LittleEndian.PutUint64(image[32:40], binderHeaderBytes)
	binary.LittleEndian.PutUint64(image[40:48], uint64(tableBytes))
	binary.LittleEndian.PutUint64(image[48:56], binderSectionBytes)
	handoffDigest := digest(handoffDomain, handoff)
	copy(image[56:88], handoffDigest[:])
	for index, placed := range placements {
		at := binderHeaderBytes + index*binderSectionBytes
		binary.LittleEndian.PutUint32(image[at:at+4], placed.kind)
		binary.LittleEndian.PutUint64(image[at+8:at+16], uint64(placed.offset))
		binary.LittleEndian.PutUint64(image[at+16:at+24], uint64(len(placed.bytes)))
		binary.LittleEndian.PutUint64(image[at+24:at+32], 8)
		binary.LittleEndian.PutUint64(image[at+32:at+40], 1)
		copy(image[placed.offset:], placed.bytes)
	}
	header := append([]byte(nil), image[:binderHeaderBytes]...)
	imageDigest := digest(imageDomain, header, image[binderHeaderBytes:])
	copy(image[88:120], imageDigest[:])
	return image
}

func requiredSections() map[uint32][]byte {
	return map[uint32][]byte{
		viewCharge:      []byte("charge"),
		viewSource:      []byte("source"),
		viewDomain:      []byte("domain"),
		viewActors:      []byte("actors"),
		viewBases:       []byte("bases"),
		viewPublicInput: []byte("public-input"),
		viewDeadline:    []byte("deadline"),
		viewIssuer:      []byte("issuer"),
		viewCustody:     []byte("custody"),
	}
}

func imageHash(image []byte) string {
	sum := sha256.Sum256(image)
	return "sha256:" + hex.EncodeToString(sum[:])
}

func TestValidateImageSealsAndBuildsViews(t *testing.T) {
	handoff := []byte(`{"contract":"service-lasso.candidate-entry-handoff.v1","lease":{"leaseId":"lease-a"}}`)
	imageBytes := testImage(t, handoff, requiredSections(), true)
	image, err := ValidateImage(imageBytes, handoff, imageHash(imageBytes), true)
	if err != nil {
		t.Fatal(err)
	}
	if image.Flags != binderSyntheticBit || len(image.Views) != len(requiredSections()) {
		t.Fatalf("unexpected image: flags=%d views=%d", image.Flags, len(image.Views))
	}
	if string(image.Views[0].Bytes) != "charge" {
		t.Fatalf("unexpected first view: %q", image.Views[0].Bytes)
	}
	if err := image.Close(); err != nil {
		t.Fatal(err)
	}
	if err := image.Close(); err != nil {
		t.Fatal(err)
	}
}

func TestValidateImageRefusesSyntheticForProduction(t *testing.T) {
	handoff := []byte(`{"contract":"service-lasso.candidate-entry-handoff.v1"}`)
	imageBytes := testImage(t, handoff, requiredSections(), true)
	_, err := ValidateImage(imageBytes, handoff, "", false)
	if err != refusalFlagsUnsupported {
		t.Fatalf("got %v", err)
	}
}

func TestValidateImageRefusesHashAndHandoffMismatch(t *testing.T) {
	handoff := []byte(`{"contract":"service-lasso.candidate-entry-handoff.v1"}`)
	imageBytes := testImage(t, handoff, requiredSections(), false)
	if _, err := ValidateImage(imageBytes, handoff, "sha256:"+string(bytes.Repeat([]byte("0"), 64)), false); err != refusalImageHashMismatch {
		t.Fatalf("got %v", err)
	}
	if _, err := ValidateImage(imageBytes, append(handoff, ' '), "", false); err != refusalHandoffDigest {
		t.Fatalf("got %v", err)
	}
}

func TestValidateImageRefusesMissingRequiredView(t *testing.T) {
	handoff := []byte(`{"contract":"service-lasso.candidate-entry-handoff.v1"}`)
	sections := requiredSections()
	delete(sections, viewCustody)
	imageBytes := testImage(t, handoff, sections, false)
	if _, err := ValidateImage(imageBytes, handoff, "", false); err != refusalSectionMissing {
		t.Fatalf("got %v", err)
	}
}

func TestBindConsumesExactHandoffAndAcknowledges(t *testing.T) {
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
	input := bytes.NewBuffer(nil)
	input.Write(append(handoffLine, '\n'))
	input.Write(append(binding, '\n'))
	input.Write(imageBytes)
	input.Write(append(bound, '\n'))
	output := bytes.NewBuffer(nil)
	entry, err := Bind(bufio.NewReader(input), output, true)
	if err != nil {
		t.Fatal(err)
	}
	if entry.LeaseID != "lease-a" || entry.Image == nil {
		t.Fatalf("unexpected entry: %#v", entry)
	}
	_ = entry.Close()

	lines := bytes.Split(bytes.TrimSuffix(output.Bytes(), []byte{'\n'}), []byte{'\n'})
	if len(lines) != 2 {
		t.Fatalf("unexpected output lines: %d", len(lines))
	}
	var observation observationEnvelope
	if err := json.Unmarshal(lines[0], &observation); err != nil {
		t.Fatal(err)
	}
	if observation.Kind != "observation" || observation.Observation.PID <= 0 ||
		observation.Observation.ExecutableIdentity == "" || len(observation.Observation.Argv) == 0 {
		t.Fatalf("unexpected observation: %#v", observation)
	}
	var ack ackEnvelope
	if err := json.Unmarshal(lines[1], &ack); err != nil {
		t.Fatal(err)
	}
	if ack.Kind != "ack" || ack.LeaseID != "lease-a" || ack.ImageSHA256 != imageHash(imageBytes) {
		t.Fatalf("unexpected ack: %#v", ack)
	}
}

func TestBindRefusesAbsentChannel(t *testing.T) {
	if _, err := Bind(nil, bytes.NewBuffer(nil), true); err != refusalChannelAbsent {
		t.Fatalf("got %v", err)
	}
}

func TestDecodeClosedRejectsDuplicateKeysAndTrailingData(t *testing.T) {
	var target map[string]any
	if err := decodeClosed([]byte(`{"a":1,"a":2}`), &target); err == nil {
		t.Fatal("duplicate key admitted")
	}
	if err := decodeClosed([]byte(`{"a":1} {}`), &target); err == nil {
		t.Fatal("trailing data admitted")
	}
	if err := decodeClosed([]byte(`{"a":1}`), &target); err != nil {
		t.Fatal(err)
	}
}
