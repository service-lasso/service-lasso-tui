package app

import (
 "context"
 "encoding/json"
 "net/http"
 "net/http/httptest"
 "strings"
 "testing"

 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// Real HTTP decode and reducer sources, UNRUN; no installed runtime/issuer proof.
func TestOperatorRetainedLogPageNeverBecomesAnotherServicePage(t *testing.T){
 reads:=0
 server:=httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter,r *http.Request){
  service:="root";if r.URL.Path=="/api/operator/v2/services/other/logs"{service="other"}else if r.URL.Path!="/api/operator/v2/services/root/logs"{t.Error("unexpected source read");w.WriteHeader(400);return}
  if r.Method!="GET"||r.URL.Query().Get("stream")!="combined"||r.Header.Get("x-service-lasso-admin-token")!="source-token"{t.Error("unexpected mutation/filter/authentication edge");w.WriteHeader(400);return};reads++
  next:="next"
  page:=api.OperatorPage[api.OperatorLogRow]{SnapshotID:"snapshot-"+service,SourceRevision:"revision-"+service,ExpiresAt:"2099-10-08T00:00:00.000Z",Rows:[]api.OperatorLogRow{{RowID:"row-"+service,ServiceID:service,Stream:"stdout",RunID:"run",Time:"2026-10-08T00:00:00.000Z",Text:"observed-"+service}},NextCursor:&next}
  json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorPage[api.OperatorLogRow]]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:page})
 }));defer server.Close()
 client,err:=api.NewClient(server.URL,server.Client(),"source-token");if err!=nil{t.Fatal(err)}
 _,discovery,_:=terminalRefreshFixture(t);discovery.Capabilities[0].ID="read_logs";discovery.Capabilities[0].RequiredScopes=[]string{"service-lasso:read","service-lasso:logs:read"};if !discovery.Valid(){t.Fatal("invalid closed discovery fixture")}
 model:=operatorModel{ctx:context.Background(),client:client,epoch:1,service:"root",journey:"logs",focus:1,width:80,height:20,discovery:&discovery}
 command:=model.load(nil);if command==nil{t.Fatal("missing original read")};originalMessage:=command().(operatorLoaded);if originalMessage.Err!=nil{t.Fatal(originalMessage.Err)}
 updated,_:=model.Update(originalMessage);model=updated.(operatorModel)
 if !strings.Contains(model.View(),"observed-root")||model.nextCursor()==nil{t.Fatal("useful original snapshot was not displayed")}
 model.invalidate();model.service="other"
 if len(model.logs.Rows)!=1||model.logs.Rows[0].Text!="observed-root"{t.Fatal("selection change discarded retained original page")}
 if strings.Contains(model.View(),"observed-root")||model.snapshotStatus()!=""||model.nextCursor()!=nil{t.Fatal("foreign selection published old rows/snapshot/cursor")}
 updated,_=model.Update(originalMessage);model=updated.(operatorModel)
 if model.pageSelectionCurrent(){t.Fatal("obsolete source reply rebound retained snapshot")}
 updated,_=model.Update(operatorLoaded{Ticket:model.ticket(),Err:&api.OperatorPublicError{Code:"permission_denied"}});model=updated.(operatorModel)
 if model.pageSelectionCurrent()||strings.Contains(model.View(),"observed-root")||model.nextCursor()!=nil||len(model.logs.Rows)!=1{t.Fatal("failed new selection read acquired or discarded retained original page")}
 command=model.load(nil);if command==nil{t.Fatal("missing fresh source read")};fresh:=command().(operatorLoaded);if fresh.Err!=nil{t.Fatal(fresh.Err)};updated,_=model.Update(fresh);model=updated.(operatorModel)
 if !strings.Contains(model.View(),"observed-other")||strings.Contains(model.View(),"observed-root")||model.nextCursor()==nil||reads!=2{t.Fatal("fresh actual source did not replace page binding usefully")}
}

