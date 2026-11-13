package main

import (
 "errors"
 "flag"
 "runtime"
 "testing"
 "github.com/service-lasso/service-lasso-tui/internal/lifetime"
)

type parserDiagnosticFixture struct{failure error;panicOriginal any;calls int;inputs [][]byte}
func(writer *parserDiagnosticFixture)Write(body []byte)(int,error){
 writer.calls++;writer.inputs=append(writer.inputs,append([]byte(nil),body...))
 if writer.panicOriginal!=nil{panic(writer.panicOriginal)}
 if len(body)==0{return 0,writer.failure};return len(body)-1,writer.failure
}
type parserNilPanicFixture struct{}
func(parserNilPanicFixture)Write([]byte)(int,error){panic(nil)}
type parserGoexitFixture struct{}
func(parserGoexitFixture)Write([]byte)(int,error){runtime.Goexit();return 0,nil}
// UNRUN. This future test's own runtime setting exercises the supported legacy
// nil-panic branch; no environment or target execution was performed to author it.
func TestParserDiagnosticNilPanicIsRethrown(t *testing.T){
 t.Setenv("GODEBUG","panicnil=1")
 writer:=&parserDiagnosticWriter{Writer:parserNilPanicFixture{}}
 outerReturned:=false;recoveredBoundary:=false
 func(){defer func(){recover();recoveredBoundary=true}();writer.Write([]byte("same input"));outerReturned=true}()
 row:=writer.First
 if outerReturned||!recoveredBoundary||row==nil||!row.Interrupted||row.Returned||row.Panic!=nil||!row.Attempted{t.Fatal("legacy nil panic was swallowed or projected")}
}
func TestParserDiagnosticGoexitKeepsUnwinding(t *testing.T){
 writer:=&parserDiagnosticWriter{Writer:parserGoexitFixture{}}
 done:=make(chan struct{});returned:=false;resumedAfterRecoveredCall:=false;var outerPanic any
 go func(){defer close(done);func(){defer func(){outerPanic=recover()}();writer.Write([]byte("same input"));returned=true}();resumedAfterRecoveredCall=true}()
 <-done;row:=writer.First
 if returned||resumedAfterRecoveredCall||outerPanic!=nil||row==nil||!row.Interrupted||row.Returned||row.Panic!=nil||!row.Attempted{t.Fatal("Goexit converted or partial occurrence lost")}
}
// UNRUN: actual FlagSet error/help paths, not a replacement parser. Diagnostic
// short/error results remain independent of the unchanged parse outcome.
func TestParserDiagnosticOutcomesRemainWithPublicOccurrence(t *testing.T){
 for _,argument:=range []string{"--help","--unknown-original-flag"}{
  cause:=errors.New("original diagnostic write failure")
  writer:=&parserDiagnosticFixture{failure:cause};startup:=&startupOccurrence{}
  startup.ParserDiagnostic.Writer=writer;receiver:=lifetime.NewReceiver()
  if runPublicCommand(startup,receiver,"public-entry",[]string{argument},nil)!=2||startup.Failure!=startup.ParseFailure{t.Fatal("parser status/original changed")}
  if argument=="--help"&&startup.ParseFailure!=flag.ErrHelp{t.Fatal("help changed")}
  index:=0
  for row:=startup.ParserDiagnostic.First;row!=nil;row=row.Next{
   if index>=len(writer.inputs)||string(row.Bytes)!=string(writer.inputs[index])||row.Writer!=writer||!row.Attempted||!row.Returned||row.Interrupted||row.Failure!=cause||row.Written!=len(row.Bytes)-1{t.Fatal("actual diagnostic result lost")};index++
  }
  if index==0||index!=writer.calls{t.Fatal("missing actual writes")}
  receiver.Join();found:=false
  if receiver.Inspect(func(original *lifetime.Original){if original==&startup.Original{found=original.Value==startup&&startup.ParserDiagnostic.First!=nil}})!=nil||!found{t.Fatal("diagnostic owner lost")}
 }
}
func TestParserDiagnosticPanicRetainsSamePartialOccurrence(t *testing.T){
 original:=&struct{value string}{"actual writer panic"};writer:=&parserDiagnosticFixture{panicOriginal:original}
 startup:=&startupOccurrence{};startup.ParserDiagnostic.Writer=writer;receiver:=lifetime.NewReceiver()
 func(){defer func(){if recover()!=original{t.Fatal("panic original changed")}}();runPublicCommand(startup,receiver,"public-entry",[]string{"--help"},nil)}()
 row:=startup.ParserDiagnostic.First
 if row==nil||!row.Attempted||row.Returned||!row.Interrupted||row.Panic!=original||startup.Returned||startup.ParseFailure!=nil||writer.calls!=1{t.Fatal("partial parser/output occurrence lost")}
}
