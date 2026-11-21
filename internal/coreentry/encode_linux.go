//go:build linux

package coreentry

import (
	"encoding/binary"
	"encoding/hex"
	"fmt"
)

// ImageSection is one private producer payload before encoding.
type ImageSection struct {
	Kind      uint32
	Bytes     []byte
	ItemCount uint64
}

// EncodeImage produces a production binding image.
func EncodeImage(handoffLine []byte, sections []ImageSection) ([]byte, string, error) {
	return EncodeImageWithFlags(handoffLine, sections, 0)
}

// EncodeImageWithFlags is the producer side of the same format ValidateImage
// consumes. Non-zero flags are only for explicit development-synthetic tests.
func EncodeImageWithFlags(handoffLine []byte, sections []ImageSection, flags uint32) ([]byte, string, error) {
	if len(sections) == 0 || len(sections) > 11 {
		return nil, "", fmt.Errorf("binder section count invalid")
	}
	seen := map[uint32]bool{}
	tableBytes := len(sections) * binderSectionBytes
	cursor := (binderHeaderBytes + tableBytes + 7) &^ 7
	type placement struct {
		kind      uint32
		bytes     []byte
		itemCount uint64
		offset    int
	}
	placements := make([]placement, 0, len(sections))
	for _, section := range sections {
		if !knownView(section.Kind) {
			return nil, "", fmt.Errorf("binder section kind invalid")
		}
		if seen[section.Kind] {
			return nil, "", fmt.Errorf("binder section duplicate")
		}
		seen[section.Kind] = true
		if len(section.Bytes) == 0 {
			return nil, "", fmt.Errorf("binder section empty")
		}
		if uint64(len(section.Bytes)) > sectionLimit(section.Kind) {
			return nil, "", fmt.Errorf("binder section limit")
		}
		itemCount := section.ItemCount
		if itemCount == 0 {
			itemCount = 1
		}
		if itemCount > uint64(len(section.Bytes)) {
			return nil, "", fmt.Errorf("binder section item count invalid")
		}
		cursor = (cursor + 7) &^ 7
		placements = append(placements, placement{kind: section.Kind, bytes: section.Bytes, itemCount: itemCount, offset: cursor})
		cursor += len(section.Bytes)
		if cursor > binderMaxBytes {
			return nil, "", fmt.Errorf("binder image too large")
		}
	}
	total := (cursor + 7) &^ 7
	if total > binderMaxBytes {
		return nil, "", fmt.Errorf("binder image too large")
	}
	image := make([]byte, total)
	copy(image[:8], []byte(binderMagic))
	binary.LittleEndian.PutUint16(image[8:10], binderVersion)
	binary.LittleEndian.PutUint16(image[10:12], binderHeaderBytes)
	binary.LittleEndian.PutUint32(image[12:16], flags)
	binary.LittleEndian.PutUint64(image[16:24], uint64(total))
	binary.LittleEndian.PutUint64(image[24:32], uint64(len(placements)))
	binary.LittleEndian.PutUint64(image[32:40], binderHeaderBytes)
	binary.LittleEndian.PutUint64(image[40:48], uint64(tableBytes))
	binary.LittleEndian.PutUint64(image[48:56], binderSectionBytes)
	handoff := digest(handoffDomain, handoffLine)
	copy(image[56:88], handoff[:])
	for index, placed := range placements {
		at := binderHeaderBytes + index*binderSectionBytes
		binary.LittleEndian.PutUint32(image[at:at+4], placed.kind)
		binary.LittleEndian.PutUint64(image[at+8:at+16], uint64(placed.offset))
		binary.LittleEndian.PutUint64(image[at+16:at+24], uint64(len(placed.bytes)))
		binary.LittleEndian.PutUint64(image[at+24:at+32], 8)
		binary.LittleEndian.PutUint64(image[at+32:at+40], placed.itemCount)
		copy(image[placed.offset:], placed.bytes)
	}
	imageDigest := digest(imageDomain, image[:binderHeaderBytes], image[binderHeaderBytes:])
	copy(image[88:120], imageDigest[:])
	return image, "sha256:" + hex.EncodeToString(sha256Sum(image)), nil
}
