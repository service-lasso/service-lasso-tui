//go:build linux && !service_lasso_native_candidate

package lifetime

import (
 "errors"
 "sync/atomic"
)

// Ordinary source builds have no admitted candidate profile or fixed geometry.
// Retain their actual rejected construction once; this is never a native grant.
var errNativeCandidateUnavailable=errors.New("native candidate source is not installed")
var absentNativeCandidate struct {
 claimed atomic.Bool
 receiver Receiver
 startup NativePayloadStartup
}

func newNativeCandidateStartupRoot(rawArguments []string)(*NativePayloadStartup,error){
 owner:=&absentNativeCandidate
 if !owner.claimed.CompareAndSwap(false,true){return &owner.startup,errOriginalEnrollment}
 startup:=&owner.startup
 startup.receiverOriginal=&owner.receiver
 startup.RawArguments=rawArguments
 startup.Attempted=true
 startup.Failure=errNativeCandidateUnavailable
 startup.Returned=true
 if err:=owner.receiver.Enroll(&startup.Original,"native-payload.candidate-unavailable",startup);err!=nil{
  startup.Failure=err
  return startup,err
 }
 return startup,startup.Failure
}
