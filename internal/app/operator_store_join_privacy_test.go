package app

import("encoding/json";"errors";"strings";"testing")

type privateNativeJoinCause struct{Owner string;Raw []byte}
func(e *privateNativeJoinCause)Error()string{return e.Owner+string(e.Raw)}
type privateJoinHeld struct{privateSavedCauseHeld;cause error}
func(h *privateJoinHeld)read()([]byte,error){h.reads++;return append([]byte(nil),h.raw...),h.cause}
func(h *privateJoinHeld)publish(raw []byte)error{h.publishes++;return h.cause}
func TestHeldStorageBoundaryRetainsPrivateOriginalWithoutPublishing(t *testing.T){
 original:=&privateNativeJoinCause{Owner:"private-held-owner",Raw:[]byte("private-held-original")}
 held:=&privateJoinHeld{privateSavedCauseHeld:privateSavedCauseHeld{raw:[]byte("retained-record")},cause:original};store:=fileOperationStore{path:"source-only-held",held:held}
 raw,readErr:=store.readRaw();publishErr:=store.publishRaw([]byte("candidate"));actionErr:=store.publication(func(fileOperationStore)error{return original})
 if string(raw)!="retained-record"||string(held.raw)!="retained-record"||held.reads!=1||held.publishes!=1{t.Fatal("failed operation lost returned data or invented record effect")}
 for _,failure:=range []error{readErr,publishErr,actionErr}{if !errors.Is(failure,original)||failure.Error()!=storageError().Error(){t.Fatal("held original or safe presentation lost")};encoded,err:=json.Marshal(failure);if err!=nil||strings.Contains(string(encoded),original.Owner){t.Fatal("held original published")}}
}
func TestRecordJoinedErrorsPreserveOriginalsAndClosedProjection(t *testing.T){
 if recordJoinedErrors(nil,nil)!=nil{t.Fatal("nil cleanup created a failure")}
 first:=&privateNativeJoinCause{Owner:"private-native-owner",Raw:[]byte("private-native-bytes")};second:=errors.New("private-close-original")
 unresolved:=&recordPublicationUnresolved{}
 failure:=recordJoinedErrors(first,nil,second,unresolved)
 if !errors.Is(failure,first)||!errors.Is(failure,second)||!errors.Is(failure,unresolved){t.Fatal("original graph lost")}
 var actual *privateNativeJoinCause;if !errors.As(failure,&actual)||actual!=first{t.Fatal("native owner identity lost")}
 if failure.Error()!=unresolved.Error(){t.Fatal("unresolved custody hidden")}
 raw,err:=json.Marshal(failure);if err!=nil{t.Fatal(err)}
 for _,private:=range []string{first.Owner,string(first.Raw),second.Error(),"Owner","Raw","cause"}{if strings.Contains(failure.Error(),private)||strings.Contains(string(raw),private){t.Fatal("private graph published")}}
 outcome:=&recordStorageOutcomeError{Outcome:recordStorageOutcome{PublicationEffect:recordReplacementApplied,CleanupRelease:recordReleaseUnresolved,Replacement:[]byte("private-replacement")},Primary:first,Cleanup:second}
 joined:=recordJoinedErrors(outcome,unresolved);observed,ok:=recordOutcome(joined)
 if !ok||!observed.replacementApplied(outcome.Outcome.Replacement)||joined.Error()!=outcome.Error(){t.Fatal("actual applied outcome lost")}
 prior:=&recordStorageOutcomeError{Outcome:recordStorageOutcome{PublicationEffect:recordReplacementApplied,CleanupRelease:recordReleased,Replacement:[]byte("prior-applied")},Primary:first}
 later:=recordJoinedErrors(prior,unresolved);unchanged,ok:=recordOutcome(later)
 if !ok||unchanged.CleanupRelease!=recordReleased||!strings.Contains(later.Error(),prior.Error())||!strings.Contains(later.Error(),unresolved.Error()){t.Fatal("later uncertainty erased or prior outcome rewritten")}
 state:=operatorModel{saved:[]byte("unrelated-current-record")};state.storageWarning(later)
 if state.custodyOutcome==nil||state.custodyOutcome.CleanupRelease!=recordReleased||state.custodyWarning!=later.Error()||string(state.saved)!="unrelated-current-record"||len(state.custodyErrors)!=1||state.custodyErrors[0]!=later{t.Fatal("reducer erased later uncertainty, original failure, or unrelated record")}
 ordinary:=recordJoinedErrors(storageError(),first)
 if ordinary.Error()!=storageError().Error()||!errors.Is(ordinary,first){t.Fatal("storage presentation or cause changed")}
}