func TestOperatorPageBindingRejectsConnectionServiceAndFilterChanges(t *testing.T){
 for _,changed:=range []string{"connection","service","filter","context"}{t.Run(changed,func(t *testing.T){
  next:="next";model:=operatorModel{epoch:1,service:"root",journey:"history",binding:selectedOperatorTestContext("a"),focus:1,filters:map[string]string{"history":"lifecycle"},history:operatorPageState[api.OperatorHistoryRow]{Snapshot:"original",Next:&next,Rows:[]api.OperatorHistoryRow{{RowID:"row",ServiceID:"root",Kind:"lifecycle",Status:"running",Phase:"executing",Summary:"original history"}}}}
  model.acceptPageSelection(model.ticket());if len(model.rowLines())!=1||model.nextCursor()==nil{t.Fatal("same selection lost useful retained history")}
  switch changed{case "connection":model.epoch++;case "service":model.service="other";case "filter":model.filters["history"]="setup";case "context":model.binding=selectedOperatorTestContext("b")}
  if model.pageSelectionCurrent()||len(model.rowLines())!=0||model.nextCursor()!=nil||model.snapshotStatus()!=""{t.Fatal("foreign selection acquired retained history authority")}
  if len(model.history.Rows)!=1||model.history.Rows[0].Summary!="original history"{t.Fatal("foreign selection discarded original history")}
 })}
}
func TestOperatorSameEpochContextChangeRejectsOriginalReadAndPreview(t *testing.T){
 model:=operatorModel{epoch:1,service:"root",journey:"overview",binding:selectedOperatorTestContext("a"),errorText:"current observation"};original:=model.ticket()
 page:=api.OperatorPage[api.OperatorService]{SnapshotID:"snapshot",SourceRevision:"revision",ExpiresAt:"2099-10-08T00:00:00.000Z",Rows:[]api.OperatorService{operatorJourneyService("root")}}
 model.binding=selectedOperatorTestContext("b")
 updated,_:=model.Update(operatorLoaded{Ticket:original,Data:page});model=updated.(operatorModel)
 if model.pageSelectionCurrent()||len(model.services.Rows)!=0{t.Fatal("old-context read stamped current page")}
 updated,_=model.Update(operatorPreviewed{Ticket:original,Err:&api.OperatorPublicError{Code:"permission_denied"}});model=updated.(operatorModel)
 if model.errorText!="current observation"||model.draft.Preview!=nil{t.Fatal("old-context preview/error contaminated current state")}
 updated,_=model.Update(operatorLoaded{Ticket:model.ticket(),Data:page});model=updated.(operatorModel)
 if !model.pageSelectionCurrent()||len(model.services.Rows)!=1{t.Fatal("current-context source result was rejected")}
 encoded,err:=json.Marshal(model.ticket());if err!=nil||strings.Contains(string(encoded),"slrc_"){t.Fatal("private context entered ticket JSON")}
}
func TestOperatorForeignPageCannotStartInboxOrQueueDraft(t *testing.T){
 for _,journey:=range []string{"inbox","queue"}{t.Run(journey,func(t *testing.T){
  _,discovery,_:=terminalRefreshFixture(t);plan:="plan"
  model:=operatorModel{epoch:1,service:"root",journey:journey,focus:1,binding:selectedOperatorTestContext("a"),discovery:&discovery,inbox:operatorPageState[api.OperatorInboxRow]{Revision:"revision",Rows:[]api.OperatorInboxRow{{RowID:"row",ServiceID:"root",Kind:"service",Source:"runtime",Severity:"info",Status:"unread",Visibility:"visible",CreatedAt:"2026-10-08T00:00:00.000Z",Summary:"Original inbox",CapabilityIDs:[]string{"inbox_mark_read"}}}},queue:operatorPageState[api.OperatorQueueRow]{Revision:"revision",Rows:[]api.OperatorQueueRow{{RowID:"row",ServiceID:"root",Kind:"manual",Status:"open",Severity:"info",CreatedAt:"2026-10-08T00:00:00.000Z",Summary:"Original queue",PlanID:&plan}}}}
  model.acceptPageSelection(model.ticket());model.beginEdit();if !model.draft.Dirty{t.Fatal("same selection lost useful original row draft")}
  model.draft=operatorDraft{};model.invalidate();model.service="other"
  if command:=model.beginEdit();command!=nil||model.draft.Dirty{t.Fatal("foreign page acquired edit/request authority")}
  if len(model.inbox.Rows)!=1||len(model.queue.Rows)!=1{t.Fatal("edit denial discarded retained original rows")}
 })}
}

