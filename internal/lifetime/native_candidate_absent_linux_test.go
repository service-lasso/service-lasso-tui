//go:build linux && !service_lasso_native_candidate

package lifetime

import "testing"

// UNRUN until whole-source review and exact-input command admission.
func TestAbsentNativeCandidateRetainsOriginalRejection(t *testing.T){
 arguments:=[]string{"original","--profile","retained"}
 startup,receiver,failure:=NewNativePayloadCommand(arguments)
 if failure!=errNativeCandidateUnavailable||startup==nil||receiver==nil{t.Fatal("absent candidate invented a grant or lost its owner")}
 if startup.Failure!=failure||!startup.Attempted||!startup.Returned||startup.Original.Value!=startup||startup.receiverOriginal!=receiver{t.Fatal("actual rejected construction not retained")}
 if &startup.RawArguments[0]!=&arguments[0]||startup.reservation!=nil||startup.candidateOriginal!=nil||startup.Endpoint.prepared||startup.Creator.CredentialEntered{t.Fatal("absent candidate acquired or synthesized inputs")}
 repeated,repeatedReceiver,repeatedFailure:=NewNativePayloadCommand([]string{"replacement"})
 if repeatedFailure!=errOriginalEnrollment||repeated!=startup||repeatedReceiver!=receiver||startup.RawArguments[0]!="original"||startup.Failure!=failure{t.Fatal("duplicate construction overwrote original rejection")}
}
