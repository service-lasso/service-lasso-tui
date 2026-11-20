//go:build linux

// Package coreentry consumes the Core-owned candidate entry handoff. It does
// not acquire a charge, source, domain, issuer, deadline, or custody object.
package coreentry

import (
	"crypto/sha256"
	"encoding/binary"
	"encoding/hex"
	"fmt"

	"golang.org/x/sys/unix"
)

const (
	binderMagic        = "SLBIND1\x00"
	binderVersion      = 1
	binderHeaderBytes  = 128
	binderSectionBytes = 40
	binderMaxBytes     = 4 * 1024 * 1024
	binderSyntheticBit = 1
)

const (
	viewCharge = iota + 1
	viewSource
	viewDomain
	viewActors
	viewBases
	viewPublicInput
	viewDeadline
	viewIssuer
	viewCustody
	viewCapture
	viewObservation
)

var requiredViews = [...]uint32{
	viewCharge,
	viewSource,
	viewDomain,
	viewActors,
	viewBases,
	viewPublicInput,
	viewDeadline,
	viewIssuer,
	viewCustody,
}

var handoffDomain = []byte("service-lasso.candidate-entry-handoff.v1\x00")
var imageDomain = []byte("service-lasso.entry-binding.v1\x00")

// Refusal is a closed binder refusal code. It is not an authorization result.
type Refusal string

func (r Refusal) Error() string { return string(r) }

const (
	refusalChannelAbsent       Refusal = "binder_channel_absent"
	refusalImageMissing        Refusal = "binder_image_missing"
	refusalImageHashMismatch   Refusal = "binder_image_hash_mismatch"
	refusalImageMagicInvalid   Refusal = "binder_image_magic_invalid"
	refusalVersionUnsupported  Refusal = "binder_image_version_unsupported"
	refusalFlagsUnsupported    Refusal = "binder_image_flags_unsupported"
	refusalImageSizeInvalid    Refusal = "binder_image_size_invalid"
	refusalImageDigestMismatch Refusal = "binder_image_digest_mismatch"
	refusalHandoffDigest       Refusal = "binder_handoff_digest_mismatch"
	refusalTableInvalid        Refusal = "binder_section_table_invalid"
	refusalSectionUnknown      Refusal = "binder_section_unknown"
	refusalSectionDuplicate    Refusal = "binder_section_duplicate"
	refusalSectionMissing      Refusal = "binder_section_missing"
	refusalSectionBounds       Refusal = "binder_section_bounds"
	refusalSectionOverlap      Refusal = "binder_section_overlap"
	refusalSectionAlignment    Refusal = "binder_section_alignment"
	refusalSectionLimit        Refusal = "binder_section_limit"
	refusalSealFailed          Refusal = "binder_seal_failed"
	refusalViewInvalid         Refusal = "binder_view_invalid"
	refusalNativeABI           Refusal = "binder_native_abi_mismatch"
)

// View is an immutable read-only window into the sealed binding image.
type View struct {
	Kind      uint32
	Flags     uint32
	Offset    uint64
	Length    uint64
	Alignment uint64
	ItemCount uint64
	Bytes     []byte
}

// Image owns the sealed mapping and every view into it. Close is required
// after the last borrower.
type Image struct {
	Base          []byte
	Views         []View
	Flags         uint32
	ImageSHA256   string
	HandoffSHA256 string

	fd     int
	closed bool
}

// Close unmaps the image and closes the sealed memfd exactly once.
func (i *Image) Close() error {
	if i == nil || i.closed {
		return nil
	}
	i.closed = true
	var first error
	if i.Base != nil {
		if err := unix.Munmap(i.Base); err != nil {
			first = err
		}
		i.Base = nil
	}
	if i.fd >= 0 {
		if err := unix.Close(i.fd); err != nil && first == nil {
			first = err
		}
		i.fd = -1
	}
	return first
}

func sectionLimit(kind uint32) uint64 {
	if kind == viewSource || kind == viewPublicInput {
		return 1024 * 1024
	}
	return 256 * 1024
}

func knownView(kind uint32) bool {
	return kind >= viewCharge && kind <= viewObservation
}

func digest(domain []byte, parts ...[]byte) [32]byte {
	hash := sha256.New()
	hash.Write(domain)
	for _, part := range parts {
		hash.Write(part)
	}
	var value [32]byte
	copy(value[:], hash.Sum(nil))
	return value
}

func digestString(value [32]byte) string {
	return "sha256:" + hex.EncodeToString(value[:])
}