func TestOperatorExpiredPageCannotContinueBeforeNextTick(t *testing.T){
 for _,journey:=range []string{"overview","graph","variables","endpoints","health_history","history","inbox","queue","audit","logs"}{t.Run(journey,func(t *testing.T){
  next:="original-next";model:=operatorModel{epoch:1,service:"root",journey:journey,binding:selectedOperatorTestContext("a")}
  model.services.Next=&next;model.graph.Next=&next;model.variables.Next=&next;model.endpoints.Next=&next;model.health.Next=&next;model.history.Next=&next;model.inbox.Next=&next;model.queue.Next=&next;model.audit.Next=&next;model.logs.Next=&next
  setExpiry:=func(expiry string){model.services.Expiry=expiry;model.graph.Expiry=expiry;model.variables.Expiry=expiry;model.endpoints.Expiry=expiry;model.health.Expiry=expiry;model.history.Expiry=expiry;model.inbox.Expiry=expiry;model.queue.Expiry=expiry;model.audit.Expiry=expiry;model.logs.Expiry=expiry}
  model.acceptPageSelection(model.ticket());setExpiry("2099-10-08T00:00:00.000Z")
  if model.nextCursor()==nil{t.Fatal("unexpired original selection lost useful continuation")}
  setExpiry("2000-10-08T00:00:00.000Z")
  if model.nextCursor()!=nil{t.Fatal("expired original snapshot supplied continuation before tick")}
  if model.services.Next==nil||*model.services.Next!=next||model.logs.Next==nil||*model.logs.Next!=next{t.Fatal("expiry denial discarded retained original handles")}
 })}
}

