//go:build linux

package coreentry

import (
	"bufio"
	"bytes"
	"crypto/sha256"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"io"
	"os"
	"strconv"
	"strings"
	"sync"

	"golang.org/x/sys/unix"
)

const maxControlLine = 64 * 1024

type leaseIdentity struct {
	LeaseID       string `json:"leaseId"`
	AttemptID     string `json:"attemptId"`
	IssuerID      string `json:"issuerId"`
	ReservationID string `json:"reservationId"`
	OperationID   string `json:"operationId"`
}

type handoffEnvelope struct {
	Contract string        `json:"contract"`
	Lease    leaseIdentity `json:"lease"`
}

type consumerObservation struct {
	PID                int      `json:"pid"`
	StartIdentity      string   `json:"startIdentity"`
	ExecutableIdentity string   `json:"executableIdentity"`
	DomainIdentity     string   `json:"domainIdentity"`
	Argv               []string `json:"argv"`
	Environment        []string `json:"environment"`
}

type observationEnvelope struct {
	Kind        string              `json:"kind"`
	Observation consumerObservation `json:"observation"`
}

type bindingEnvelope struct {
	Kind        string `json:"kind"`
	LeaseID     string `json:"leaseId"`
	ImageBytes  uint64 `json:"imageBytes"`
	ImageSHA256 string `json:"imageSha256"`
}

type ackEnvelope struct {
	Kind        string `json:"kind"`
	LeaseID     string `json:"leaseId"`
	ImageSHA256 string `json:"imageSha256"`
}

type boundEnvelope struct {
	Kind    string `json:"kind"`
	LeaseID string `json:"leaseId"`
}

// Entry is the Core-bound consumer record. It is not a charge, source, domain,
// issuer, deadline, or custody object.
type Entry struct {
	LeaseID string
	Image   *Image

	writer      io.Writer
	releaseOnce sync.Once
	releaseErr  error
}

func (e *Entry) Close() error {
	if e == nil {
		return nil
	}
	return e.Image.Close()
}

// Release tells Core the last borrower is finished. It is once-only and must
// be called before Close.
func (e *Entry) Release() error {
	if e == nil {
		return nil
	}
	e.releaseOnce.Do(func() {
		if e.writer == nil || e.LeaseID == "" {
			e.releaseErr = refusalChannelAbsent
			return
		}
		e.releaseErr = writeControlLine(e.writer, map[string]string{
			"kind":    "release",
			"leaseId": e.LeaseID,
		})
	})
	return e.releaseErr
}

func releaseControl(writer io.Writer, leaseID string) {
	if writer == nil || leaseID == "" {
		return
	}
	_ = writeControlLine(writer, map[string]string{"kind": "release", "leaseId": leaseID})
}

