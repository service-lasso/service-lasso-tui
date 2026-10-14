//go:build linux

package cancelreader

import("errors";"os";"testing")

// UNRUN: actual concurrent native reader/cancel/close source relation.
func TestOwnedLinuxConcurrentReadsCancelAndCloseJoinActualBorrowers(t *testing.T){
 input,writer,err:=os.Pipe();if err!=nil{t.Fatal(err)};defer input.Close();defer writer.Close()
 var owner *Ownership;reader,err:=NewReaderOwned(input,func(value *Ownership){owner=value});if err!=nil{t.Fatal(err)}
 actual:=reader.(*epollCancelReader);results:=make(chan error,2);admitted:=make(chan struct{},2);waiting:=make(chan struct{},2);actual.readAdmitted=func(){admitted<-struct{}{}};actual.waitEntered=func(){waiting<-struct{}{}};actual.readSerial.Lock()
 for i:=0;i<2;i++{go func(){_,readErr:=reader.Read(make([]byte,1));results<-readErr}()};<-admitted;<-admitted;actual.readSerial.Unlock();<-waiting
 if !reader.Cancel(){t.Fatal("actual cancellation failed")};firstClose:=reader.Close();if firstClose!=nil{t.Fatal(firstClose)}
 for i:=0;i<2;i++{readErr:=<-results;if !errors.Is(readErr,ErrCanceled)&&!errors.Is(readErr,os.ErrClosed){t.Fatal(readErr)}}
 if reader.Close()!=firstClose{t.Fatal("once cleanup result changed")}
 actual.state.Lock();closed:=actual.closing;actual.state.Unlock();if !closed{t.Fatal("read admission remained open")}
 closes:=0;owner.VisitOriginals(func(operation *OriginalOperation){if operation.Source=="epoll.close"||operation.Source=="cancel-pipe.close"{closes++;if !operation.Returned{t.Fatal("actual cleanup not returned")}}});if closes!=3{t.Fatal("independent once cleanup missing or repeated",closes)}
}