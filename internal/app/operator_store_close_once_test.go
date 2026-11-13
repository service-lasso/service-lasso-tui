//go:build linux || windows

package app

import (
 "errors"
 "os"
 "testing"
)

// Actual local file/helper source regressions only. They do not qualify the
// platform publication primitive, native caller lifetime or durable custody.
func TestRecordFileCloseJoinsOriginalSuccessfulObservation(t *testing.T){
 file,err:=os.CreateTemp(t.TempDir(),"owned-close-");if err!=nil{t.Fatal(err)}
 closeFile,outcome:=recordCloseSourceFixture()
 if err:=closeFile(file);err!=nil{t.Fatal(err)}
 first:=outcome();if len(first.FileCloses)!=1||len(first.Releases)!=1{t.Fatal("original successful close was not observed")}
 observation:=first.FileCloses[0]
 if observation.File!=file||!observation.Attempted||!observation.Completed||observation.Cause!=nil||first.Releases[0].Cause!=nil{t.Fatal("actual successful file close identity or result lost")}
 if err:=closeFile(file);err!=nil{t.Fatal("repeat retried an already completed Close")}
 second:=outcome();if len(second.FileCloses)!=1||second.FileCloses[0]!=observation||len(second.Releases)!=1||second.CleanupRelease!=recordReleased{t.Fatal("repeat fabricated another release or uncertainty")}
}
func TestRecordFailedFileCloseRetainsSameOriginalAndDoesNotRetry(t *testing.T){
 file,err:=os.CreateTemp(t.TempDir(),"failed-close-");if err!=nil{t.Fatal(err)}
 // This fixture supplies an actually closed File; the helper's own Close then
 // returns its original os.PathError, rather than a fabricated failure string.
 if err:=file.Close();err!=nil{t.Fatal(err)}
 closeFile,outcome:=recordCloseSourceFixture()
 firstErr:=closeFile(file);if !errors.Is(firstErr,os.ErrClosed){t.Fatal("actual failed Close cause lost")}
 first:=outcome();if len(first.FileCloses)!=1||len(first.Releases)!=1||len(first.RetainedOwners)!=1{t.Fatal("failed close original lifetime omitted")}
 observation:=first.FileCloses[0]
 if observation.File!=file||observation.Cause!=firstErr||!observation.Attempted||!observation.Completed||first.RetainedOwners[0].Object!=file||first.RetainedOwners[0].Cause!=firstErr||!first.RetainedOwners[0].CloseAttempted{t.Fatal("failed Close replaced original file, error or observation")}
 if again:=closeFile(file);again!=firstErr{t.Fatal("repeat replaced original error or attempted Close again")}
 second:=outcome();if len(second.FileCloses)!=1||second.FileCloses[0]!=observation||len(second.Releases)!=1||len(second.RetainedOwners)!=1||second.CleanupRelease!=recordReleaseUnresolved{t.Fatal("repeat changed retained failure custody")}
}
func TestRecordFinalizerRepeatsJoinSameActualResult(t *testing.T){
 for _,failed:=range []bool{false,true}{
  file,err:=os.CreateTemp(t.TempDir(),"finalizer-");if err!=nil{t.Fatal(err)}
  if failed{if err:=file.Close();err!=nil{t.Fatal(err)}}
  finalize,outcome:=recordFinalizerSourceFixture(file)
  firstErr:=finalize();first:=outcome()
  if failed&&!errors.Is(firstErr,os.ErrClosed)||!failed&&firstErr!=nil{t.Fatal("finalizer changed actual original Close result")}
  if len(first.FileCloses)!=1||len(first.Releases)!=1{t.Fatal("finalizer omitted actual Close")}
  if failed{if first.CleanupRelease!=recordReleaseUnresolved||len(first.HelperFailures)==0||!errors.Is(first.HelperFailures[0].Cause,os.ErrClosed){t.Fatal("finalizer failed to retain original release failure")};snapshot,snapshotErr:=recordArbiterSnapshot(first);if snapshotErr!=nil||snapshot.Phase!=recordArbiterPoisoned{t.Fatal("failed finalizer could publish ready arbitration custody")}}
  if again:=finalize();again!=firstErr{t.Fatal("repeated finalizer replaced first result")}
  second:=outcome();if len(second.FileCloses)!=1||second.FileCloses[0]!=first.FileCloses[0]||len(second.Releases)!=1{t.Fatal("repeated finalizer touched released native values")}
 }
}
func TestPreparedDiscardRepeatDoesNotQueryReleasedOriginal(t *testing.T){
 for _,failed:=range []bool{false,true}{
  file,err:=os.CreateTemp(t.TempDir(),"discard-once-");if err!=nil{t.Fatal(err)}
  if failed{if err:=file.Close();err!=nil{t.Fatal(err)}}
  discard,outcome:=recordDiscardSourceFixture(file);firstErr:=discard();first:=outcome()
  if failed&&!errors.Is(firstErr,os.ErrClosed)||!failed&&firstErr!=nil{t.Fatal("original prepared cleanup result changed")}
  if again:=discard();again!=firstErr{t.Fatal("repeat prepared cleanup replaced original result")}
  second:=outcome()
  if len(second.IdentityFailures)!=len(first.IdentityFailures)||len(second.FileCloses)!=len(first.FileCloses)||len(second.Releases)!=len(first.Releases)||second.CleanupRelease!=first.CleanupRelease{t.Fatal("repeat queried or closed released original native value")}
 }
}