// Bind consumes the Core handoff, sends the consumer observation, validates the
// binding image, acknowledges it, and waits for Core's bound message.
func Bind(reader *bufio.Reader, writer io.Writer, allowSynthetic bool) (*Entry, error) {
	if reader == nil || writer == nil {
		return nil, refusalChannelAbsent
	}
	handoffLine, err := readControlLine(reader)
	if err != nil {
		return nil, err
	}
	var handoff handoffEnvelope
	if err := decodeClosed(handoffLine, &handoff); err != nil {
		return nil, fmt.Errorf("core entry handoff: %w", err)
	}
	if handoff.Contract != "service-lasso.candidate-entry-handoff.v1" || handoff.Lease.LeaseID == "" {
		return nil, fmt.Errorf("core entry handoff: invalid contract or lease")
	}

	observation, err := observeSelf()
	if err != nil {
		return nil, err
	}
	if err := writeControlLine(writer, observationEnvelope{Kind: "observation", Observation: observation}); err != nil {
		return nil, err
	}

	bindingLine, err := readControlLine(reader)
	if err != nil {
		return nil, err
	}
	var binding bindingEnvelope
	if err := decodeClosed(bindingLine, &binding); err != nil {
		return nil, fmt.Errorf("core entry binding: %w", err)
	}
	if binding.Kind != "binding" ||
		binding.LeaseID != handoff.Lease.LeaseID ||
		binding.ImageBytes == 0 ||
		binding.ImageBytes > binderMaxBytes ||
		!validSHA256(binding.ImageSHA256) {
		return nil, fmt.Errorf("core entry binding: invalid message")
	}

	image := make([]byte, int(binding.ImageBytes))
	if _, err := io.ReadFull(reader, image); err != nil {
		return nil, fmt.Errorf("core entry image: %w", err)
	}
	sealed, err := ValidateImage(image, handoffLine, binding.ImageSHA256, allowSynthetic)
	if err != nil {
		return nil, err
	}
	if err := writeControlLine(writer, ackEnvelope{
		Kind:        "ack",
		LeaseID:     handoff.Lease.LeaseID,
		ImageSHA256: sealed.ImageSHA256,
	}); err != nil {
		releaseControl(writer, handoff.Lease.LeaseID)
		_ = sealed.Close()
		return nil, err
	}

	boundLine, err := readControlLine(reader)
	if err != nil {
		releaseControl(writer, handoff.Lease.LeaseID)
		_ = sealed.Close()
		return nil, err
	}
	var bound boundEnvelope
	if err := decodeClosed(boundLine, &bound); err != nil {
		releaseControl(writer, handoff.Lease.LeaseID)
		_ = sealed.Close()
		return nil, fmt.Errorf("core entry bound: %w", err)
	}
	if bound.Kind != "bound" || bound.LeaseID != handoff.Lease.LeaseID {
		releaseControl(writer, handoff.Lease.LeaseID)
		_ = sealed.Close()
		return nil, fmt.Errorf("core entry bound: invalid message")
	}
	return &Entry{LeaseID: handoff.Lease.LeaseID, Image: sealed, writer: writer}, nil
}

// BindInherited consumes the descriptors Core reserved for the candidate entry.
func BindInherited(allowSynthetic bool) (*Entry, error) {
	for _, fd := range []int{3, 4} {
		if _, err := unix.FcntlInt(uintptr(fd), unix.F_GETFD, 0); err != nil {
			return nil, refusalChannelAbsent
		}
	}
	reader := bufio.NewReaderSize(os.NewFile(3, "core-entry-handoff"), maxControlLine)
	writer := os.NewFile(4, "core-entry-ack")
	return Bind(reader, writer, allowSynthetic)
}

func readControlLine(reader *bufio.Reader) ([]byte, error) {
	line, err := reader.ReadBytes('\n')
	if err != nil {
		if err == io.EOF {
			return nil, refusalChannelAbsent
		}
		return nil, err
	}
	if len(line) == 0 || len(line) > maxControlLine || line[len(line)-1] != '\n' {
		return nil, fmt.Errorf("core entry control line invalid")
	}
	line = line[:len(line)-1]
	if bytes.IndexByte(line, 0) >= 0 || bytes.IndexByte(line, '\r') >= 0 {
		return nil, fmt.Errorf("core entry control line invalid")
	}
	return line, nil
}

func writeControlLine(writer io.Writer, value any) error {
	encoded, err := json.Marshal(value)
	if err != nil {
		return err
	}
	if len(encoded) > maxControlLine {
		return fmt.Errorf("core entry control line too large")
	}
	if _, err := writer.Write(append(encoded, '\n')); err != nil {
		return err
	}
	return nil
}

func decodeClosed(value []byte, target any) error {
	if err := rejectDuplicateKeys(value); err != nil {
		return err
	}
	decoder := json.NewDecoder(bytes.NewReader(value))
	decoder.DisallowUnknownFields()
	if err := decoder.Decode(target); err != nil {
		return err
	}
	var extra any
	if err := decoder.Decode(&extra); err != io.EOF {
		return fmt.Errorf("trailing data")
	}
	return nil
}