func TestOperatorExpiredInboxOrQueueCannotBeginDraftBeforeNextTick(t *testing.T){
 for _,journey:=range []string{"inbox","queue"}{t.Run(journey,func(t *testing.T){
  _,discovery,_:=terminalRefreshFixture(t);plan:="plan"
  model:=operatorModel{epoch:1,service:"root",journey:journey,focus:1,binding:selectedOperatorTestContext("a"),discovery:&discovery,inbox:operatorPageState[api.OperatorInboxRow]{Expiry:"2000-10-08T00:00:00.000Z",Revision:"original-revision",Rows:[]api.OperatorInboxRow{{RowID:"original-row",ServiceID:"root",Kind:"service",Source:"runtime",Severity:"info",Status:"unread",Visibility:"visible",CreatedAt:"2026-10-08T00:00:00.000Z",Summary:"Original inbox",CapabilityIDs:[]string{"inbox_mark_read"}}}},queue:operatorPageState[api.OperatorQueueRow]{Expiry:"2000-10-08T00:00:00.000Z",Revision:"original-revision",Rows:[]api.OperatorQueueRow{{RowID:"original-row",ServiceID:"root",Kind:"manual",Status:"open",Severity:"info",CreatedAt:"2026-10-08T00:00:00.000Z",Summary:"Original queue",PlanID:&plan}}}}
  model.acceptPageSelection(model.ticket())
  if command:=model.beginEdit();command!=nil||model.draft.Dirty{t.Fatal("expired row supplied local draft admission before tick")}
  if !model.inbox.Stale||!model.queue.Stale||len(model.inbox.Rows)!=1||len(model.queue.Rows)!=1||model.inbox.Revision!="original-revision"||model.queue.Revision!="original-revision"{t.Fatal("expiry failed to mark stale or discarded original rows/revisions")}
 })}
}
func TestOperatorSnapshotEmptyLabelFollowsAdmittedReducerObservation(t *testing.T){
 model:=operatorModel{epoch:1,journey:"overview",focus:1,width:120,height:30}
 page:=api.OperatorPage[api.OperatorService]{SnapshotID:"original",SourceRevision:"revision",ExpiresAt:"2099-10-08T00:00:00.000Z",Rows:[]api.OperatorService{}}
 updated,_:=model.Update(operatorLoaded{Ticket:model.ticket(),Data:page});model=updated.(operatorModel)
 if !strings.Contains(model.View(),"complete; no rows"){t.Fatal("admitted complete empty snapshot was not explicit")}
 retained:=model
 model.services.Truncated=true
 if strings.Contains(model.View(),"no rows")||!strings.Contains(model.View(),"omitted rows remain unknown"){t.Fatal("truncated observation became empty")}
 model=retained;model.services.Expiry="2000-10-08T00:00:00.000Z"
 if strings.Contains(model.View(),"no rows")||!strings.Contains(model.View(),"retained rows need refresh"){t.Fatal("expired before-tick observation became empty")}
 model=retained;model.epoch++
 if strings.Contains(model.View(),"no rows")||model.snapshotStatus()!=""{t.Fatal("foreign selection acquired an empty observation")}
 model=retained;page.Rows=[]api.OperatorService{operatorJourneyService("root")}
 updated,_=model.Update(operatorLoaded{Ticket:model.ticket(),Data:page});model=updated.(operatorModel)
 if strings.Contains(model.View(),"no rows")||!strings.Contains(model.View(),"root"){t.Fatal("useful populated replacement stayed empty")}
}
func TestOperatorMetadataUnknownLayoutNeverBecomesAutomatic(t *testing.T){
 for _,reason:=range []string{"source_unavailable","source_corrupt","ready"}{t.Run(reason,func(t *testing.T){
  ready:=reason=="ready";meta:=api.OperatorMeta{ServiceID:"root",Availability:api.OperatorAvailability{Supported:true}}
  if ready{revision:="metadata-revision";favorite:=false;meta.Availability.Available=true;meta.SourceRevision=&revision;meta.Favorite=&favorite}else{value:=reason;meta.Availability.ReasonCode=&value}
  if !meta.Valid(){t.Fatal("metadata fixture violates current closed codec")}
  server:=httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter,r *http.Request){
   if metadataBindingFixture(t,w,r){return};if r.Method!="GET"||r.URL.Path!="/api/operator/v2/services/root/meta"{t.Error("unexpected metadata mutation/path");w.WriteHeader(400);return}
   json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorMeta]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:meta})
  }));defer server.Close()
  client,err:=api.NewClient(server.URL,server.Client(),"source-token");if err!=nil{t.Fatal(err)}
  _,discovery,_:=terminalRefreshFixture(t);discovery.Capabilities[0].ID="service_meta_read"
  if !discovery.Valid(){t.Fatal("metadata discovery fixture violates current codec")}
  model:=operatorModel{ctx:context.Background(),client:client,epoch:1,service:"root",journey:"metadata",focus:1,width:120,height:30,discovery:&discovery,binding:selectedOperatorTestContext("a")}
  command:=model.load(nil);if command==nil{t.Fatal("missing useful metadata read")};message:=command().(operatorLoaded);if message.Err!=nil{t.Fatal(message.Err)}
  updated,_:=model.Update(message);model=updated.(operatorModel);screen:=model.View()
  if ready{if !strings.Contains(screen,"layout: automatic")||!strings.Contains(screen,"favorite=false"){t.Fatal("ready null-layout/default-favorite observation lost useful meaning")}}else{
   if !strings.Contains(screen,"layout: unknown")||!strings.Contains(screen,"favorite=unknown")||strings.Contains(screen,"layout: automatic"){t.Fatal("unknown preference acquired a default layout")}
   if command=model.beginEdit();command!=nil||model.draft.Dirty||model.meta.SourceRevision!=nil{t.Fatal("unknown metadata acquired edit authority")}
  }
 })}
}
func TestOperatorUnavailableSetupFormIsDistinctFromReadyEmpty(t *testing.T){
 for _,ready:=range []bool{false,true}{
  reason:="source_unavailable";availability:=api.OperatorAvailability{Supported:true,Available:ready};if !ready{availability.ReasonCode=&reason}
  form:=api.OperatorSetupForm{ServiceID:"root",DefinitionRevision:"definition",Steps:[]api.OperatorSetupStep{},Availability:availability}
  server:=httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter,r *http.Request){
   if r.Method!="GET"||r.URL.Path!="/api/operator/v2/services/root/setup-form"{t.Error("unexpected setup mutation/path");w.WriteHeader(400);return}
   json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorSetupForm]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:form})
  }))
  client,err:=api.NewClient(server.URL,server.Client(),"source-token");if err!=nil{server.Close();t.Fatal(err)}
  _,discovery,_:=terminalRefreshFixture(t)
  model:=operatorModel{ctx:context.Background(),client:client,epoch:1,service:"root",journey:"setup",focus:1,width:120,height:30,discovery:&discovery,binding:selectedOperatorTestContext("a")}
  command:=model.load(nil);if command==nil{server.Close();t.Fatal("missing setup read")};message:=command().(operatorLoaded);server.Close();if message.Err!=nil{t.Fatal(message.Err)}
  updated,_:=model.Update(message);model=updated.(operatorModel);screen:=model.View()
  if ready{if !strings.Contains(screen,"Form: available; no declared rows"){t.Fatal("ready empty form lost distinct meaning")}}else{
   if !strings.Contains(screen,"Form: source unavailable")||strings.Contains(screen,"no declared rows"){t.Fatal("unavailable empty roster became a ready empty form")}
   model.height=1;if !strings.Contains(model.View(),"Form: source unavailable"){t.Fatal("narrow pane hid original form unavailability")}
  }
  if command=model.beginEdit();command!=nil||model.draft.Dirty{t.Fatal("unavailable or genuinely empty form acquired row edit authority")}
 }
}
func TestOperatorOriginalSetupFormDoesNotAcquireNewBootContext(t *testing.T){
 ready:=api.OperatorAvailability{Supported:true,Available:true};revision:="revision"
 form:=api.OperatorSetupForm{ServiceID:"root",DefinitionRevision:revision,Availability:ready,Steps:[]api.OperatorSetupStep{{StepID:"step",Description:"Original declared step",Rerun:"always",DefinitionRevision:revision,InputFields:[]api.OperatorField{},Availability:ready,ExpectedRevision:&revision}}}
 reads:=0;server:=httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter,r *http.Request){
  if r.Method!="GET"||r.URL.Path!="/api/operator/v2/services/root/setup-form"{t.Error("unexpected setup mutation/path");w.WriteHeader(400);return};reads++
  json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorSetupForm]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:form})
 }));defer server.Close()
 client,err:=api.NewClient(server.URL,server.Client(),"source-token");if err!=nil{t.Fatal(err)}
 _,discovery,_:=terminalRefreshFixture(t)
 model:=operatorModel{ctx:context.Background(),client:client,epoch:1,service:"root",journey:"setup",focus:1,width:120,height:30,discovery:&discovery,binding:selectedOperatorTestContext("a")}
 command:=model.load(nil);if command==nil{t.Fatal("missing own setup read")};original:=command().(operatorLoaded);if original.Err!=nil{t.Fatal(original.Err)}
 updated,_:=model.Update(original);model=updated.(operatorModel)
 if !model.pageSelectionCurrent()||!strings.Contains(model.View(),"Original declared step"){t.Fatal("admitted original form was not useful")}
 model.beginEdit();if !model.draft.Dirty{t.Fatal("original admitted form lost typed editing")};model.draft=operatorDraft{}
 retained:=model.setup
 updated,_=model.Update(operatorBooted{Epoch:model.epoch,Discovery:discovery,Context:selectedOperatorTestContext("b")});model=updated.(operatorModel)
 if model.setup!=retained||model.pageSelectionCurrent()||len(model.rowLines())!=0{t.Fatal("new same-epoch context acquired or discarded original form")}
 if status,_:=model.formStatus();status!=""||strings.Contains(model.View(),"Form: available")||strings.Contains(model.View(),"Original declared step"){t.Fatal("foreign retained form supplied availability/empty/row presentation")}
 updated,_=model.Update(original);model=updated.(operatorModel)
 updated,_=model.Update(operatorLoaded{Ticket:model.ticket(),Err:&api.OperatorPublicError{Code:"permission_denied"}});model=updated.(operatorModel)
 if model.pageSelectionCurrent()||model.setup!=retained||model.beginEdit()!=nil||model.draft.Dirty{t.Fatal("stale or failed current read rebound original edit authority")}
 command=model.load(nil);fresh:=command().(operatorLoaded);if fresh.Err!=nil{t.Fatal(fresh.Err)};updated,_=model.Update(fresh);model=updated.(operatorModel)
 model.beginEdit();if !model.pageSelectionCurrent()||!model.draft.Dirty||model.draft.Context!=selectedOperatorTestContext("b")||reads!=2{t.Fatal("fresh genuine read did not restore useful current form editing")}
}

