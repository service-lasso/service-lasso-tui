// Package lifetime owns actual local originals and producer/consumer routes.
// Its observations are not native authority, disposal eligibility or archive ACK.
package lifetime

import("errors";"sync";"context")

var errDispatchStopped=errors.New("local original source dispatch is stopped")
var errOriginalEnrollment=errors.New("local original enrollment is inconsistent")

// Original is embedded in the actual source operation cell before its effects.
// Value preserves the actual object and its aliases, never an Error projection.
type Original struct{Source string;Value any;next *Original;owner *Receiver}
// Task is the actual source scheduler occurrence. Completion includes the real
// worker function AND its delivery callback, not merely context cancellation.
type Task struct{Original Original;DeliveryBorrow *Borrow;Parent *Task;Children []*Task;OnAbnormal func(*Task);NotificationAttempted,NotificationReturned bool;NotificationPanic any;Command any;Delivery any;Result any;Panic any;Started,Returned,DeliveryReturned,Consumed,AbnormalTermination bool;Failure error;Interruption error;done chan struct{};next *Task}

type Receiver struct{
 mutex sync.Mutex
 originals *Original
 tasks *Task
 workers sync.WaitGroup
 stopped bool
 joined bool
 terminalIO int
 inspections int
 // Original enrollment/dispatch failures stay owned without emitting raw data.
 enrollmentFailure error
 conflictingOriginal *Original
}
func NewReceiver()*Receiver{return &Receiver{}}
func(r *Receiver)Enroll(original *Original,source string,value any)error{
 if r==nil{return nil}
 r.mutex.Lock();defer r.mutex.Unlock()
 if original==nil||original.owner!=nil{r.enrollmentFailure=errOriginalEnrollment;r.conflictingOriginal=original;return errOriginalEnrollment}
 if r.joined{r.enrollmentFailure=errOriginalEnrollment;r.conflictingOriginal=original;return errOriginalEnrollment};original.Source=source;original.Value=value;original.owner=r;original.next=r.originals;r.originals=original;if r.stopped{return errDispatchStopped};return nil
}
// Dispatch enrolls the actual callback/result cell before creating its worker.
// Stop and Add share the same lock, so Join cannot race a later admitted Add.
func(r *Receiver)Dispatch(task *Task,source string,command func()any,deliver func(any))error{
 if r==nil||task==nil{return errOriginalEnrollment}
 task.Command=command;task.Delivery=deliver;task.done=make(chan struct{});if err:=r.Enroll(&task.Original,source,task);err!=nil{task.Failure=err;close(task.done);return err}
 if task.DeliveryBorrow!=nil{if err:=r.Borrow(task.DeliveryBorrow,&task.Original,source+".delivery",task);err!=nil{task.Failure=err;close(task.done);return err}}
 r.mutex.Lock();task.next=r.tasks;r.tasks=task;if task.Parent!=nil{if task.Parent.Original.owner!=r{task.Failure=errOriginalEnrollment;r.mutex.Unlock();close(task.done);return task.Failure};task.Parent.Children=append(task.Parent.Children,task)}
 if r.stopped{task.Failure=errDispatchStopped;r.mutex.Unlock();close(task.done);return task.Failure}
 r.workers.Add(1);r.mutex.Unlock()
 go func(){
  defer r.workers.Done();defer close(task.done)
  finished:=false
  defer func(){value:=recover();if !finished{r.mutex.Lock();task.Panic=value;task.AbnormalTermination=true;r.stopped=true;r.mutex.Unlock();r.notifyAbnormal(task)}}()
  r.mutex.Lock();if r.stopped{task.Failure=errDispatchStopped;r.mutex.Unlock();finished=true;return};task.Started=true;r.mutex.Unlock()
  result:=command()
  r.mutex.Lock();task.Result=result;task.Returned=true;r.mutex.Unlock()
  if deliver!=nil{deliver(result)}
  r.mutex.Lock();task.DeliveryReturned=true;r.mutex.Unlock();finished=true
 }()
 return nil
}
func(r *Receiver)Consumed(task *Task){if r==nil||task==nil{return};r.mutex.Lock();if task.Original.owner!=r{r.enrollmentFailure=errOriginalEnrollment;r.conflictingOriginal=&task.Original}else{task.Consumed=true};r.mutex.Unlock()}
func(r *Receiver)Stop(){if r==nil{return};r.mutex.Lock();r.stopped=true;r.mutex.Unlock()}
func(r *Receiver)Join(){if r==nil{return};r.Stop();r.workers.Wait();r.mutex.Lock();r.joined=true;r.mutex.Unlock()}
// Inspect visits SAME private objects only after actual producer joins. It is
// inspection, not a receipt, eligibility decision, release or graph transfer.
func(r *Receiver)Inspect(visit func(*Original))error{if r==nil||visit==nil{return errOriginalEnrollment};r.mutex.Lock();if !r.joined||r.terminalIO!=0{r.mutex.Unlock();return errOriginalEnrollment};head:=r.originals;r.inspections++;r.mutex.Unlock();defer func(){r.mutex.Lock();r.inspections--;r.mutex.Unlock()}();for original:=head;original!=nil;original=original.next{visit(original)};return nil}
func(r *Receiver)MarshalJSON()([]byte,error){return []byte("{}"),nil}

