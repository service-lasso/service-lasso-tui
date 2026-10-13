package tea

import ("sync";"runtime/debug";"errors";"io")

// SourceOccurrence retains actual private library operation objects and results.
// It conveys no native authority, source completion, custody ACK or disposition.
type SourceOccurrence struct {
 Source string
 Subject,Input,Output any
 Worker,Binding,Invocation any
 Failure error
 ReturnedError error
 Panic any
 Stack []byte
 Started,Returned bool
 WorkerLaunched,WorkerEntered,WorkerReturned bool
 next *SourceOccurrence
}
// sourceOwnership belongs to the SAME Program before its input/native effects.
// Profile fit and finite terminal/failure reservation are separate obligations.
type sourceOwnership struct{mutex sync.Mutex;head *SourceOccurrence;workers sync.WaitGroup;stopped bool}
func(p *Program)sourceStart(source string,subject,input any)*SourceOccurrence{
 occurrence:=&SourceOccurrence{Source:source,Subject:subject,Input:input,Started:true}
 if p==nil{return occurrence};p.sourceOwnership.mutex.Lock();occurrence.next=p.sourceOwnership.head;p.sourceOwnership.head=occurrence;p.sourceOwnership.mutex.Unlock();return occurrence
}
func(p *Program)sourceError(source string,subject,input any,invoke func()error)error{
 occurrence:=p.sourceStart(source,subject,input);occurrence.Invocation=invoke;occurrence.Failure=invoke();occurrence.ReturnedError=occurrence.Failure;occurrence.Returned=true;return occurrence.Failure
}
// WithProgramOwner attaches the actual Program before the remaining options or
// constructors can produce originals. The required TUI runner puts this first.
func WithProgramOwner(attach func(*Program))ProgramOption{return func(p *Program){if attach!=nil{attach(p)}}}
// VisitSourceOriginals only exposes SAME library occurrences after actual Run
// completion. This inspection is not an eligible cleanup or archive receipt.
func(p *Program)VisitSourceOriginals(visit func(*SourceOccurrence))error{
 if p.finished==nil{return ErrProgramKilled};select{case <-p.finished:default:return ErrProgramKilled}
 p.sourceOwnership.mutex.Lock();head:=p.sourceOwnership.head;p.sourceOwnership.mutex.Unlock()
 for occurrence:=head;occurrence!=nil;occurrence=occurrence.next{visit(occurrence)};return nil
}

// sourceWorker owns callback/result/panic before goroutine creation. Cancellation
// is a request; workers stay owned until their real function and delivery return.
func(p *Program)sourceWorker(source string,subject,input any,invoke func(*SourceOccurrence))bool{return p.sourceWorkerBound(source,subject,input,invoke,nil)}
func(p *Program)sourceWorkerBound(source string,subject,input any,invoke func(*SourceOccurrence),bind func(*SourceOccurrence))bool{
 occurrence:=p.sourceStart(source,subject,input);occurrence.Worker=invoke;occurrence.Binding=bind;if bind!=nil{bind(occurrence)}
 p.sourceOwnership.mutex.Lock();if p.sourceOwnership.stopped{occurrence.Failure=ErrProgramKilled;occurrence.ReturnedError=ErrProgramKilled;occurrence.Returned=true;p.sourceOwnership.mutex.Unlock();return false};occurrence.WorkerLaunched=true;p.sourceOwnership.workers.Add(1);p.sourceOwnership.mutex.Unlock()
 go func(){defer p.sourceOwnership.workers.Done();finished:=false;defer func(){original:=recover();if !finished{occurrence.Panic=original;occurrence.Stack=debug.Stack();occurrence.Failure=ErrProgramPanic;occurrence.ReturnedError=ErrProgramPanic;p.cancel()}}();occurrence.WorkerEntered=true;invoke(occurrence);occurrence.WorkerReturned=true;occurrence.Returned=true;finished=true}();return true
}
func(p *Program)stopSourceWorkers(){p.sourceOwnership.mutex.Lock();p.sourceOwnership.stopped=true;p.sourceOwnership.mutex.Unlock()}
func(p *Program)joinSourceWorkers(){p.sourceOwnership.workers.Wait()}
func(p *Program)sourceFailures()error{
 p.sourceOwnership.mutex.Lock();defer p.sourceOwnership.mutex.Unlock()
 var result error;for occurrence:=p.sourceOwnership.head;occurrence!=nil;occurrence=occurrence.next{result=errors.Join(result,occurrence.ReturnedError)};return result
}

