package app

import ("sync";"reflect";"errors"
 "github.com/service-lasso/service-lasso-tui/internal/lifetime"
 tea "github.com/charmbracelet/bubbletea"
)

// ownedProgram routes application commands through actual receiver workers.
// BubbleTea still owns its input/render workers; their completion and native
// custody remain independent obligations, never implied by this worker join.
type ownedProgram struct {
 original lifetime.Original
 receiver *lifetime.Receiver
 model tea.Model
 program *tea.Program
 activeSource *tea.SourceOccurrence
 failure error
 mutex sync.Mutex
}
type ownedDelivery struct{ task *lifetime.Task; message tea.Msg }
type modelOccurrence struct {
 original lifetime.Original
 before tea.Model
 message tea.Msg
 after tea.Model
 command tea.Cmd
 borrow lifetime.Borrow
 librarySource *tea.SourceOccurrence
}

func(m *ownedProgram)recordFailure(err error){m.mutex.Lock();if m.failure==nil{m.failure=err};m.mutex.Unlock()}
type commandInterrupted struct{Task *lifetime.Task}
func(e *commandInterrupted)Error()string{return "application command producer did not complete"}
func(e *commandInterrupted)Unwrap()error{cause,_:=e.Task.Panic.(error);return cause}
func(m *ownedProgram)commandFailed(task *lifetime.Task){m.recordFailure(task.Interruption);m.program.Kill()}
func(m *ownedProgram)dispatch(command tea.Cmd)*lifetime.Task{return m.dispatchFrom(command,nil)}
func (m *ownedProgram) dispatchFrom(command tea.Cmd,parent *lifetime.Task) *lifetime.Task {
 if command==nil{return nil}
 task:=&lifetime.Task{Parent:parent,OnAbnormal:m.commandFailed,DeliveryBorrow:&lifetime.Borrow{}};task.Interruption=&commandInterrupted{Task:task}
 if err:=m.receiver.Dispatch(task,"app.command",func()any{return command()},func(result any){
  message,ok:=result.(tea.Msg);if !ok||message==nil{return}
  // The shared receiver remains open through actual Program shutdown terminal
  // operations. RunOwned stops/joins after Run returns; Quit is no join.
  if batch,ok:=message.(tea.BatchMsg);ok{workers:=make([]*lifetime.Task,0,len(batch));for _,child:=range batch{if worker:=m.dispatchFrom(child,task);worker!=nil{workers=append(workers,worker)}};for _,worker:=range workers{if err:=m.receiver.WaitTask(worker);err!=nil{m.recordFailure(err)}};return}
  if reflect.TypeOf(message).PkgPath()=="github.com/charmbracelet/bubbletea"&&reflect.TypeOf(message).Name()=="sequenceMsg"{sequence:=reflect.ValueOf(message);for index:=0;index<sequence.Len();index++{child:=sequence.Index(index).Interface().(tea.Cmd);if worker:=m.dispatchFrom(child,task);worker!=nil{if err:=m.receiver.WaitTask(worker);err!=nil{m.recordFailure(err);return}}};return}
  if err:=m.program.SendOwned(message,task,func(source *tea.SourceOccurrence)error{return m.receiver.BindBorrow(task.DeliveryBorrow,source)},func(*tea.SourceOccurrence){m.receiver.ReturnBorrow(task.DeliveryBorrow);m.receiver.Consumed(task)});err!=nil{m.recordFailure(err)} });err!=nil{m.recordFailure(err)}
 return task
}
func(m *ownedProgram)Init()tea.Cmd{
 occurrence:=&modelOccurrence{before:m.model}
 if err:=m.receiver.Enroll(&occurrence.original,"app.init",occurrence);err!=nil{m.recordFailure(err);return nil}
 occurrence.command=m.model.Init();m.dispatch(occurrence.command);return nil
}
func(m *ownedProgram)Update(message tea.Msg)(tea.Model,tea.Cmd){
 occurrence:=&modelOccurrence{before:m.model,message:message,librarySource:m.activeSource}
 if err:=m.receiver.Enroll(&occurrence.original,"app.update",occurrence);err!=nil{m.recordFailure(err);return m,nil}
 occurrence.after,occurrence.command=m.model.Update(message)
 m.model=occurrence.after;m.dispatch(occurrence.command);return m,nil
}
func(m *ownedProgram)View()string{return m.model.View()}

// RunOwned keeps the actual final wrapper, model and Run failure in the caller's
// original occurrence. Return proves only actual application command joins.
// It does not authorize local disposition, native closure or public success.
func RunOwned(model tea.Model,receiver *lifetime.Receiver,options ...tea.ProgramOption)(tea.Model,error){
 if err:=lifetime.Require(receiver);err!=nil{return model,err}
 owner:=&ownedProgram{receiver:receiver,model:model}
 if err:=receiver.Enroll(&owner.original,"app.program",owner);err!=nil{return owner,err}
 ownedOptions:=make([]tea.ProgramOption,0,len(options)+2);ownedOptions=append(ownedOptions,tea.WithProgramOwner(func(program *tea.Program){owner.program=program}));ownedOptions=append(ownedOptions,tea.WithMessageSourceObserver(func(source *tea.SourceOccurrence){owner.activeSource=source}));ownedOptions=append(ownedOptions,options...)
 owner.program=tea.NewProgram(owner,ownedOptions...)
 final,err:=owner.program.Run()
 receiver.Stop();receiver.Join()
 return final,errors.Join(err,owner.failure)
}