// ValidateImage verifies the exact Core-authored image and materializes it into
// a sealed read-only memfd. It does not call the native owner entry.
func ValidateImage(image []byte, handoffLine []byte, expectedSHA256 string, allowSynthetic bool) (*Image, error) {
	if len(image) < binderHeaderBytes {
		return nil, refusalImageSizeInvalid
	}
	if len(image) > binderMaxBytes {
		return nil, refusalImageSizeInvalid
	}
	if expectedSHA256 != "" {
		actual := "sha256:" + hex.EncodeToString(sha256Sum(image))
		if actual != expectedSHA256 {
			return nil, refusalImageHashMismatch
		}
	}
	if string(image[:8]) != binderMagic {
		return nil, refusalImageMagicInvalid
	}
	if binary.LittleEndian.Uint16(image[8:10]) != binderVersion {
		return nil, refusalVersionUnsupported
	}
	if binary.LittleEndian.Uint16(image[10:12]) != binderHeaderBytes {
		return nil, refusalImageSizeInvalid
	}
	flags := binary.LittleEndian.Uint32(image[12:16])
	if flags != 0 && !(allowSynthetic && flags == binderSyntheticBit) {
		return nil, refusalFlagsUnsupported
	}
	imageBytes := binary.LittleEndian.Uint64(image[16:24])
	if imageBytes != uint64(len(image)) || imageBytes > binderMaxBytes {
		return nil, refusalImageSizeInvalid
	}
	for index := 120; index < binderHeaderBytes; index++ {
		if image[index] != 0 {
			return nil, refusalImageSizeInvalid
		}
	}
	sectionCount := binary.LittleEndian.Uint64(image[24:32])
	if sectionCount == 0 || sectionCount > 11 {
		return nil, refusalTableInvalid
	}
	tableOffset := binary.LittleEndian.Uint64(image[32:40])
	tableBytes := binary.LittleEndian.Uint64(image[40:48])
	entryBytes := binary.LittleEndian.Uint64(image[48:56])
	if tableOffset < binderHeaderBytes || tableOffset%8 != 0 ||
		entryBytes != binderSectionBytes ||
		tableBytes != sectionCount*binderSectionBytes ||
		tableOffset+tableBytes > imageBytes {
		return nil, refusalTableInvalid
	}
	handoff := digest(handoffDomain, handoffLine)
	if string(image[56:88]) != string(handoff[:]) {
		return nil, refusalHandoffDigest
	}
	header := append([]byte(nil), image[:binderHeaderBytes]...)
	for index := 88; index < 120; index++ {
		header[index] = 0
	}
	expectedImageDigest := digest(imageDomain, header, image[binderHeaderBytes:])
	if string(image[88:120]) != string(expectedImageDigest[:]) {
		return nil, refusalImageDigestMismatch
	}

	tableEnd := tableOffset + tableBytes
	seen := make(map[uint32]bool, int(sectionCount))
	views := make([]View, 0, int(sectionCount))
	for index := uint64(0); index < sectionCount; index++ {
		at := tableOffset + index*entryBytes
		kind := binary.LittleEndian.Uint32(image[at : at+4])
		if !knownView(kind) {
			return nil, refusalSectionUnknown
		}
		if seen[kind] {
			return nil, refusalSectionDuplicate
		}
		seen[kind] = true
		sectionFlags := binary.LittleEndian.Uint32(image[at+4 : at+8])
		if sectionFlags != 0 {
			return nil, refusalTableInvalid
		}
		offset := binary.LittleEndian.Uint64(image[at+8 : at+16])
		length := binary.LittleEndian.Uint64(image[at+16 : at+24])
		alignment := binary.LittleEndian.Uint64(image[at+24 : at+32])
		itemCount := binary.LittleEndian.Uint64(image[at+32 : at+40])
		if length == 0 || itemCount == 0 {
			return nil, refusalSectionBounds
		}
		if alignment != 8 || offset%8 != 0 {
			return nil, refusalSectionAlignment
		}
		if offset < tableEnd || offset+length < offset || offset+length > imageBytes {
			return nil, refusalSectionBounds
		}
		if length > sectionLimit(kind) {
			return nil, refusalSectionLimit
		}
		if itemCount > length {
			return nil, refusalSectionBounds
		}
		views = append(views, View{
			Kind:      kind,
			Flags:     sectionFlags,
			Offset:    offset,
			Length:    length,
			Alignment: alignment,
			ItemCount: itemCount,
		})
	}
	for _, required := range requiredViews {
		if !seen[required] {
			return nil, refusalSectionMissing
		}
	}
	for left := 1; left < len(views); left++ {
		for right := 0; right < left; right++ {
			a, b := views[right], views[left]
			if a.Offset < b.Offset+b.Length && b.Offset < a.Offset+a.Length {
				return nil, refusalSectionOverlap
			}
		}
	}

	mapping, fd, err := sealImage(image)
	if err != nil {
		return nil, refusalSealFailed
	}
	for index := range views {
		views[index].Bytes = mapping[views[index].Offset : views[index].Offset+views[index].Length]
	}
	return &Image{
		Base:          mapping,
		Views:         views,
		Flags:         flags,
		ImageSHA256:   "sha256:" + hex.EncodeToString(sha256Sum(image)),
		HandoffSHA256: digestString(handoff),
		fd:            fd,
	}, nil
}

func sha256Sum(value []byte) []byte {
	sum := sha256.Sum256(value)
	return sum[:]
}

func sealImage(image []byte) ([]byte, int, error) {
	fd, err := unix.MemfdCreate("service-lasso-tui-core-entry", unix.MFD_CLOEXEC|unix.MFD_ALLOW_SEALING)
	if err != nil {
		return nil, -1, err
	}
	fail := func(err error) ([]byte, int, error) {
		unix.Close(fd)
		return nil, -1, err
	}
	if err := unix.Ftruncate(fd, int64(len(image))); err != nil {
		return fail(err)
	}
	written, err := unix.Pwrite(fd, image, 0)
	if err != nil || written != len(image) {
		return fail(fmt.Errorf("write sealed image: %w", err))
	}
	seals := unix.F_SEAL_WRITE | unix.F_SEAL_GROW | unix.F_SEAL_SHRINK | unix.F_SEAL_SEAL
	if _, err := unix.FcntlInt(uintptr(fd), unix.F_ADD_SEALS, seals); err != nil {
		return fail(err)
	}
	actual, err := unix.FcntlInt(uintptr(fd), unix.F_GET_SEALS, 0)
	if err != nil || actual != seals {
		return fail(fmt.Errorf("seal verification: %w", err))
	}
	mapping, err := unix.Mmap(fd, 0, len(image), unix.PROT_READ, unix.MAP_PRIVATE)
	if err != nil {
		return fail(err)
	}
	return mapping, fd, nil
}
