package main

import (
 "flag"
 "testing"
 "github.com/service-lasso/service-lasso-tui/internal/lifetime"
)

// UNRUN. These reach the actual public command parser/return boundary without
// connection resolution or native admission. They do not qualify D2 exit.
func TestPublicParseReturnsSameOriginalBeforeOuterExit(t *testing.T){
 for _,arguments:=range [][]string{{"--not-a-public-flag"},{"--help"}}{
  receiver:=lifetime.NewReceiver();startup:=&startupOccurrence{}
  status:=runPublicCommand(startup,receiver,"original-public-entry",arguments,nil)
  if status!=2||startup.Status!=2||!startup.Returned||startup.Failure==nil||startup.Failure!=startup.ParseFailure||startup.Manager!=nil||startup.Client!=nil||startup.Initial!=nil{t.Fatal("parse outcome did not return its original before connection effects")}
  if startup.PublicZero!="original-public-entry"||&startup.PublicTail[0]!=&arguments[0]||startup.Flags.Name()!=startup.PublicZero{t.Fatal("public argument identity was projected or renamed")}
  for _,name:=range []string{"api","connections","profile","token-env","compat-v1"}{if startup.Flags.Lookup(name)==nil{t.Fatal("original public flag missing",name)}}
  if arguments[0]=="--help"&&startup.ParseFailure!=flag.ErrHelp{t.Fatal("help original/status changed")}
  first:=startup.ParseFailure;receiver.Join();found:=false
  if err:=receiver.Inspect(func(original *lifetime.Original){if original==&startup.Original{found=original.Value==startup}});err!=nil||!found{t.Fatal("actual joined receiver lost command root")}
  if runPublicCommand(startup,receiver,"replacement",nil,nil)!=2||startup.ParseFailure!=first||startup.PublicZero!="original-public-entry"{t.Fatal("duplicate command rewrote original outcome")}
 }
}
