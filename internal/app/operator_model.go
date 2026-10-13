package app

import (
 "context"
 "errors"
 "time"

 tea "github.com/charmbracelet/bubbletea"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// Tickets bind every response to its connection, selection and page request.
// Server generation/model epochs are checked separately before mutation.
type operatorTicket struct{ Connection,Selection,Request uint64; Service,Journey,Filter,Search string; Binding api.OperatorContext `json:"-"` }
type operatorLoaded struct{ Ticket operatorTicket `json:"-"`; Cursor *string `json:"-"`; Data any `json:"-"`; Err error `json:"-"` }
type operatorBooted struct{ Epoch uint64; Discovery api.OperatorDiscovery; Context api.OperatorContext `json:"-"`; Restore operatorRestoration `json:"-"`; Err error `json:"-"` }
type operatorPoll struct{ Epoch uint64; ID string; Readback api.OperatorReadback; Err error `json:"-"`; Binding api.OperatorContext `json:"-"`; Sequence uint64 }
type operatorTick struct{ Epoch,Sequence uint64 }
// Original received messages remain private even when their ticket is retired.
// This process-local retention is not durable capture or finite lifetime proof.
type operatorReceivedFailure struct{Message any `json:"-"`;Err error `json:"-"`}
type operatorDraft struct{
 ReadConnection,ReadSelection uint64
 Service,ReadService,ReadJourney,ReadFilter string
 MetadataGeneration string
 ConfigModelEpoch string
 Input []byte
 Selector,Text,Action,Definition,Expected,StepID,DeclaredID,Idempotency string
 Preview *api.OperatorPreview
 Context api.OperatorContext
 MutationTicket *operatorTicket
 Payload *operatorPayloadNodeDraft
 PayloadIndex int
 InlinePayload bool
 PayloadPolicy api.OperatorPayloadPolicy
 PayloadReferences []api.OperatorPayloadReference
 ReferenceIndex int
 PreviewOffset int
 PreviewReview bool
 SetupReview bool
 SetupOffset int
 ChoiceOffset int
 ChoiceReview bool
 LiteralNext bool
 Dirty,Sent,Retired,Review,Force,IncludeManual bool
 Fields []api.OperatorField
 Values map[string][]byte
 FieldIndex int
 Cursor int
 ReferenceMode bool
 PayloadRef *string
 ConfigOperations []api.OperatorConfigOperation
 QueueRow *api.OperatorQueueRow
 InboxRow *api.OperatorInboxRow
 Catalog *api.OperatorCatalog
 EditEntry *api.OperatorCatalogEntry
 SlotIndex,TargetIndex,SourceIndex int
 RootMode string
 RootHidden,RootProtected bool
}
// Drafts are private editor state, not a wire DTO. This closes serialization
// for input bytes, values, context, references and mutation credentials together.
func(operatorDraft)MarshalJSON()([]byte,error){return []byte("{}"),nil}
type operatorModel struct{
 client *api.Client
 ctx context.Context
 connection string
 connections connectionManager
 store fileOperationStore
 epoch,selection,request uint64
 pollTimer,pollRequest uint64
 pollScheduled,pollInFlight bool
 cancelSequence uint64
 cancelInFlight,cancelAttempted bool
 terminalRefreshID,terminalRefreshWarning string
 terminalRefreshSequence uint64
 terminalRefreshInFlight bool
 terminalDetail *api.OperatorDetail
 terminalRefreshFailures []operatorTerminalRefreshFailure
 editorFailures []operatorEditorFailure
 receivedFailures []operatorReceivedFailure
 cancel context.CancelFunc
 discovery *api.OperatorDiscovery
 binding api.OperatorContext
 service,journey string
 filters map[string]string
 logSearch string
 logSearchDraft *operatorDraft
 journeyPositions map[string]operatorJourneyPosition
 pageSelections map[string]operatorPageSelection
 failedFormSelections map[string]operatorPageSelection
 declaredReadReview bool
 declaredReadOffset int
 declaredReadAction string
 declaredReadBinding operatorPageSelection
 width,height,focus,row,scroll int
 loading bool
 errorText,warning,custodyWarning string
 custodyBlocked bool
 restorationBlocked bool
 custodyOutcome *recordStorageOutcome
 custodyErrors []error
 bootInFlight bool
 fieldErrors []api.OperatorFieldError
 draft operatorDraft
 services operatorPageState[api.OperatorService]
 graph operatorGraphState
 variables operatorPageState[api.OperatorVariable]
 endpoints operatorPageState[api.OperatorEndpoint]
 health operatorPageState[api.OperatorHistoryRow]
 history operatorPageState[api.OperatorHistoryRow]
 inbox operatorPageState[api.OperatorInboxRow]
 queue operatorPageState[api.OperatorQueueRow]
 audit operatorPageState[api.OperatorAuditRow]
 logs operatorPageState[api.OperatorLogRow]
 detail *api.OperatorDetail
 metadata map[operatorMetaKey]operatorMetaObservation
 metadataCurrent map[string]operatorMetaObservation
 favoritesOnly bool
 meta *api.OperatorMeta
 lifecycle *api.OperatorLifecycleAvailability
 config *api.OperatorConfigForm
 setup *api.OperatorSetupForm
 actions *api.OperatorActionForm
 updates *api.OperatorUpdateForm
 removal *api.OperatorRemovalForm
 operation *api.OperatorReadback
 saved []byte
 legacyRecord bool
}
func NewOperator(client *api.Client,ctx context.Context,connection string)tea.Model{
 store,_:=defaultOperationStore().(fileOperationStore)
 return operatorModel{client:client,ctx:ctx,connection:connection,store:store,epoch:1,bootInFlight:true,journey:"overview",focus:1,filters:map[string]string{},width:80,height:24}
}
func NewOperatorWithConnections(client *api.Client,connections connectionManager,ctx context.Context)tea.Model{
 if connections==nil{return NewOperator(client,ctx,"")}
 model:=NewOperator(client,ctx,connections.Current()).(operatorModel);model.connections=connections;return model
}
func(m operatorModel)Init()tea.Cmd{return m.bootstrap()}
// Retained bytes are not authority to replace an unreadable, foreign-context,
// missing or legacy own record. Read journeys remain useful while mutation
// admission waits for exact own-ID restoration and validated migration.
func(m operatorModel)mutationBlocked()bool{return m.custodyBlocked||m.restorationBlocked||len(m.saved)>0&&(m.legacyRecord||m.operation==nil)}
func(m operatorModel)bootstrap()tea.Cmd{
 epoch,client,ctx,connection,store:=m.epoch,m.client,m.ctx,m.connection,m.store
 return func()tea.Msg{
  d,err:=client.OperatorDiscovery(ctx);if err!=nil{return operatorBooted{Epoch:epoch,Err:err}}
  binding,err:=client.OperatorContext(ctx);if err!=nil{return operatorBooted{Epoch:epoch,Err:err}}
  restored,restoreErr:=restoreOperator(ctx,client,connection,store)
  if restoreErr!=nil{restored.Warning=restoreErr}
  after,err:=client.OperatorContext(ctx);if err!=nil||after!=binding{return operatorBooted{Epoch:epoch,Restore:restored,Err:errors.Join(&api.OperatorError{Stage:"context"},err,restored.Warning)}}
  if restored.Readback!=nil&&restored.Record.Context!=binding{restored.Migration=nil;restored.Warning=errors.Join(restored.Warning,&api.OperatorError{Stage:"context"})}
  return operatorBooted{Epoch:epoch,Discovery:d,Context:binding,Restore:restored}
 }
}
func(m *operatorModel)invalidate(){m.metadataCurrent=nil;m.logSearchDraft=nil;m.declaredReadReview=false;m.declaredReadOffset=0;if m.cancel!=nil{m.cancel();m.cancel=nil};m.selection++;m.request++;m.loading=false;m.row=0;m.scroll=0;m.errorText="";m.fieldErrors=nil;m.detail=nil;m.meta=nil;m.lifecycle=nil;m.config=nil;m.setup=nil;m.actions=nil;m.updates=nil;m.removal=nil}
func(m operatorModel)ticket()operatorTicket{return operatorTicket{m.epoch,m.selection,m.request,m.service,m.journey,m.filter(),m.readSearch(),m.binding}}
func(m operatorModel)current(t operatorTicket)bool{return t==m.ticket()}
func(m *operatorModel)load(cursor *string)tea.Cmd{
 m.declaredReadReview=false;m.declaredReadOffset=0
 if m.discovery==nil{return nil};if allowed,code:=operatorReadAdmission(m.discovery,m.journey);!allowed{m.errorText=api.OperatorMessage(code);m.loading=false;return nil};if m.cancel!=nil{m.cancel()};ctx,cancel:=context.WithCancel(m.ctx);m.cancel=cancel;m.request++;m.loading=true;if m.journey=="overview"||m.journey=="graph"||m.journey=="metadata"{m.metadataCurrent=nil}
 metadataAllowed,_:=operatorReadAdmission(m.discovery,"metadata");ticket,client:=m.ticket(),m.client;var savedCursor *string;if cursor!=nil{value:=*cursor;savedCursor=&value}
 return func()tea.Msg{
  var data any;var err error
  switch ticket.Journey{
  case "overview":page,readErr:=client.OperatorServices(ctx,savedCursor,ticket.Filter);err=readErr;if err==nil{read:=operatorOverviewRead{Page:page};if metadataAllowed{for _,row:=range page.Rows{read.Metadata=append(read.Metadata,operatorReadMetadata(ctx,client,ticket,row.ServiceID))}};data=read}
  case "detail":data,err=client.OperatorDetail(ctx,ticket.Service)
  case "graph":page,readErr:=client.OperatorGraph(ctx,savedCursor);err=readErr;if err==nil{read:=operatorGraphRead{Page:page};seen:=map[string]bool{};if metadataAllowed{for _,row:=range page.Rows{if !seen[row.Node.ServiceID]{seen[row.Node.ServiceID]=true;read.Metadata=append(read.Metadata,operatorReadMetadata(ctx,client,ticket,row.Node.ServiceID))}}};data=read}
  case "variables":data,err=client.OperatorVariables(ctx,ticket.Service,ticket.Filter,savedCursor)
  case "endpoints":data,err=client.OperatorEndpoints(ctx,ticket.Service,savedCursor)
  case "lifecycle":data,err=client.OperatorLifecycleAvailability(ctx,ticket.Service)
  case "config":data,err=client.OperatorConfigForm(ctx,ticket.Service)
  case "setup":data,err=client.OperatorSetupForm(ctx,ticket.Service)
  case "declared_actions":data,err=client.OperatorActionForm(ctx,ticket.Service)
  case "updates":data,err=client.OperatorUpdateForm(ctx,ticket.Service)
  case "health_history":data,err=client.OperatorHealthHistory(ctx,ticket.Service,ticket.Filter,savedCursor)
  case "history":data,err=client.OperatorHistory(ctx,ticket.Service,ticket.Filter,savedCursor)
  case "inbox":data,err=client.OperatorInbox(ctx,ticket.Service,ticket.Filter,savedCursor)
  case "queue":data,err=client.OperatorQueue(ctx,ticket.Service,ticket.Filter,savedCursor)
  case "audit":data,err=client.OperatorAudit(ctx,ticket.Service,ticket.Filter,savedCursor)
  case "logs":data,err=client.OperatorSearchLogs(ctx,ticket.Service,ticket.Filter,ticket.Search,savedCursor)
  case "metadata":observation:=operatorReadMetadata(ctx,client,ticket,ticket.Service);err=observation.Err;data=operatorMetadataRead{observation}
  case "removal":data,err=client.OperatorRemovalForm(ctx,ticket.Service)
  default:err=&api.OperatorPublicError{Code:"unsupported_capability"}
  };return operatorLoaded{ticket,savedCursor,data,err}
 }
}
func(m *operatorModel)applyLoaded(msg operatorLoaded)error{
 switch value:=msg.Data.(type){
  case operatorMetadataRead:if value.Observation.Err!=nil{return value.Observation.Err};if !operatorMetadataObservationBound(value.Observation,msg.Ticket)||value.Observation.Key.Service!=msg.Ticket.Service{return &api.OperatorError{Stage:"response"}};m.meta=&value.Observation.Value;m.acceptMetadata(value.Observation)
  case operatorOverviewRead:for _,observation:=range value.Metadata{if !operatorMetadataObservationBound(observation,msg.Ticket){return &api.OperatorError{Stage:"response"}}};if err:=m.services.append(value.Page,msg.Cursor,func(r api.OperatorService)string{return r.ServiceID});err!=nil{return err};for _,observation:=range value.Metadata{m.acceptMetadata(observation)}
  case operatorGraphRead:for _,observation:=range value.Metadata{if !operatorMetadataObservationBound(observation,msg.Ticket){return &api.OperatorError{Stage:"response"}}};if err:=m.graph.append(value.Page,msg.Cursor);err!=nil{return err};for _,observation:=range value.Metadata{m.acceptMetadata(observation)}
 case api.OperatorPage[api.OperatorService]:return m.services.append(value,msg.Cursor,func(r api.OperatorService)string{return r.ServiceID})
 case api.OperatorPage[api.OperatorGraphGroup]:return m.graph.append(value,msg.Cursor)
 case api.OperatorPage[api.OperatorVariable]:return m.variables.append(value,msg.Cursor,func(r api.OperatorVariable)string{return r.Scope+"\x00"+r.Name})
 case api.OperatorPage[api.OperatorEndpoint]:return m.endpoints.append(value,msg.Cursor,func(r api.OperatorEndpoint)string{return r.Protocol+"\x00"+r.Label})
 case api.OperatorPage[api.OperatorHistoryRow]:switch m.journey{case "health_history":return m.health.append(value,msg.Cursor,func(r api.OperatorHistoryRow)string{return r.RowID});default:return m.history.append(value,msg.Cursor,func(r api.OperatorHistoryRow)string{return r.RowID})}
 case api.OperatorPage[api.OperatorInboxRow]:return m.inbox.append(value,msg.Cursor,func(r api.OperatorInboxRow)string{return r.RowID})
 case api.OperatorPage[api.OperatorQueueRow]:return m.queue.append(value,msg.Cursor,func(r api.OperatorQueueRow)string{return r.RowID})
 case api.OperatorPage[api.OperatorAuditRow]:return m.audit.append(value,msg.Cursor,func(r api.OperatorAuditRow)string{return r.RowID})
 case api.OperatorPage[api.OperatorLogRow]:return m.logs.append(value,msg.Cursor,func(r api.OperatorLogRow)string{return r.RowID})
 case api.OperatorDetail:m.detail=&value
 case api.OperatorMeta:return &api.OperatorError{Stage:"response"}
 case api.OperatorLifecycleAvailability:m.lifecycle=&value
 case api.OperatorConfigForm:m.config=&value
 case api.OperatorSetupForm:m.setup=&value
 case api.OperatorActionForm:m.actions=&value
 case api.OperatorUpdateForm:m.updates=&value
 case api.OperatorRemovalForm:m.removal=&value
 default:return &api.OperatorError{Stage:"response"}
 };return nil
}
// One timer or one own-ID read is admitted at a time. Extra bootstrap,
// submission or cancellation scheduling cannot create another polling chain.
func(m *operatorModel)nextPollTick()tea.Cmd{
 if m.pollScheduled||m.pollInFlight{return nil};m.pollTimer++;m.pollScheduled=true;epoch,sequence:=m.epoch,m.pollTimer
 return tea.Tick(time.Second,func(time.Time)tea.Msg{return operatorTick{Epoch:epoch,Sequence:sequence}})
}