func rejectDuplicateKeys(value []byte) error {
	decoder := json.NewDecoder(bytes.NewReader(value))
	if err := walkJSON(decoder); err != nil {
		return err
	}
	if _, err := decoder.Token(); err != io.EOF {
		return fmt.Errorf("trailing data")
	}
	return nil
}

func walkJSON(decoder *json.Decoder) error {
	token, err := decoder.Token()
	if err != nil {
		return err
	}
	delimiter, ok := token.(json.Delim)
	if !ok {
		return nil
	}
	switch delimiter {
	case '{':
		seen := map[string]bool{}
		for decoder.More() {
			keyToken, err := decoder.Token()
			if err != nil {
				return err
			}
			key, ok := keyToken.(string)
			if !ok {
				return fmt.Errorf("object key invalid")
			}
			if seen[key] {
				return fmt.Errorf("duplicate object key")
			}
			seen[key] = true
			if err := walkJSON(decoder); err != nil {
				return err
			}
		}
		_, err = decoder.Token()
		return err
	case '[':
		for decoder.More() {
			if err := walkJSON(decoder); err != nil {
				return err
			}
		}
		_, err = decoder.Token()
		return err
	default:
		return fmt.Errorf("unexpected delimiter")
	}
}

func validSHA256(value string) bool {
	if len(value) != 71 || !strings.HasPrefix(value, "sha256:") {
		return false
	}
	for _, character := range value[7:] {
		if (character < '0' || character > '9') && (character < 'a' || character > 'f') {
			return false
		}
	}
	return true
}

func observeSelf() (consumerObservation, error) {
	pid := os.Getpid()
	stat, err := os.ReadFile("/proc/self/stat")
	if err != nil {
		return consumerObservation{}, err
	}
	close := bytes.LastIndexByte(stat, ')')
	if close < 0 {
		return consumerObservation{}, fmt.Errorf("core entry self observation: stat invalid")
	}
	fields := strings.Fields(string(stat[close+2:]))
	if len(fields) < 20 {
		return consumerObservation{}, fmt.Errorf("core entry self observation: start unavailable")
	}
	start, err := strconv.ParseUint(fields[19], 10, 64)
	if err != nil {
		return consumerObservation{}, fmt.Errorf("core entry self observation: start invalid")
	}
	executable, err := os.ReadFile("/proc/self/exe")
	if err != nil {
		return consumerObservation{}, err
	}
	executableHash := sha256.Sum256(executable)
	pidNamespace, err := os.Readlink("/proc/self/ns/pid")
	if err != nil {
		return consumerObservation{}, err
	}
	mountNamespace, err := os.Readlink("/proc/self/ns/mnt")
	if err != nil {
		return consumerObservation{}, err
	}
	argv, err := readNulArguments("/proc/self/cmdline")
	if err != nil {
		return consumerObservation{}, err
	}
	environment, err := readNulArguments("/proc/self/environ")
	if err != nil {
		return consumerObservation{}, err
	}
	if len(argv) == 0 {
		return consumerObservation{}, fmt.Errorf("core entry self observation: argv unavailable")
	}
	return consumerObservation{
		PID:                pid,
		StartIdentity:      "linux-start-v1:" + strconv.FormatUint(start, 10),
		ExecutableIdentity: "sha256:" + hex.EncodeToString(executableHash[:]),
		DomainIdentity:     "linux-ns-v1:" + pidNamespace + ":" + mountNamespace,
		Argv:               argv,
		Environment:        environment,
	}, nil
}

func readNulArguments(path string) ([]string, error) {
	value, err := os.ReadFile(path)
	if err != nil {
		return nil, err
	}
	parts := bytes.Split(value, []byte{0})
	if len(parts) > 0 && len(parts[len(parts)-1]) == 0 {
		parts = parts[:len(parts)-1]
	}
	result := make([]string, len(parts))
	for index, part := range parts {
		result[index] = string(part)
	}
	return result, nil
}
