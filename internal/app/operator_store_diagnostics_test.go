package app

import (
 "reflect"
 "testing"
)

// Test diagnostics project only the existing private native originals' closed
// boundary labels, numeric error codes and effect/release states. Never print
// paths, handles, payload, original record bytes or arbitrary Error strings.
func logRecordFailure(t *testing.T, failure error) {
 t.Helper()
 outcome,ok:=recordOutcome(failure)
 if !ok { t.Logf("storage failure type=%T (no observed native outcome)",failure); return }
 t.Logf("storage effect=%s release=%s originalKnown=%t",outcome.PublicationEffect,outcome.CleanupRelease,outcome.OriginalKnown)
 var cause func(string,error)
 cause=func(boundary string,err error){
  if err==nil{return}
  value:=reflect.ValueOf(err)
  switch value.Kind(){
  case reflect.Uint,reflect.Uint32,reflect.Uint64,reflect.Uintptr:
   t.Logf("storage boundary=%s nativeType=%T nativeCode=%d",boundary,err,value.Uint())
  }
  if joined,ok:=err.(interface{Unwrap()[]error});ok{for _,child:=range joined.Unwrap(){cause(boundary,child)}}else if wrapped,ok:=err.(interface{Unwrap()error});ok{cause(boundary,wrapped.Unwrap())}
 }
 for _,call:=range outcome.NativeCalls{if original,ok:=call.(recordNativeCallOriginal);ok{cause(original.nativeCallOwner().Kind,original.nativeCallFailure())}}
 for _,helper:=range outcome.HelperFailures{cause(helper.Owner.Kind,helper.Cause)}
 for _,helper:=range outcome.FileHelpers{cause(helper.Kind,helper.Cause)}
 for _,mismatch:=range outcome.IdentityMismatches{t.Logf("storage identity mismatch boundary=%s",mismatch.Owner.Kind)}
 for _,mismatch:=range outcome.BodyMismatches{t.Logf("storage body mismatch boundary=%s",mismatch.Owner.Kind)}
 cause("primary",failure)
}
