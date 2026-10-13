package app

import (
	"bytes"
	"encoding/json"
	"errors"
	"testing"
)

// These source-authored serialization and identity cases remain UNRUN.
func TestStorageOutcomePublicJSONRetainsPrivateOriginalsInternally(t *testing.T) {
	primary := errors.New("private-primary-original")
	cleanup := errors.New("private-cleanup-original")
	native := errors.New("private-native-original")
	retainedCause := errors.New("private-retained-owner-original")
	owner := recordNativeIdentity{Platform:"private-native-platform",Kind:"private-native-kind",Handle:987654321,IdentityKnown:true}
	retainedObject := &struct{Secret string}{Secret:"private-owner-object"}
	original := []byte("private-original-record")
	replacement := []byte("private-replacement-record")
	failure := &recordStorageOutcomeError{
		Primary:primary,Cleanup:cleanup,
		Outcome:recordStorageOutcome{
			PublicationEffect:recordEffectUnresolved,CleanupRelease:recordReleaseUnresolved,
			OriginalKnown:true,Original:original,Replacement:replacement,
			Releases:[]recordNativeRelease{{Owner:owner,Cause:native}},
			BodyMismatches:[]recordNativeBodyMismatch{{Owner:owner,Expected:original,Observed:replacement}},
			RetainedOwners:[]recordNativeLifetime{{Owner:owner,Object:retainedObject,CloseAttempted:true,Cause:retainedCause}},
		},
	}
	encoded, err := json.Marshal(struct{Failure error `json:"failure"`}{Failure:failure})
	if err != nil { t.Fatal(err) }
	expected, err := json.Marshal(struct{Failure struct{Message string `json:"message"`} `json:"failure"`}{Failure:struct{Message string `json:"message"`}{Message:failure.Error()}})
	if err != nil || !bytes.Equal(encoded,expected) { t.Fatal("public projection contains more than the fixed safe message") }
	for _, cause := range []error{primary,cleanup,native,retainedCause} {
		if !errors.Is(failure,cause) { t.Fatal("original cause identity was lost") }
	}
	observed, ok := recordOutcome(failure)
	if !ok || !bytes.Equal(observed.Original,original) || !bytes.Equal(observed.Replacement,replacement) || observed.RetainedOwners[0].Object != retainedObject || observed.RetainedOwners[0].Cause != retainedCause || observed.Releases[0].Cause != native {
		t.Fatal("private outcome inspection lost original custody")
	}
}
