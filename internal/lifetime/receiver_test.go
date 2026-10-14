package lifetime

import (
 "errors"
 "runtime"
 "testing"
)

// UNRUN source cases: actual worker entry/delivery joins and SAME graph aliases.
// These cases do not establish native ownership, finite fit or disposal rights.
func TestJoinWaitsForActualDeliveryAndRetainsSameGraph(t *testing.T){
 receiver:=NewReceiver();task:=&Task{};entered:=make(chan struct{});release:=make(chan struct{});joined:=make(chan struct{})
 cause:=errors.New("private actual failure");graph:=&struct{Cause error}{cause}
 if err:=receiver.Dispatch(task,"test.actual-command",func()any{return graph},func(value any){if value!=graph{t.Error("result graph replaced")};close(entered);<-release});err!=nil{t.Fatal(err)}
 <-entered;go func(){receiver.Join();close(joined)}()
 select{case <-joined:t.Fatal("Join returned while delivery was still executing");default:}
 close(release);<-joined
 if task.Result!=graph||graph.Cause!=cause||!task.DeliveryReturned||task.AbnormalTermination{t.Fatal("actual result/delivery relation lost")}
 found:=false;if err:=receiver.Inspect(func(original *Original){if original==&task.Original{found=original.Value==task}});err!=nil||!found{t.Fatal("actual task owner lost")}
}
func TestStoppedDispatchNeverEntersCommandOrManufacturesAbnormalExit(t *testing.T){
 receiver:=NewReceiver();receiver.Stop();task:=&Task{};entered:=false
 if err:=receiver.Dispatch(task,"test.stopped",func()any{entered=true;return nil},nil);err==nil{t.Fatal("stopped dispatch admitted")}
 receiver.Join();if entered||task.Started||task.AbnormalTermination{t.Fatal("stopped source confused with actual producer failure")}
}
func TestGoexitRemainsUnresolvedActualTask(t *testing.T){
 receiver:=NewReceiver();task:=&Task{};entered:=make(chan struct{})
 if err:=receiver.Dispatch(task,"test.goexit",func()any{close(entered);runtime.Goexit();return nil},nil);err!=nil{t.Fatal(err)}
 <-entered;receiver.Join();if !task.Started||task.Returned||!task.AbnormalTermination||task.DeliveryReturned{t.Fatal("Goexit falsely treated as completed delivery")}
}
func TestInspectionRequiresActualJoinAndAllowsReceiverReentry(t *testing.T){
 receiver:=NewReceiver();original:=&Original{};if err:=receiver.Enroll(original,"test.original",original);err!=nil{t.Fatal(err)}
 if err:=receiver.Inspect(func(*Original){t.Error("unjoined inspection ran")});err==nil{t.Fatal("unjoined inspection admitted")}
 receiver.Join();if err:=receiver.Inspect(func(*Original){receiver.Stop()});err!=nil{t.Fatal(err)}
 if err:=receiver.Enroll(&Original{},"test.late",nil);err==nil{t.Fatal("new source admitted after joined inspection cut")}
}
func TestBorrowRejectsForeignOwnerAndKeepsSameRoot(t *testing.T){
 receiver:=NewReceiver();other:=NewReceiver();root:=&Original{};if err:=receiver.Enroll(root,"test.root",root);err!=nil{t.Fatal(err)}
 if err:=other.Borrow(&Borrow{},root,"test.foreign",root);err==nil{t.Fatal("foreign root admitted")}
 borrow:=&Borrow{};if err:=receiver.Borrow(borrow,root,"test.borrow",root);err!=nil{t.Fatal(err)};receiver.ReturnBorrow(borrow)
 if borrow.Root!=root||borrow.Consumer!=root||!borrow.Returned{t.Fatal("same actual borrow relation lost")}
}
func TestActualAbnormalProducerNotifiesBeforeJoinReturns(t *testing.T){
 receiver:=NewReceiver();task:=&Task{};entered:=make(chan struct{});cause:=errors.New("actual command panic");var received *Task
 task.OnAbnormal=func(value *Task){received=value}
 if err:=receiver.Dispatch(task,"test.panic-notification",func()any{close(entered);panic(cause)},nil);err!=nil{t.Fatal(err)}
 <-entered;receiver.Join()
 if received!=task||task.Panic!=cause||!task.NotificationAttempted||!task.NotificationReturned{t.Fatal("actual failure did not reach joined notification source")}
}
