package app

import (
 "bytes"
 "encoding/json"
 "errors"
 "os"
 "io"
 "strings"
 "testing"
)

// These exercise original local Go file results only, not native durable EOF.
func TestRecordFileReadKeepsActualBoundedSliceAndOriginalOwner(t *testing.T){
 file,err:=os.CreateTemp(t.TempDir(),"read-original-");if err!=nil{t.Fatal(err)}
 defer file.Close()
 if _,err:=file.Write([]byte("original-private-tail"));err!=nil{t.Fatal(err)}
 if _,err:=file.Seek(0,0);err!=nil{t.Fatal(err)}
 outcome:=recordStorageOutcome{}
 raw,err:=recordReadFile(&outcome,file,recordNativeIdentity{Kind:"source-file",IdentityKnown:true},8)
 if err!=nil||string(raw)!="original"||len(outcome.FileReads)!=1{t.Fatal("actual original bounded read lost")}
 read:=outcome.FileReads[0]
 if read.File!=file||read.Bound!=8||!read.Attempted||!read.Completed||read.Cause!=nil||len(read.Raw)!=len(raw)||&read.Raw[0]!=&raw[0]{t.Fatal("original read replaced by copied bytes or another owner")}
 // A bounded result never claims file EOF; the existing unconsumed tail stays.
 tail:=make([]byte,1);if n,err:=file.Read(tail);n!=1||err!=nil||tail[0]!='-'{t.Fatal("bounded read consumed or invented the source tail")}
}
func TestRecordFileReadFailureKeepsExactCauseWithoutPublicBytes(t *testing.T){
 file,err:=os.CreateTemp(t.TempDir(),"read-failure-");if err!=nil{t.Fatal(err)}
 if err:=file.Close();err!=nil{t.Fatal(err)}
 outcome:=recordStorageOutcome{PublicationEffect:recordUnchanged,CleanupRelease:recordReleaseUnresolved}
 raw,cause:=recordReadFile(&outcome,file,recordNativeIdentity{Kind:"private-owner",IdentityKnown:true},4097)
 if !errors.Is(cause,os.ErrClosed)||len(outcome.FileReads)!=1{t.Fatal("actual file read failure lost")}
 read:=outcome.FileReads[0]
 if read.File!=file||read.Cause!=cause||!read.Completed||!bytes.Equal(read.Raw,raw){t.Fatal("original failed read observation replaced")}
 failure:=&recordStorageOutcomeError{Outcome:outcome}
 if !errors.Is(failure,cause){t.Fatal("private original read is unreachable")}
 encoded,err:=json.Marshal(failure);if err!=nil||strings.Contains(string(encoded),file.Name())||strings.Contains(string(encoded),"private-owner"){t.Fatal("file read originals entered safe public projection")}
}
func TestRecordPositionedReadRetainsPartialBufferAndActualEOF(t *testing.T){
 file,err:=os.CreateTemp(t.TempDir(),"positioned-read-");if err!=nil{t.Fatal(err)};defer file.Close()
 if _,err:=file.Write([]byte("abc"));err!=nil{t.Fatal(err)}
 raw:=make([]byte,8);outcome:=recordStorageOutcome{}
 n,cause:=recordReadFileAt(&outcome,file,recordNativeIdentity{Kind:"source-file",IdentityKnown:true},raw,1)
 if n!=2||cause!=io.EOF||len(outcome.FileReads)!=1{t.Fatal("actual partial ReadAt result lost")}
 read:=outcome.FileReads[0]
 if read.File!=file||read.Offset!=1||!read.ReadAt||read.Transferred!=2||read.Cause!=cause||&read.Raw[0]!=&raw[0]||string(read.Raw[:n])!="bc"{t.Fatal("positioned original extent or buffer replaced")}
}
func TestRecordPositionedWritePreservesOriginalInputAndFailure(t *testing.T){
 path:=t.TempDir()+string(os.PathSeparator)+"private-source-file"
 if err:=os.WriteFile(path,[]byte("prior"),0600);err!=nil{t.Fatal(err)}
 file,err:=os.Open(path);if err!=nil{t.Fatal(err)};defer file.Close()
 raw:=[]byte("private-replacement");outcome:=recordStorageOutcome{}
 n,cause:=recordWriteFile(&outcome,file,recordNativeIdentity{Kind:"private-write-owner",IdentityKnown:true},raw,true,3)
 if cause==nil||n!=0||len(outcome.FileWrites)!=1{t.Fatal("actual refused positioned write lost")}
 write:=outcome.FileWrites[0]
 if write.File!=file||!write.WriteAt||write.Offset!=3||!write.Attempted||!write.Completed||write.Cause!=cause||&write.Raw[0]!=&raw[0]{t.Fatal("original input/file/result identity replaced")}
 failure:=&recordStorageOutcomeError{Outcome:outcome}
 if !errors.Is(failure,cause){t.Fatal("positioned write original not reachable")}
 after,err:=os.ReadFile(path);if err!=nil||string(after)!="prior"{t.Fatal("failed write fixture changed original bytes")}
}
func TestRecordUnknownNativeIdentityDeniesBeforeReadOrWrite(t *testing.T){
 file,err:=os.CreateTemp(t.TempDir(),"unknown-native-owner-");if err!=nil{t.Fatal(err)};defer file.Close()
 if _,err:=file.Write([]byte("original"));err!=nil{t.Fatal(err)}
 if _,err:=file.Seek(0,0);err!=nil{t.Fatal(err)}
 outcome:=recordStorageOutcome{}
 if _,err:=recordReadFile(&outcome,file,recordNativeIdentity{},8);err==nil{t.Fatal("unknown native owner read")}
 if _,err:=recordWriteFile(&outcome,file,recordNativeIdentity{},[]byte("replacement"),false,0);err==nil{t.Fatal("unknown native owner wrote")}
 if outcome.FileReads[0].Attempted||outcome.FileReads[0].Completed||outcome.FileWrites[0].Attempted||outcome.FileWrites[0].Completed{t.Fatal("refusal fabricated an IO observation")}
 raw:=make([]byte,8);if n,err:=file.Read(raw);n!=8||err!=nil||string(raw)!="original"{t.Fatal("refused native IO changed source bytes or cursor")}
}
func TestRecordFileHelpersRetainOriginalResults(t *testing.T){
 file,err:=os.CreateTemp(t.TempDir(),"helper-original-");if err!=nil{t.Fatal(err)};defer file.Close()
 outcome:=recordStorageOutcome{};owner:=recordNativeIdentity{IdentityKnown:true}
 info,err:=recordFileStat(&outcome,file,owner);if err!=nil{t.Fatal(err)}
 if len(outcome.FileHelpers)!=1||outcome.FileHelpers[0].File!=file||outcome.FileHelpers[0].Info!=info||!outcome.FileHelpers[0].Completed{t.Fatal("original metadata object was not retained")}
 position,seekErr:=recordFileSeek(&outcome,file,owner,2,io.SeekStart);if seekErr!=nil||position!=2||outcome.FileHelpers[len(outcome.FileHelpers)-1].Position!=position||outcome.FileHelpers[len(outcome.FileHelpers)-1].File!=file{t.Fatal("original seek result/owner was not retained")}
 if err:=recordFileTruncate(&outcome,file,owner,3);err!=nil{t.Fatal(err)}
 if err:=recordFileSync(&outcome,file,owner);err!=nil{t.Fatal(err)}
 if err:=file.Close();err!=nil{t.Fatal(err)}
 cause:=recordFileSync(&outcome,file,owner);last:=outcome.FileHelpers[len(outcome.FileHelpers)-1]
 if cause==nil||last.Cause!=cause||last.File!=file||!last.Attempted||!last.Completed{t.Fatal("original helper failure was not retained")}
 wrapped:=&recordStorageOutcomeError{Outcome:outcome};if !errors.Is(wrapped,os.ErrClosed){t.Fatal("original helper failure graph was lost")}
}
func TestArbiterSnapshotIncludesOriginalIOAndHelperFailures(t *testing.T){
 readCause:=errors.New("private read original");writeCause:=errors.New("private write original");helperCause:=errors.New("private sync original")
 owner:=recordNativeIdentity{IdentityKnown:true}
 outcome:=recordStorageOutcome{OriginalKnown:true,PublicationEffect:recordUnchanged,CleanupRelease:recordReleased,FileReads:[]*recordFileRead{{Owner:owner,Cause:readCause}},FileWrites:[]*recordFileWrite{{Owner:owner,Cause:writeCause}},FileHelpers:[]*recordFileHelper{{Owner:owner,Kind:"sync",Cause:helperCause}}}
 snapshot,err:=recordArbiterSnapshot(outcome);if err!=nil{t.Fatal(err)}
 if len(snapshot.Causes)!=3||snapshot.Causes[0].Message!=readCause.Error()||snapshot.Causes[1].Message!=writeCause.Error()||snapshot.Causes[2].Message!=helperCause.Error(){t.Fatal("durable arbitration projection omitted original helper failures")}
 // Private exact representations supplement, and never replace, live originals.
 if outcome.FileReads[0].Cause!=readCause||outcome.FileWrites[0].Cause!=writeCause||outcome.FileHelpers[0].Cause!=helperCause{t.Fatal("snapshot replaced original error objects")}
}