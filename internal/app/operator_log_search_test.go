package app

import (
 "context"
 "encoding/json"
 "net/http"
 "net/http/httptest"
 "strings"
 "testing"
 tea "github.com/charmbracelet/bubbletea"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// HTTP/load/reducer/view source cases, UNRUN; no actual Core/native issuer.
func TestOperatorLogSearchApplyRejectsOldQueryResponseAndCursor(t *testing.T){
 reads:=0;server:=httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter,r *http.Request){reads++;if r.Method!="GET"||r.URL.Path!="/api/operator/v2/services/root/logs"||r.URL.Query().Get("search")!="界 +"||r.URL.Query().Has("cursor"){t.Error("search reused old cursor or changed query")};page:=api.OperatorPage[api.OperatorLogRow]{SnapshotID:"new-snapshot",SourceRevision:"new-source",Rows:[]api.OperatorLogRow{{RowID:"new-row",ServiceID:"root",Stream:"combined",RunID:"run",Time:"2026-10-09T00:00:00.000Z",Text:"new matched row"}},ExpiresAt:"2099-10-09T00:00:00.000Z"};json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorPage[api.OperatorLogRow]]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-09T00:00:00.000Z",Data:page})}));defer server.Close();client,err:=api.NewClient(server.URL,server.Client(),"source-token");if err!=nil{t.Fatal(err)}
 discovery:=api.OperatorDiscovery{Capabilities:[]api.OperatorCapability{{ID:"read_logs",Availability:api.OperatorAvailability{Supported:true,Available:true}}}} // explicit selection vector; no discovery issuer
 cursor:="old-cursor";model:=operatorModel{ctx:context.Background(),client:client,discovery:&discovery,epoch:1,service:"root",journey:"logs",focus:1,width:40,height:8,binding:selectedOperatorTestContext("a"),logs:operatorPageState[api.OperatorLogRow]{Snapshot:"old-snapshot",Revision:"old-source",Rows:[]api.OperatorLogRow{{RowID:"old-row",Text:"old unmatched row"}},Next:&cursor,Expiry:"2099-10-09T00:00:00.000Z"}}
 model.acceptPageSelection(model.ticket());oldTicket:=model.ticket();original,_:=json.Marshal(model.logs)
 updated,command:=model.Update(tea.KeyMsg{Type:tea.KeyRunes,Runes:[]rune{'/'}});model=updated.(operatorModel);if command!=nil||model.logSearchDraft==nil||model.draft.Dirty{t.Fatal("search opened mutation draft")}
 updated,_=model.Update(tea.KeyMsg{Type:tea.KeyRunes,Runes:[]rune("界 +")});model=updated.(operatorModel)
 updated,command=model.Update(tea.KeyMsg{Type:tea.KeyEnter});model=updated.(operatorModel)
 if command==nil||model.logSearch!="界 +"||model.logSearchDraft!=nil||model.pageSelectionCurrent()||model.nextCursor()!=nil||len(model.rowLines())!=0{t.Fatal("new query inherited old rows/cursor authority")}
 updated,_=model.Update(operatorLoaded{Ticket:oldTicket,Data:api.OperatorPage[api.OperatorLogRow]{Rows:[]api.OperatorLogRow{{Text:"late old reply"}}}});model=updated.(operatorModel);retained,_:=json.Marshal(model.logs);if string(retained)!=string(original)||!model.loading{t.Fatal("late query reply replaced original page or loading state")}
 reply:=command().(operatorLoaded);if reply.Err!=nil{t.Fatal(reply.Err)};updated,_=model.Update(reply);model=updated.(operatorModel);if reads!=1||!model.pageSelectionCurrent()||len(model.logs.Rows)!=1||model.logs.Rows[0].RowID!="new-row"{t.Fatal("fresh selected query failed publication")}
}
func TestOperatorLogSearchEscapeUnicodeCursorAndRetainedMutationCustody(t *testing.T){
 originalDraft:=operatorDraft{Sent:true,Dirty:true,Action:"declared_action_run",Input:[]byte("retained-input"),Text:"retained-phrase"};model:=operatorModel{epoch:1,journey:"logs",focus:1,width:10,height:1,logSearch:"界👩‍💻",draft:originalDraft,custodyBlocked:true,custodyWarning:"retained warning"};original,_:=privateOperatorDraftSnapshot(model.draft)
 updated,_:=model.Update(tea.KeyMsg{Type:tea.KeyRunes,Runes:[]rune{'/'}});model=updated.(operatorModel)
 updated,_=model.Update(tea.KeyMsg{Type:tea.KeyLeft});model=updated.(operatorModel);updated,_=model.Update(tea.KeyMsg{Type:tea.KeyBackspace});model=updated.(operatorModel);if model.logSearchDraft.Text!="👩‍💻"{t.Fatal("search cursor split original grapheme")}
 updated,_=model.Update(tea.KeyMsg{Type:tea.KeyRunes,Runes:[]rune{'\x1b'}});model=updated.(operatorModel);model.width=80;model.height=8;view:=model.View();if strings.ContainsRune(view,'\x1b')||!strings.Contains(view,"\\x1b"){t.Fatal("literal query control escaped terminal boundary")}
 updated,command:=model.Update(tea.KeyMsg{Type:tea.KeyEsc});model=updated.(operatorModel);retained,_:=privateOperatorDraftSnapshot(model.draft);if command!=nil||model.logSearchDraft!=nil||model.logSearch!="界👩‍💻"||string(retained)!=string(original)||!model.custodyBlocked||model.custodyWarning!="retained warning"{t.Fatal("search Escape changed original query/mutation/storage custody")}
 model.logSearchDraft=&operatorDraft{Text:strings.Repeat("界",85),Cursor:255};model.logSearchKey(tea.KeyMsg{Type:tea.KeyRunes,Runes:[]rune{'界'}});if len(model.logSearchDraft.Text)!=255{t.Fatal("Unicode search exceeded byte bound")}
 model.logSearchKey(tea.KeyMsg{Type:tea.KeyCtrlU});if model.logSearchDraft.Text!=""||model.logSearchDraft.Cursor!=0{t.Fatal("query clear was not explicit")}
}