// sourceWriter records the actual output operation before invoking its writer.
// The original input alias and a truthful pre-write byte snapshot are distinct.
type sourceWriteOriginal struct{Writer io.Writer;Input,ObservedBytes []byte;Count int;Failure error}
type sourceWriter struct{program *Program;writer io.Writer}
func(w sourceWriter)Write(input []byte)(int,error){
 operation:=&sourceWriteOriginal{Writer:w.writer,Input:input,ObservedBytes:append([]byte(nil),input...)}
 occurrence:=w.program.sourceStart("output.write",w.writer,operation)
 operation.Count,operation.Failure=w.writer.Write(input);occurrence.Output=operation.Count;occurrence.Failure=operation.Failure;occurrence.ReturnedError=operation.Failure;occurrence.Returned=true
 return operation.Count,operation.Failure
}

type sourceMessage struct{occurrence *SourceOccurrence;message Msg;completed func(*SourceOccurrence)}
type sourceMessageOriginal struct{Owner any;Message Msg;Before,After Model;Command Cmd;BorrowError error;SendAttempted,SendReturned,Accepted,ConsumerStarted,ConsumerReturned bool;ContextError error}
// SendOwned links an existing producer to the genuine event-loop consumer while
// keeping the original message type intact for filter/control/renderer/model.
func(p *Program)SendOwned(message Msg,owner any,borrow func(*SourceOccurrence)error,completed func(*SourceOccurrence))error{
 original:=&sourceMessageOriginal{Owner:owner,Message:message}
 occurrence:=p.sourceStart("message.delivery",p,original);occurrence.Invocation=borrow;occurrence.Binding=completed
 if borrow!=nil{original.BorrowError=borrow(occurrence);if original.BorrowError!=nil{occurrence.Failure=original.BorrowError;return original.BorrowError}}
 original.SendAttempted=true
 select{case <-p.ctx.Done():original.ContextError=p.ctx.Err();case p.msgs<-sourceMessage{occurrence:occurrence,message:message,completed:completed}:original.Accepted=true};original.SendReturned=true;return nil
}
// WithMessageSourceObserver binds actual model update occurrences to this exact
// event-loop consumer. The hook observes source relations, not eligibility.
func WithMessageSourceObserver(observer func(*SourceOccurrence))ProgramOption{return func(p *Program){p.messageSourceObserver=observer}}

// Owned parser input keeps the same message object/type through the consumer.
// Nil owner retains the existing package fixture channel behavior.
func ownedInputCarrier(owner *Program,producer any,message Msg)Msg{
 if owner==nil{return message};original:=&sourceMessageOriginal{Owner:producer,Message:message,SendAttempted:true};occurrence:=owner.sourceStart("input.message-delivery",producer,original);return sourceMessage{occurrence:occurrence,message:message}
}
func finishInputSend(carrier Msg,accepted bool,err error){if owned,ok:=carrier.(sourceMessage);ok{original:=owned.occurrence.Input.(*sourceMessageOriginal);original.Accepted=accepted;original.ContextError=err;original.SendReturned=true}}
// Deferred directly by source call owners: normal completion is recorded only
// by the actual caller after invoke returns. Goexit retains unfinished state.
func retainSourceInterruption(occurrence *SourceOccurrence){if original:=recover();original!=nil{occurrence.Panic=original;occurrence.Stack=debug.Stack();panic(original)};if !occurrence.Returned{occurrence.Stack=debug.Stack()}}