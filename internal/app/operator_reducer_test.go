package app

import (
 "context"
 "encoding/json"
 "strings"
 "testing"

 tea "github.com/charmbracelet/bubbletea"
 "github.com/charmbracelet/x/ansi"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// Reducer/unit source cases. They do not exercise or qualify packaged Core.
func TestOperatorObsoleteTicketCannotReplaceSelection(t *testing.T){
 model:=operatorModel{epoch:1,selection:2,request:3,service:"new",journey:"detail",width:80,height:24}
 old:=operatorTicket{Connection:1,Selection:1,Request:2,Service:"old",Journey:"detail"}
 updated,_:=model.Update(operatorLoaded{Ticket:old,Data:api.OperatorDetail{Summary:api.OperatorService{ServiceID:"old"}}})
 actual:=updated.(operatorModel);if actual.detail!=nil||actual.service!="new"{t.Fatal("obsolete response replaced current root")}
}
func TestOperatorBootRestoreContextMismatchRetainsRaw(t *testing.T){
 a:=api.OperatorContext{InstanceBinding:"slrc_"+strings.Repeat("a",64),WorkspaceBinding:"slrc_"+strings.Repeat("b",64),ActorBinding:"slrc_"+strings.Repeat("c",64),ClientBinding:"slrc_"+strings.Repeat("d",64)};b:=a;b.InstanceBinding="slrc_"+strings.Repeat("e",64)
 if !a.Valid()||!b.Valid(){t.Fatal("context fixture violates the actual binding codec")}
 model:=operatorModel{ctx:context.Background(),epoch:1,journey:"overview",width:80,height:24}
 record:=api.OperatorSavedV3{Version:3,OperationID:"old",ServiceID:"original",ActionID:"service_start",Context:b}
 raw,err:=json.Marshal(record);if err!=nil{t.Fatal(err)};if _,err=api.DecodeOperatorSavedV3(raw);err!=nil{t.Fatal("saved fixture violates the actual record codec",err)}
 outcome:="succeeded";operation:=api.OperatorOperation{OperationID:"old",ServiceID:"original",ActionID:"service_start",Targets:[]string{"original"},Status:"succeeded",Phase:"completed",Outcome:&outcome,CorrelationID:"correlation",CreatedAt:"2026-10-08T00:00:00.000Z",UpdatedAt:"2026-10-08T00:00:00.000Z",ExpiresAt:"2099-10-08T00:00:00.000Z"}
 discovery:=api.OperatorDiscovery{MetadataRevision:"metadata",ModelEpoch:"epoch",SupportedJourneys:[]string{},MutationSemantics:"unavailable",Limits:api.OperatorLimits{RequestBytes:65536,ResponseBytes:262144,ErrorBytes:8192,Depth:16,TotalValues:8192,PropertyNameBytes:128,PublicStringBytes:4096,IDBytes:128,SummaryBytes:512,DisplayNameBytes:256,DescriptionBytes:1024,PageRows:100,ContextBytes:4096},Capabilities:[]api.OperatorCapability{}}
 if !operation.Valid()||!discovery.Valid(){t.Fatal("boot fixture violates the actual public DTO codec")}
 message:=operatorBooted{Epoch:1,Discovery:discovery,Context:a,Restore:operatorRestoration{Raw:raw,Record:record,Readback:&api.OperatorReadback{Operation:operation}}}
 updated,_:=model.Update(message);actual:=updated.(operatorModel)
 if actual.operation!=nil||string(actual.saved)!=string(raw)||actual.warning==""{t.Fatal("mismatched restore attached or lost original record")}
}
func TestOperatorSelectionPreservesChosenOverviewRow(t *testing.T){
 model:=operatorModel{ctx:context.Background(),epoch:1,journey:"overview",focus:1,row:1,services:operatorPageState[api.OperatorService]{Rows:[]api.OperatorService{{ServiceID:"first"},{ServiceID:"chosen"}}}}
 model.acceptPageSelection(model.ticket())
 updated,_:=model.Update(tea.KeyMsg{Type:tea.KeyEnter});actual:=updated.(operatorModel)
 if actual.service!="chosen"||actual.journey!="detail"{t.Fatal("selection reset chose a different row")}
}
func TestOperatorNarrowPageKeysHaveStableDirection(t *testing.T){
 model:=operatorModel{epoch:1,journey:"overview",focus:1,height:3,scroll:1,row:1,services:operatorPageState[api.OperatorService]{Rows:[]api.OperatorService{{ServiceID:"a"},{ServiceID:"b"},{ServiceID:"c"}}}}
 model.acceptPageSelection(model.ticket())
 updated,_:=model.Update(tea.KeyMsg{Type:tea.KeyPgDown});down:=updated.(operatorModel);if down.scroll!=2{t.Fatal("narrow page down reversed or skipped movement")}
 updated,_=down.Update(tea.KeyMsg{Type:tea.KeyPgUp});up:=updated.(operatorModel);if up.scroll!=1{t.Fatal("narrow page up reversed movement")}
}
func TestOperatorEscapeInvalidatesPendingPreview(t *testing.T){
 model:=operatorModel{epoch:1,request:2,journey:"lifecycle",draft:operatorDraft{Dirty:true,Action:"service_start"}}
 ticket:=model.ticket();model.editKey(tea.KeyMsg{Type:tea.KeyEsc})
 updated,_:=model.Update(operatorPreviewed{Ticket:ticket,Preview:api.OperatorPreview{PlanID:"late"}})
 if updated.(operatorModel).draft.Preview!=nil{t.Fatal("cancelled draft admitted late preview")}
}
func TestOperatorPollingReplacesShrunkAndEmptyTargets(t *testing.T){
 original:=api.OperatorOperation{OperationID:"exact",ServiceID:"root",ActionID:"service_start",Targets:[]string{"root","dependency"}}
 model:=operatorModel{epoch:1,pollRequest:1,pollInFlight:true,operation:&api.OperatorReadback{Operation:original}}
 for _,targets:=range [][]string{{"root"},{}}{model.pollInFlight=true;readback:=original;readback.Targets=targets;updated,_:=model.Update(operatorPoll{Epoch:1,Sequence:1,ID:"exact",Readback:api.OperatorReadback{Operation:readback}});model=updated.(operatorModel);if len(model.operation.Operation.Targets)!=len(targets){t.Fatal("polling merged previous targets")}}
}
func TestOperatorRendererEscapesDataAndRespectsCells(t *testing.T){
 model:=operatorModel{journey:"logs",focus:1,width:12,height:8,logs:operatorPageState[api.OperatorLogRow]{Rows:[]api.OperatorLogRow{{Text:"safe\x1b[2J\n\t界👩‍💻"}}}}
 model.acceptPageSelection(model.ticket())
 if operatorText("a\x1b\nb")!="a\\x1b\\nb"{t.Fatal("literal controls escaped incorrectly")}
 for _,line:=range strings.Split(model.View(),"\n"){if strings.ContainsRune(line,'\x1b')||ansi.StringWidth(line)>model.width{t.Fatal("unsafe control or excessive terminal cell width")}}
 if removeLastGrapheme("a👩‍💻")!="a"{t.Fatal("backspace split a Unicode grapheme")}
}
func TestOperatorPageRejectsMixedSnapshotAndDuplicates(t *testing.T){
 next:="next";state:=operatorPageState[api.OperatorInboxRow]{}
 first:=api.OperatorPage[api.OperatorInboxRow]{SnapshotID:"a",SourceRevision:"r",ExpiresAt:"2100-01-01T00:00:00.000Z",Rows:[]api.OperatorInboxRow{{RowID:"one"}},NextCursor:&next}
 if err:=state.append(first,nil,func(row api.OperatorInboxRow)string{return row.RowID});err!=nil{t.Fatal(err)}
 second:=first;second.SnapshotID="b";second.NextCursor=nil;if state.append(second,&next,func(row api.OperatorInboxRow)string{return row.RowID})==nil{t.Fatal("mixed snapshot continuation admitted")}
 state=operatorPageState[api.OperatorInboxRow]{};_ = state.append(first,nil,func(row api.OperatorInboxRow)string{return row.RowID});second=first;second.NextCursor=nil;if state.append(second,&next,func(row api.OperatorInboxRow)string{return row.RowID})==nil{t.Fatal("duplicate row across pages admitted")}
}
func TestOperatorEditorCursorAndDeletePreserveGraphemes(t *testing.T){
 draft:=operatorDraft{Text:"a👩‍💻界z",Cursor:len("a👩‍💻界z")}
 draft.moveCursor(false);if draft.Cursor!=len("a👩‍💻界"){t.Fatal("left movement split character")}
 draft.moveCursor(false);draft.erase(true);if draft.Text!="a界z"||draft.Cursor!=1{t.Fatal("backspace did not remove one complete joined emoji")}
 draft.erase(false);if draft.Text!="az"{t.Fatal("Delete did not remove one wide grapheme")}
 draft.insert("e\u0301",100);draft.moveCursor(false);if draft.Cursor!=1{t.Fatal("cursor split combining sequence")}
 draft.erase(false);if draft.Text!="az"{t.Fatal("Delete split combining sequence")}
}
func TestOperatorEditorCRLFLineBoundaries(t *testing.T){
 draft:=operatorDraft{Text:"界👩‍💻\r\ne\u0301z",Cursor:0}
 draft.lineBoundary(true);if draft.Cursor!=len("界👩‍💻"){t.Fatal("End entered CRLF grapheme")}
 draft.moveLine(true);if draft.Cursor!=len(draft.Text){t.Fatal("down did not clamp to complete next-line graphemes")}
 draft.lineBoundary(false);if draft.Cursor!=len("界👩‍💻\r\n"){t.Fatal("Home entered CRLF grapheme")}
 draft.moveLine(false);if draft.Cursor!=0{t.Fatal("up did not preserve line column")}
 draft.lineBoundary(true);draft.erase(false);if draft.Text!="界👩‍💻e\u0301z"{t.Fatal("Delete split CRLF grapheme")}
}
func TestOperatorUnknownRequestAllowsReadOnlyNavigation(t *testing.T){
 original:=operatorDraft{Sent:true,Dirty:true,Action:"install",Context:api.OperatorContext{},Input:[]byte("{}")}
 model:=operatorModel{journey:"overview",focus:0,width:80,height:24,draft:original}
 updated,_:=model.Update(tea.KeyMsg{Type:tea.KeyTab});actual:=updated.(operatorModel)
 if actual.focus!=1||!actual.draft.Sent||string(actual.draft.Input)!="{}"{t.Fatal("read navigation lost unknown request custody")}
 actual.beginEdit();if !actual.draft.Sent||actual.draft.Action!="install"{t.Fatal("new editing replaced unknown request")}
 updated,_=actual.Update(tea.KeyMsg{Type:tea.KeyRunes,Runes:[]rune{'c'},Alt:true});actual=updated.(operatorModel)
 if !actual.draft.Sent||actual.connection!=model.connection{t.Fatal("connection change discarded original request")}
}
func TestOperatorLateDirectDenialIsNotDiscardedByBrowsing(t *testing.T){
 ticket:=operatorTicket{Connection:1,Selection:1,Request:1,Service:"original",Journey:"metadata"}
 model:=operatorModel{epoch:1,selection:2,request:2,service:"other",journey:"detail",draft:operatorDraft{Sent:true,Dirty:true,Action:"meta_favorite",MutationTicket:&ticket}}
 updated,_:=model.Update(operatorDirectResult{Ticket:ticket,Binding:model.draft.Context,Err:&api.OperatorError{Stage:"context"}});actual:=updated.(operatorModel)
 if actual.draft.Sent||!actual.draft.Retired||actual.errorText==""||actual.service!="other"{t.Fatal("late denial lost original custody or rewrote current browsing selection")}
 foreign:=ticket;foreign.Request++
 model.draft.Sent=true;updated,_=model.Update(operatorDirectResult{Ticket:foreign,Binding:model.draft.Context,Err:&api.OperatorError{Stage:"context"}});actual=updated.(operatorModel)
 if !actual.draft.Sent{t.Fatal("foreign response modified retained request")}
}
func TestOperatorGraphEmptyContinuationRetainsPriorRows(t *testing.T){
 next:="next";graph:=operatorGraphState{Snapshot:"old",Nodes:[]api.OperatorNode{{ServiceID:"retained"}}}
 page:=api.OperatorPage[api.OperatorGraphGroup]{SnapshotID:"new",SourceRevision:"revision",ExpiresAt:"2100-01-01T00:00:00.000Z",Rows:[]api.OperatorGraphGroup{},NextCursor:&next}
 if graph.append(page,nil)==nil||!graph.Stale||len(graph.Nodes)!=1||graph.Nodes[0].ServiceID!="retained"{t.Fatal("empty continued first page replaced retained graph")}
 graph=operatorGraphState{Snapshot:"new",Revision:"revision",Expiry:page.ExpiresAt,Next:&next,Nodes:[]api.OperatorNode{{ServiceID:"retained"}}};later:="later";page.NextCursor=&later
 if graph.append(page,&next)==nil||!graph.Stale||len(graph.seenCursors)!=0{t.Fatal("nonprogressing graph continuation grew cursor roster")}
}

func TestOperatorPollingRejectsChangedOriginalCorrelation(t *testing.T){
 original:=api.OperatorOperation{OperationID:"exact",ServiceID:"root",ActionID:"service_start",Targets:[]string{"root","dependency"},Status:"running",Phase:"executing",CorrelationID:"original-correlation",CreatedAt:"2026-10-08T00:00:00.000Z",UpdatedAt:"2026-10-08T00:00:00.000Z",ExpiresAt:"2099-10-08T00:00:00.000Z"}
 changed:=original;changed.CorrelationID="foreign-correlation";changed.Targets=[]string{}
 if !original.Valid()||!changed.Valid(){t.Fatal("correlation fixture violates the actual operation codec")}
 model:=operatorModel{epoch:1,pollRequest:1,pollInFlight:true,operation:&api.OperatorReadback{Operation:original}}
 updated,_:=model.Update(operatorPoll{Epoch:1,Sequence:1,ID:"exact",Readback:api.OperatorReadback{Operation:changed}})
 actual:=updated.(operatorModel)
 if actual.operation.Operation.CorrelationID!=original.CorrelationID||len(actual.operation.Operation.Targets)!=len(original.Targets)||actual.warning==""{t.Fatal("same-ID foreign correlation replaced original operation authority")}
}