func TestOperatorRetainedFormLabelsRequireOriginalAdmittedSelection(t *testing.T){
 for _,journey:=range []string{"lifecycle","config","setup","declared_actions","removal","metadata","detail","updates"}{t.Run(journey,func(t *testing.T){
  for _,changed:=range []string{"connection","service","filter","context"}{
   reason:="source_unavailable";denied:=api.OperatorAvailability{Supported:true,ReasonCode:&reason}
   model:=operatorModel{epoch:1,service:"root",journey:journey,focus:1,binding:selectedOperatorTestContext("a"),lifecycle:&api.OperatorLifecycleAvailability{Availability:denied},config:&api.OperatorConfigForm{Availability:denied},setup:&api.OperatorSetupForm{Availability:denied},actions:&api.OperatorActionForm{Availability:denied},removal:&api.OperatorRemovalForm{Availability:denied},meta:&api.OperatorMeta{Availability:denied},detail:&api.OperatorDetail{},updates:&api.OperatorUpdateForm{Availability:denied}}
   model.acceptPageSelection(model.ticket());if !model.pageSelectionCurrent(){t.Fatal("original synthetic admitted read lost selection")}
   switch changed{case "connection":model.epoch++;case "service":model.service="foreign";case "filter":model.filters=map[string]string{journey:"foreign"};case "context":model.binding=selectedOperatorTestContext("b")}
   if model.pageSelectionCurrent()||len(model.rowLines())!=0{t.Fatal("foreign read selection acquired retained form rows")}
   if status,_:=model.formStatus();status!=""{t.Fatal("foreign read selection acquired retained form reason")}
   if model.setup==nil||model.config==nil||model.actions==nil||model.meta==nil{t.Fatal("presentation denial discarded private retained form observations")}
  }
 })}
}