package app

import("encoding/json";"errors";"reflect";"testing";tea "github.com/charmbracelet/bubbletea";"github.com/service-lasso/service-lasso-tui/internal/api")

// Source cases only, UNRUN; private message retention is no native capture.
func TestReceivedFailuresRetainActualStaleMessageWithoutAdmission(t *testing.T){
 operation,_,detail:=terminalRefreshFixture(t);binding:=selectedOperatorTestContext("a");cause:=errors.New("private-received-original");warning:=errors.New("private-restoration-original");ticket:=operatorTicket{Connection:99,Service:"root",Journey:"detail",Binding:binding}
 messages:=[]tea.Msg{
  operatorBooted{Epoch:99,Context:binding,Restore:operatorRestoration{Raw:[]byte("private-retained-record"),Warning:warning},Err:cause},
  operatorLoaded{Ticket:ticket,Data:detail,Err:cause},
  operatorPreviewed{Ticket:ticket,Preview:api.OperatorPreview{ServiceID:"root"},Err:cause},
  operatorDraftBinding{Ticket:ticket,Action:"service_start",Err:cause},
  operatorSubmitted{Epoch:99,Binding:binding,Expected:[]byte("private-preimage"),Submission:api.OperatorSubmission{Operation:operation},Err:cause,Ticket:ticket},
  operatorCancelled{Epoch:99,ID:operation.OperationID,Binding:binding,Dispatched:true,Err:cause},
  operatorDirectResult{Ticket:ticket,Binding:binding,Dispatched:true,Err:cause},
  operatorPoll{Epoch:99,ID:operation.OperationID,Readback:api.OperatorReadback{Operation:operation},Binding:binding,Err:cause},
  operatorTerminalRefreshed{Epoch:99,Binding:binding,Original:operation,Ticket:ticket,Detail:detail,Err:cause},
 }
 for _,message:=range messages{
  model:=operatorModel{epoch:1,service:"current",journey:"detail",binding:binding,saved:[]byte("current-record")}
  updated,command:=model.Update(message);current:=updated.(operatorModel)
  if command!=nil||current.operation!=nil||current.detail!=nil||current.draft.Sent||current.cancelAttempted||current.service!="current"||string(current.saved)!="current-record"||len(current.receivedFailures)!=1{t.Fatal("stale failure admitted data, dispatched work or lost record")}
  retained:=current.receivedFailures[0]
  if retained.Err!=cause||!errors.Is(retained.Err,cause)||!reflect.DeepEqual(retained.Message,message){t.Fatal("actual message/cause/binding/dispatch graph lost")}
  encoded,err:=json.Marshal(retained);if err!=nil||string(encoded)!="{}"{t.Fatal("private received graph published")}
  current.invalidate();if len(current.receivedFailures)!=1||current.receivedFailures[0].Err!=cause{t.Fatal("navigation erased original")}
 }
}

func TestUsefulCurrentDetailPublishesWithoutSyntheticReceivedFailure(t *testing.T){
 _,_,detail:=terminalRefreshFixture(t);binding:=selectedOperatorTestContext("a")
 model:=operatorModel{epoch:1,service:"root",journey:"detail",binding:binding,loading:true}
 updated,command:=model.Update(operatorLoaded{Ticket:model.ticket(),Data:detail});current:=updated.(operatorModel)
 if command!=nil||current.detail==nil||!reflect.DeepEqual(*current.detail,detail)||current.loading||len(current.receivedFailures)!=0||!current.pageSelectionCurrent(){t.Fatal("useful admitted detail lost or synthetic failure created")}
}

func TestStaleOverviewMetadataOriginalSurvivesPartialReadMessage(t *testing.T){
 cause:=errors.New("private-metadata-observation-original");observation:=operatorMetaObservation{Err:cause}
 for _,data:=range []any{operatorOverviewRead{Metadata:[]operatorMetaObservation{observation}},operatorGraphRead{Metadata:[]operatorMetaObservation{observation}},operatorMetadataRead{Observation:observation}}{message:=operatorLoaded{Ticket:operatorTicket{Connection:99},Data:data}
 updated,command:=(operatorModel{epoch:1,journey:"overview"}).Update(message);current:=updated.(operatorModel)
 if command!=nil||len(current.receivedFailures)!=1||!errors.Is(current.receivedFailures[0].Err,cause)||!reflect.DeepEqual(current.receivedFailures[0].Message,message)||current.metadataCurrent!=nil||len(current.services.Rows)!=0{t.Fatal("partial metadata original lost or stale page admitted")}
 encoded,err:=json.Marshal(current.receivedFailures[0]);if err!=nil||string(encoded)!="{}"{t.Fatal("partial original published")}
 }
}

func TestUsefulOverviewSurvivesMetadataFailureAndNilObservationsCreateNoFailure(t *testing.T){
 _,_,detail:=terminalRefreshFixture(t);binding:=selectedOperatorTestContext("a");cause:=errors.New("private-partial-original");reason:="source_unavailable"
 for _,failure:=range []error{cause,nil}{
  model:=operatorModel{epoch:1,journey:"overview",binding:binding,loading:true};ticket:=model.ticket()
  meta:=api.OperatorMeta{ServiceID:"root",Availability:api.OperatorAvailability{Supported:true,ReasonCode:&reason}};if !meta.Valid(){t.Fatal("unavailable metadata fixture invalid")}
  observation:=operatorMetaObservation{Key:operatorMetaKey{Connection:1,Binding:binding,Service:"root",Generation:detail.Summary.Generation},Ticket:ticket,Value:meta,Err:failure}
  page:=api.OperatorPage[api.OperatorService]{SnapshotID:"snapshot",SourceRevision:"list-revision",ExpiresAt:"2099-10-08T00:00:00.000Z",Rows:[]api.OperatorService{detail.Summary}};if !page.ValidHeader(100){t.Fatal("useful page fixture invalid")}
  updated,command:=model.Update(operatorLoaded{Ticket:ticket,Data:operatorOverviewRead{Page:page,Metadata:[]operatorMetaObservation{observation}}});current:=updated.(operatorModel)
  if command!=nil||current.loading||len(current.services.Rows)!=1||current.services.Rows[0].ServiceID!="root"||!current.pageSelectionCurrent()||current.favoriteLabel("root",detail.Summary.Generation)!="unknown"{t.Fatal("partial useful page denied or unknown metadata became authority")}
  if failure==nil{if len(current.receivedFailures)!=0{t.Fatal("nil observation invented failure")}}else if len(current.receivedFailures)!=1||!errors.Is(current.receivedFailures[0].Err,cause){t.Fatal("partial actual original lost")}
 }
}
