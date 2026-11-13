package lifetime

import("os";"runtime";"testing")

type custodyInterruptedEndpoint struct{goexit bool;writes int}
func(e *custodyInterruptedEndpoint)Write([]byte)(int,error){e.writes++;if e.goexit{runtime.Goexit()};panic(nil)}
func(e *custodyInterruptedEndpoint)Read([]byte)(int,error){panic("unexpected read after interrupted write")}
// UNRUN: actual prepared+joined exchange, both legacy nil-panic and Goexit.
// Each episode owns separate receiver/transfer/endpoint and keeps partial IO.
func TestLocalExchangeInterruptedWriteDoesNotReturnSuccess(t *testing.T){
 t.Setenv("GODEBUG","panicnil=1")
 for _,goexit:=range []bool{false,true}{
  receiver:=NewReceiver();endpoint:=&custodyInterruptedEndpoint{goexit:goexit};transfer:=&LocalTransfer{Endpoint:endpoint,Body:[]byte{7}}
  if receiver.PrepareLocalRepresentation(transfer)!=nil{t.Fatal("preparation failed")};receiver.Join()
  done:=make(chan struct{});resumed:=false;returned:=false
  go func(){defer close(done);func(){defer func(){recover()}();receiver.ExchangeLocalRepresentation(transfer);returned=true}();resumed=true}()
  <-done
  if returned||resumed==goexit||!transfer.Interrupted||transfer.Returned||transfer.Borrow.Returned||transfer.Panic!=nil||len(transfer.Stack)==0||endpoint.writes!=1||len(transfer.HeaderWrite.Attempts)!=1||transfer.exchanging||receiver.terminalIO!=0{t.Fatal("interrupted exchange became success or changed Goexit")}
  if receiver.ExchangeLocalRepresentation(transfer)==nil||endpoint.writes!=1{t.Fatal("uncertain exchange retried")}
 }
}
func TestTerminalInterruptedWritePoisonsJoinedCustody(t *testing.T){
 t.Setenv("GODEBUG","panicnil=1")
 for _,goexit:=range []bool{false,true}{
  receiver:=NewReceiver();endpoint:=&custodyInterruptedEndpoint{goexit:goexit};transfer:=&LocalTransfer{Endpoint:endpoint,Body:[]byte{9}}
  if receiver.PrepareLocalRepresentation(transfer)!=nil{t.Fatal("preparation failed")}
  control:=&TerminalControl{Transfer:transfer,Calls:make([]TerminalCall,1)}
  if receiver.PrepareTerminalControl(control)!=nil{t.Fatal("terminal preparation failed")}
  done:=make(chan struct{});resumed:=false;returned:=false
  go func(){defer close(done);func(){defer func(){recover()}();control.Fallback();returned=true}();resumed=true}()
  <-done;receiver.Join();call:=&control.Calls[0]
  if returned||resumed==goexit||!call.Interrupted||call.Returned||call.Borrow.Returned||call.Panic!=nil||call.Failure!=errTerminalControlAbnormal||transfer.streamFailureOriginal!=call{t.Fatal("terminal interruption lost or changed")}
  if receiver.ExchangeLocalRepresentation(transfer)!=errTerminalControlAbnormal||endpoint.writes!=1||!transfer.Returned||transfer.Interrupted{t.Fatal("poisoned endpoint reused for custody")}
 }
}
type collectionInterruptedEndpoint struct{goexit bool;reads,writes int}
func(e *collectionInterruptedEndpoint)Read([]byte)(int,error){e.reads++;if e.goexit{runtime.Goexit()};panic(nil)}
func(e *collectionInterruptedEndpoint)Write([]byte)(int,error){e.writes++;panic("unexpected collector reply")}
func TestCollectionInterruptedReadRetainsUnreturnedOwner(t *testing.T){
 t.Setenv("GODEBUG","panicnil=1")
 for _,goexit:=range []bool{false,true}{
  endpoint:=&collectionInterruptedEndpoint{goexit:goexit}
  // The placeholder storage is never called: first header-read interruption
  // must prevent every storage and reply operation in this actual body.
  collection:=&LocalCollection{Endpoint:endpoint,Storage:new(os.File)}
  if PrepareLocalCollection(collection,1)!=nil{t.Fatal("preparation failed")}
  done:=make(chan struct{});resumed:=false;returned:=false
  go func(){defer close(done);func(){defer func(){recover()}();collection.Collect();returned=true}();resumed=true}()
  <-done
  if returned||resumed==goexit||!collection.Interrupted||collection.Returned||collection.Panic!=nil||len(collection.Stack)==0||endpoint.reads!=1||endpoint.writes!=0||len(collection.HeaderRead.Attempts)!=1||collection.Write.Count!=0||collection.Read.Count!=0||collection.SyncFailure!=nil{t.Fatal("collector interruption became completed custody")}
  if collection.Collect()==nil||endpoint.reads!=1{t.Fatal("uncertain collector retried")}
 }
}