type Borrow struct{Original Original;Root *Original;Consumer any;Returned bool}
func(r *Receiver)Borrow(borrow *Borrow,root *Original,source string,consumer any)error{
 if r==nil||borrow==nil||root==nil{return errOriginalEnrollment}
 r.mutex.Lock();valid:=root.owner==r;if !valid{r.enrollmentFailure=errOriginalEnrollment;r.conflictingOriginal=root};r.mutex.Unlock();if !valid{return errOriginalEnrollment}
 borrow.Root=root;borrow.Consumer=consumer;return r.Enroll(&borrow.Original,source,borrow)
}
func(r *Receiver)ReturnBorrow(borrow *Borrow){if r==nil||borrow==nil{return};r.mutex.Lock();if borrow.Original.owner==r{borrow.Returned=true}else{r.enrollmentFailure=errOriginalEnrollment};r.mutex.Unlock()}
// Returning a borrow records the actual source consumer completion. It never
// clears the original object or authorizes native/original final disposition.
type receiverContextKey struct{}
func WithReceiver(ctx context.Context,receiver *Receiver)context.Context{return context.WithValue(ctx,receiverContextKey{},receiver)}
func FromContext(ctx context.Context)*Receiver{if ctx==nil{return nil};receiver,_:=ctx.Value(receiverContextKey{}).(*Receiver);return receiver}
// Require rejects the fixture-only nil receiver before governed entry effects.
func Require(receiver *Receiver)error{if receiver==nil{return errOriginalEnrollment};return nil}

// WaitTask joins this actual worker/delivery occurrence without stopping siblings.
func(r *Receiver)WaitTask(task *Task)error{if r==nil||task==nil{return errOriginalEnrollment};r.mutex.Lock();if task.Original.owner!=r{r.mutex.Unlock();return errOriginalEnrollment};done:=task.done;r.mutex.Unlock();if done==nil{return errOriginalEnrollment};<-done;return nil}

func(r *Receiver)notifyAbnormal(task *Task){
 if task.OnAbnormal==nil{return}
 r.mutex.Lock();task.NotificationAttempted=true;r.mutex.Unlock()
 returned:=false;defer func(){secondary:=recover();r.mutex.Lock();task.NotificationReturned=returned;if !returned{task.NotificationPanic=secondary};r.mutex.Unlock()}()
 task.OnAbnormal(task);returned=true
}
// BindBorrow attaches a genuine consumer to a preowned alias cell, including
// terminal delivery after Stop. It admits no new source or worker.
func(r *Receiver)BindBorrow(borrow *Borrow,consumer any)error{if r==nil||borrow==nil{return errOriginalEnrollment};r.mutex.Lock();defer r.mutex.Unlock();if borrow.Original.owner!=r||borrow.Returned{return errOriginalEnrollment};borrow.Consumer=consumer;return nil}
