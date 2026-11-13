package app

import (
 "encoding/json"
 "strings"
 "testing"

 tea "github.com/charmbracelet/bubbletea"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// Source cases only, UNRUN. These do not qualify runtime snapshot issuance.
func TestOperatorSemanticReadRefusalRetiresContinuation(t *testing.T){
 cursor:="original-cursor"
 state:=operatorModel{epoch:1,selection:1,request:1,journey:"overview",focus:1,width:80,height:24}
 page:=api.OperatorPage[api.OperatorService]{SnapshotID:"original",SourceRevision:"revision",ExpiresAt:"2099-10-08T00:00:00.000Z",Rows:[]api.OperatorService{{ServiceID:"root",DisplayName:"original-row"}},NextCursor:&cursor}
 if err:=state.services.append(page,nil,func(row api.OperatorService)string{return row.ServiceID});err!=nil{t.Fatal(err)}
 state.acceptPageSelection(state.ticket())
 if state.nextCursor()==nil{t.Fatal("useful initial continuation denied")}
 // A foreign metadata observation rejects the composite before page.append.
 foreign:=state.ticket();foreign.Selection++
 refused:=operatorLoaded{Ticket:state.ticket(),Data:operatorOverviewRead{Page:page,Metadata:[]operatorMetaObservation{{Ticket:foreign}}}}
 updated,follow:=state.Update(refused);state=updated.(operatorModel)
 if follow!=nil||!state.services.Stale||state.nextCursor()!=nil||len(state.services.Rows)!=1||state.services.Rows[0].DisplayName!="original-row"||state.services.Snapshot!="original"{t.Fatal("semantic refusal discarded rows or retained cursor authority")}
 if len(state.receivedFailures)!=1||state.receivedFailures[0].Err==nil||state.receivedFailures[0].Message.(operatorLoaded).Ticket!=refused.Ticket{t.Fatal("actual reducer refusal/message lost")}
 encoded,err:=json.Marshal(state.receivedFailures);if err!=nil||string(encoded)!="[{}]"{t.Fatal("private refusal graph published")}
 if !strings.Contains(state.snapshotStatus(),"stale"){t.Fatal("retained snapshot presented as current")}
 _,command:=state.Update(tea.KeyMsg{Type:tea.KeyRunes,Runes:[]rune{'n'}});if command!=nil{t.Fatal("keyboard continuation dispatched after refusal")}
 // A stale reply cannot retire an unrelated current snapshot.
 state.services.Stale=false;old:=refused;old.Ticket.Request++
 updated,_=state.Update(old);state=updated.(operatorModel)
 if state.services.Stale||len(state.receivedFailures)!=1{t.Fatal("retired reply changed current read authority")}
 // A deliberate complete replacement restores ordinary useful navigation.
 page.SnapshotID="replacement";page.Rows=[]api.OperatorService{{ServiceID:"other",DisplayName:"fresh-row"}}
 updated,follow=state.Update(operatorLoaded{Ticket:state.ticket(),Data:page});state=updated.(operatorModel)
 if follow!=nil||state.services.Stale||state.services.Snapshot!="replacement"||state.nextCursor()==nil||len(state.services.Rows)!=1||state.services.Rows[0].ServiceID!="other"||state.errorText!=""{t.Fatal("fresh successful read failed to restore useful navigation")}
}
