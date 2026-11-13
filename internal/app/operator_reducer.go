package app

import (
 "context"
 "bytes"
 "errors"
 "time"

 tea "github.com/charmbracelet/bubbletea"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

var operatorViews=[]string{"overview","detail","graph","variables","endpoints","lifecycle","metadata","config","setup","declared_actions","updates","health_history","history","inbox","queue","audit","logs","removal"}
func operatorJourneyNeedsService(journey string)bool{return journey!="overview"&&journey!="graph"&&journey!="inbox"&&journey!="queue"&&journey!="audit"}
func(m operatorModel)Update(message tea.Msg)(tea.Model,tea.Cmd){
 // Retain the actual original before ticket/context admission can reject it.
 // The record never grants publication, settlement, replay or cancellation.
 var receivedErr error
 switch msg:=message.(type){case operatorBooted:receivedErr=msg.Err;if receivedErr==nil{receivedErr=msg.Restore.Warning};case operatorLoaded:receivedErr=msg.Err;switch data:=msg.Data.(type){case operatorMetadataRead:receivedErr=errors.Join(receivedErr,data.Observation.Err);case operatorOverviewRead:for _,observation:=range data.Metadata{receivedErr=errors.Join(receivedErr,observation.Err)};case operatorGraphRead:for _,observation:=range data.Metadata{receivedErr=errors.Join(receivedErr,observation.Err)}};case operatorPreviewed:receivedErr=msg.Err;case operatorDraftBinding:receivedErr=msg.Err;case operatorSubmitted:receivedErr=msg.Err;case operatorCancelled:receivedErr=msg.Err;case operatorDirectResult:receivedErr=msg.Err;case operatorPoll:receivedErr=msg.Err;case operatorTerminalRefreshed:receivedErr=msg.Err}
 if receivedErr!=nil{m.receivedFailures=append(m.receivedFailures,operatorReceivedFailure{Message:message,Err:receivedErr})}
 switch msg:=message.(type){
 case tea.WindowSizeMsg:m.width=msg.Width;m.height=msg.Height;if m.width<0{m.width=0};if m.height<0{m.height=0};if m.draft.SetupReview{m.draft.SetupOffset=0};if m.declaredReadReview{m.declaredReadOffset=0}
 case operatorBooted:
  // Native custody survives browsing/connection epochs and context failures.
  if _,ok:=recordOutcome(msg.Restore.Warning);ok{m.storageWarning(msg.Restore.Warning)}
  if _,already:=recordOutcome(msg.Restore.Warning);!already{if _,ok:=recordOutcome(msg.Err);ok{m.storageWarning(msg.Err)}}
  if msg.Epoch==m.epoch{m.bootInFlight=false}
  if msg.Epoch!=m.epoch{return m,nil};if msg.Err!=nil{m.saved=append([]byte(nil),msg.Restore.Raw...);m.errorText=api.SafeError(msg.Err);return m,nil}
  m.discovery=&msg.Discovery;m.binding=msg.Context
  restored:=msg.Restore;m.saved=append([]byte(nil),restored.Raw...);m.legacyRecord=restored.Legacy;m.restorationBlocked=restored.Warning!=nil
  if restored.Record.Context==m.binding{m.operation=restored.Readback}else if restored.Readback!=nil{restored.Warning=&api.OperatorError{Stage:"context"}}
  if restored.Warning!=nil{m.restorationBlocked=true;m.warning=api.SafeError(restored.Warning)}
  if !m.custodyBlocked&&restored.Migration!=nil&&restored.Record.Context==m.binding{
   if err:=m.store.replaceRaw(restored.Raw,restored.Migration);err!=nil{m.storageWarning(err)}else{m.saved=append([]byte(nil),restored.Migration...);m.legacyRecord=false}
  }
  cmd:=m.load(nil);refresh:=m.refreshTerminalState(false);next:=m.nextPollTick();return m,tea.Batch(cmd,refresh,next)
 case operatorLoaded:
  if !m.current(msg.Ticket){return m,nil};m.loading=false
  if msg.Err!=nil{if m.journey=="overview"||m.journey=="graph"||m.journey=="metadata"{m.metadataCurrent=nil};m.errorText=api.SafeError(msg.Err);m.markPageStale();m.markFormReadFailed();return m,nil}
  if err:=m.applyLoaded(msg);err!=nil{m.receivedFailures=append(m.receivedFailures,operatorReceivedFailure{Message:msg,Err:err});if m.journey=="overview"||m.journey=="graph"||m.journey=="metadata"{m.metadataCurrent=nil};m.errorText=api.SafeError(err);m.markPageStale();m.markFormReadFailed();return m,nil};m.acceptPageSelection(msg.Ticket);m.errorText="";m.clampJourneyPosition()
 case operatorPreviewed:
  if !m.current(msg.Ticket){return m,nil};m.loading=false;m.fieldErrors=nil;if msg.Err!=nil{m.errorText=api.SafeError(msg.Err);public,ok:=msg.Err.(*api.OperatorPublicError);if ok{m.fieldErrors=append([]api.OperatorFieldError(nil),public.FieldErrors...);if m.draft.Payload!=nil&&len(m.fieldErrors)>0{for i,focus:=range m.draft.Payload.focuses(){if focus.Path==m.fieldErrors[0].Selector{m.draft.PayloadIndex=i;m.showPayloadFocus();break}}}};return m,nil}
  m.draft.Preview=&msg.Preview;m.setEditorText("");m.errorText=""
 case operatorDraftBinding:
  if !m.current(msg.Ticket)||m.draft.Action!=msg.Action{return m,nil};m.loading=false;if !m.draftSelectionCurrent()||msg.Availability.ServiceID!=""&&msg.Availability.ServiceID!=m.draft.Service{m.errorText=api.SafeError(&api.OperatorError{Stage:"context"});return m,nil};if msg.Err!=nil{m.errorText=api.SafeError(msg.Err);return m,nil}
  if !msg.Availability.Availability.Available{m.errorText="Current lifecycle mutation binding is unavailable.";return m,nil};for _,binding:=range msg.Availability.MutationBindings{if binding.ActionID==msg.Action&&binding.Availability.Available&&binding.ExpectedRevision!=nil{m.draft.Definition=*msg.Availability.DefinitionRevision;m.draft.Expected=*binding.ExpectedRevision;m.lifecycle=&msg.Availability;return m,nil}};m.errorText="Current action has no mutation revision grant."
 case operatorSubmitted:
  if msg.Epoch!=m.epoch||!m.draft.Sent||m.draft.MutationTicket==nil||*m.draft.MutationTicket!=msg.Ticket||m.draft.Context!=msg.Binding{return m,nil};if m.current(msg.Ticket){m.loading=false}
  if msg.Err!=nil{var failure *api.OperatorSubmissionFailure;if errors.As(msg.Err,&failure)&&failure.Dispatched{m.warning="Submission outcome is UNKNOWN. No operation ID was admitted; do not replay or infer it from a list."}else{m.draft.Sent=false;m.draft.Retired=!m.current(msg.Ticket);m.errorText=api.SafeError(msg.Err)};return m,nil}
  if msg.Binding!=m.binding||m.draft.Preview==nil||msg.Submission.Operation.ServiceID!=m.draft.Preview.ServiceID||msg.Submission.Operation.ActionID!=m.draft.Preview.ActionID{m.warning="Submission requires reconciliation under its original trusted context.";return m,nil}
  op:=msg.Submission.Operation;m.terminalRefreshID="";m.terminalRefreshWarning="";m.terminalDetail=nil;m.terminalRefreshInFlight=false;m.terminalRefreshSequence++;m.pollRequest++;m.pollInFlight=false;m.cancelInFlight=false;m.cancelAttempted=false;m.cancelSequence++;m.operation=&api.OperatorReadback{Operation:op,Result:msg.Submission.Result}
  raw,err:=persistOperator(m.store,msg.Expected,op,msg.Binding);if err!=nil{m.storageWarning(err)}else{m.saved=raw;m.legacyRecord=false};m.draft=operatorDraft{};refresh:=m.refreshTerminalState(false);next:=m.nextPollTick();return m,tea.Batch(refresh,next)
 case operatorTerminalRefreshed:
  if msg.Err!=nil{m.terminalRefreshFailures=append(m.terminalRefreshFailures,operatorTerminalRefreshFailure{msg.Epoch,msg.Sequence,msg.Binding,msg.Original,msg.Err})}
  if msg.Epoch!=m.epoch||!m.terminalRefreshInFlight||msg.Sequence!=m.terminalRefreshSequence||m.operation==nil||!m.operation.Operation.CanClear()||!msg.Original.SameRoot(m.operation.Operation){return m,nil}
  m.terminalRefreshInFlight=false
  if msg.Binding!=m.binding{m.terminalRefreshWarning=api.SafeError(&api.OperatorError{Stage:"context"});return m,nil}
  if msg.Err!=nil{m.terminalRefreshWarning=api.SafeError(msg.Err);return m,nil}
  if !msg.Detail.Valid()||msg.Detail.Summary.ServiceID!=msg.Original.ServiceID{m.terminalRefreshWarning=api.SafeError(&api.OperatorError{Stage:"response"});return m,nil}
  m.terminalRefreshWarning="";m.terminalDetail=&msg.Detail
  // No replacement of a snapshot row by an unrelated Detail observation.
  if m.current(msg.Ticket)&&m.journey=="detail"&&m.service==msg.Original.ServiceID{m.detail=&msg.Detail;m.acceptPageSelection(msg.Ticket)}
  return m,nil
 case operatorCancelled:
  if msg.Epoch!=m.epoch||m.operation==nil||m.operation.Operation.OperationID!=msg.ID||!m.cancelInFlight||msg.Sequence!=m.cancelSequence{return m,nil};if msg.Binding!=m.binding{m.warning=api.SafeError(&api.OperatorError{Stage:"context"});next:=m.nextPollTick();return m,next};m.cancelInFlight=false;m.warning="";if msg.Dispatched{m.cancelAttempted=true};if msg.Err!=nil{if msg.Dispatched{m.warning="Cancellation response is UNKNOWN. Retain the original operation; do not repeat cancellation. Read authoritative operation state."}else{m.errorText=api.SafeError(msg.Err)}}else{m.warning="Cancellation "+msg.Response.Status+"; authoritative operation readback determines the outcome."};next:=m.nextPollTick();return m,next
 case operatorDirectResult:
  if !m.draft.Sent||m.draft.MutationTicket==nil||*m.draft.MutationTicket!=msg.Ticket||msg.Binding!=m.draft.Context||msg.Ticket.Connection!=m.epoch{return m,nil};if m.current(msg.Ticket){m.loading=false}
  if msg.Err!=nil{if msg.Dispatched{m.warning="Mutation response is UNKNOWN. Refresh authoritative state; do not repeat the mutation."}else{m.draft.Sent=false;m.draft.Review=false;m.draft.Retired=!m.current(msg.Ticket);m.errorText=api.SafeError(msg.Err)};return m,nil}
  m.draft=operatorDraft{};m.invalidate();cmd:=m.load(nil);return m,cmd
 case operatorTick:
  if msg.Epoch!=m.epoch||!m.pollScheduled||msg.Sequence!=m.pollTimer{return m,nil};m.pollScheduled=false
  m.expirePages(time.Now())
  if m.operation==nil{next:=m.nextPollTick();return m,next};m.pollRequest++;m.pollInFlight=true;sequence:=m.pollRequest;epoch,original,client,ctx,binding:=m.epoch,m.operation.Operation,m.client,m.ctx,m.binding
  return m,func()tea.Msg{readback,err:=readOperatorOwn(ctx,client,binding,original);return operatorPoll{Epoch:epoch,ID:original.OperationID,Readback:readback,Err:err,Binding:binding,Sequence:sequence}}
 case operatorPoll:
  if msg.Epoch!=m.epoch||!m.pollInFlight||msg.Sequence!=m.pollRequest{return m,nil};m.pollInFlight=false
  if m.operation==nil||m.operation.Operation.OperationID!=msg.ID{next:=m.nextPollTick();return m,next}
  if msg.Err!=nil{m.warning=api.SafeError(msg.Err);next:=m.nextPollTick();return m,next}
  if msg.Binding!=m.binding{m.warning=api.SafeError(&api.OperatorError{Stage:"context"});next:=m.nextPollTick();return m,next}
  if !msg.Readback.Operation.SameRoot(m.operation.Operation)||msg.Readback.Operation.UpdatedAt<m.operation.Operation.UpdatedAt||m.operation.Operation.CanClear()&&msg.Readback.Operation.Outcome==nil{m.warning=api.SafeError(&api.OperatorError{Stage:"response"});next:=m.nextPollTick();return m,next}
  // Every read replaces the authoritative targets, including a shrink to [];
  // polling never replays an action or guesses an ID from an operation list.
  m.operation=&msg.Readback;refresh:=m.refreshTerminalState(false);next:=m.nextPollTick();return m,tea.Batch(refresh,next)
 case tea.KeyMsg:
  if m.consumeQuotedPayloadKey(msg){return m,nil}
  if msg.String()=="ctrl+c"{m.invalidate();return m,tea.Quit}
   if m.logSearchDraft!=nil{cmd:=m.logSearchKey(msg);return m,cmd}
  // A dispatched request without an admitted response retains its original
  // draft as read-only custody while ordinary read navigation stays usable.
  if !m.draft.Sent&&!m.draft.Retired&&(m.draft.Dirty||m.draft.Preview!=nil){cmd:=m.editKey(msg);return m,cmd}
  if m.declaredReadReview{
   if !m.declaredInspectionCurrent(){m.declaredReadReview=false;return m,nil}
   switch msg.String(){case "alt+pgdown":m.moveDeclaredInspection(true);case "alt+pgup":m.moveDeclaredInspection(false);case "down":m.declaredReadOffset++;if rows:=m.declaredInspectionRows();m.declaredReadOffset>=len(rows){m.declaredReadOffset=len(rows)-1};case "up":if m.declaredReadOffset>0{m.declaredReadOffset--};case "alt+home","esc":m.declaredReadReview=false;case "tab","shift+tab":m.declaredReadReview=false;m.toggleFocus();case "left","right":cmd:=m.changeView(msg.String()=="right");return m,cmd;case "r":m.invalidate();cmd:=m.load(nil);refresh:=m.refreshTerminalState(true);return m,tea.Batch(cmd,refresh);case "q":m.invalidate();return m,tea.Quit};return m,nil
  }
  switch msg.String(){
  case "/":if m.journey=="logs"&&m.focus==1&&!m.loading{m.logSearchDraft=&operatorDraft{Selector:"Search logs",Text:m.logSearch,Cursor:len(m.logSearch)}}
   case "alt+pgdown":m.moveDeclaredInspection(true)
  case "alt+pgup":m.moveDeclaredInspection(false)
  case "alt+c":
   if m.bootInFlight{m.warning="Connection is loading. Wait for it before switching.";return m,nil}
   if m.draft.Sent||m.custodyBlocked{m.warning="The original request and storage custody are retained. Read-only navigation is available; connection changes require reconciliation.";return m,nil}
   if m.connections==nil{return m,nil};names:=m.connections.Names();if len(names)<2{return m,nil};next:=0;for i,name:=range names{if name==m.connection{next=(i+1)%len(names)}};client,err:=m.connections.Switch(names[next]);if err!=nil{m.errorText=api.SafeError(err);return m,nil}
   m.invalidate();m.epoch++;m.terminalRefreshID="";m.terminalRefreshWarning="";m.terminalDetail=nil;m.terminalRefreshInFlight=false;m.terminalRefreshSequence++;m.pollScheduled=false;m.pollInFlight=false;m.pollRequest++;m.cancelInFlight=false;m.cancelAttempted=false;m.cancelSequence++;m.bootInFlight=true;m.client=client;m.connection=names[next];m.discovery=nil;m.binding=api.OperatorContext{};m.service="";m.journey="overview";m.operation=nil;m.legacyRecord=false;m.services=operatorPageState[api.OperatorService]{};m.graph=operatorGraphState{};m.metadataCurrent=nil;cmd:=m.bootstrap();return m,cmd
  case "q":m.invalidate();return m,tea.Quit
  case "tab":m.toggleFocus()
  case "shift+tab":m.toggleFocus()
  case "up","k":if m.row>0{m.row--};if m.row<m.scroll{m.scroll=m.row}
  case "down","j":if m.row+1<m.rowCount(){m.row++};visible:=m.height-8;if visible<1{visible=1};if m.row>=m.scroll+visible{m.scroll=m.row-visible+1}
  case "pgup":distance:=m.height-8;if distance<1{distance=1};m.scroll-=distance;if m.scroll<0{m.scroll=0};m.row=m.scroll
  case "pgdown":distance:=m.height-8;if distance<1{distance=1};m.scroll+=distance;if m.scroll>=m.rowCount(){m.scroll=m.rowCount()-1};if m.scroll<0{m.scroll=0};m.row=m.scroll
  case "home":m.row=0;m.scroll=0
  case "end":m.row=m.rowCount()-1;if m.row<0{m.row=0};m.scroll=m.row
  case "left","right":
   cmd:=m.changeView(msg.String()=="right");return m,cmd
  case "enter":if m.focus==0{if m.row>=0&&m.row<len(operatorViews)&&(m.service!=""||!operatorJourneyNeedsService(operatorViews[m.row])){journey:=operatorViews[m.row];m.invalidate();m.journey=journey;m.focus=1;m.restoreJourneyPosition();cmd:=m.load(nil);return m,cmd};return m,nil};if m.journey=="overview"&&m.pageSelectionCurrent()&&m.row<len(m.overviewServices()){service:=m.overviewServices()[m.row].ServiceID;m.rememberJourneyPosition();m.invalidate();m.service=service;m.journey="detail";m.restoreJourneyPosition();cmd:=m.load(nil);return m,cmd};cmd:=m.beginEdit();return m,cmd
  case "r":m.invalidate();cmd:=m.load(nil);refresh:=m.refreshTerminalState(true);return m,tea.Batch(cmd,refresh)
  case "g":m.invalidate();m.service="";if operatorJourneyNeedsService(m.journey){m.journey="overview"};cmd:=m.load(nil);return m,cmd
  case "F":if m.journey=="overview"&&m.focus==1{m.favoritesOnly=!m.favoritesOnly;m.row=0;m.scroll=0}
   case "f":if m.cycleFilter(){cmd:=m.load(nil);return m,cmd}
  case "n":if !m.loading{if cursor:=m.nextCursor();cursor!=nil{cmd:=m.load(cursor);return m,cmd}}
  case "e":if m.focus==1{cmd:=m.beginEdit();return m,cmd}
  case "c":cmd:=m.cancelOperation();return m,cmd
  case "x":if !m.mutationBlocked()&&m.operation!=nil&&m.operation.Operation.CanClear()&&!m.legacyRecord{if err:=clearOperator(m.store,m.saved,m.operation.Operation);err!=nil{m.storageWarning(err)}else{m.saved=nil;m.operation=nil;m.terminalRefreshSequence++;m.terminalRefreshInFlight=false;m.terminalRefreshID="";m.terminalRefreshWarning="";m.terminalDetail=nil /* The original read closure finishes normally; its retired reply cannot publish after deliberate clear. */}}
  }
 }
 return m,nil
}
func(m *operatorModel)markPageStale(){switch m.journey{case "overview":m.services.Stale=true;case "graph":m.graph.Stale=true;case "variables":m.variables.Stale=true;case "endpoints":m.endpoints.Stale=true;case "health_history":m.health.Stale=true;case "history":m.history.Stale=true;case "inbox":m.inbox.Stale=true;case "queue":m.queue.Stale=true;case "audit":m.audit.Stale=true;case "logs":m.logs.Stale=true}}
func(m *operatorModel)expirePages(now time.Time){m.services.expire(now);m.variables.expire(now);m.endpoints.expire(now);m.health.expire(now);m.history.expire(now);m.inbox.expire(now);m.queue.expire(now);m.audit.expire(now);m.logs.expire(now);if operatorSnapshotExpired(m.graph.Expiry,now){m.graph.Stale=true}}
func(m operatorModel)nextCursor()*string{if !m.pageSelectionCurrent()||m.pageSelectionExpired(time.Now()){return nil};switch m.journey{case "overview":if !m.services.Stale{return m.services.Next};case "graph":if !m.graph.Stale{return m.graph.Next};case "variables":if !m.variables.Stale{return m.variables.Next};case "endpoints":if !m.endpoints.Stale{return m.endpoints.Next};case "health_history":if !m.health.Stale{return m.health.Next};case "history":if !m.history.Stale{return m.history.Next};case "inbox":if !m.inbox.Stale{return m.inbox.Next};case "queue":if !m.queue.Stale{return m.queue.Next};case "audit":if !m.audit.Stale{return m.audit.Next};case "logs":if !m.logs.Stale{return m.logs.Next}};return nil}
// Context cancellation accompanies ticket invalidation, but ticket comparison
// is still required because an already received response can outlive cancel.
func(m *operatorModel)requestContext()context.Context{if m.cancel!=nil{m.cancel()};ctx,cancel:=context.WithCancel(m.ctx);m.cancel=cancel;return ctx}
func(m *operatorModel)storageWarning(err error){
 m.custodyBlocked=true
 m.custodyErrors=append(m.custodyErrors,err)
 if outcome,ok:=recordOutcome(err);ok{m.custodyOutcome=&outcome;m.custodyWarning=recordJoinedErrors(err).Error();if outcome.OriginalKnown&&bytes.Equal(m.saved,outcome.Original){switch outcome.PublicationEffect{case recordReplacementApplied:record,decodeErr:=api.DecodeOperatorSavedV3(outcome.Replacement);if decodeErr!=nil{m.custodyErrors=append(m.custodyErrors,decodeErr)};if decodeErr==nil&&record.Context==m.binding&&m.operation!=nil&&record.OperationID==m.operation.Operation.OperationID&&record.ServiceID==m.operation.Operation.ServiceID&&record.ActionID==m.operation.Operation.ActionID{m.saved=append([]byte(nil),outcome.Replacement...);m.legacyRecord=false};case recordClearApplied:m.saved=nil}};return}
 var unresolved *recordPublicationUnresolved;if errors.As(err,&unresolved){m.custodyWarning=unresolved.Error()}else{m.custodyWarning="Local operation record could not be saved or cleared. Retained record requires reconciliation."}
}
