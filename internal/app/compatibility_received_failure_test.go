package app

import("context";"encoding/json";"errors";"reflect";"strings";"testing";tea "github.com/charmbracelet/bubbletea";"github.com/service-lasso/service-lasso-tui/internal/api")

type compatibilityOptionalCauseClient struct{fakeClient;cause error}
func(c compatibilityOptionalCauseClient)Capabilities(context.Context)(api.Capabilities,error){return api.Capabilities{},c.cause}
func(c compatibilityOptionalCauseClient)SetupStatus(context.Context)(api.SetupStatus,error){return api.SetupStatus{State:"ready"},nil}
func(c compatibilityOptionalCauseClient)RuntimeIdentity(context.Context)(api.RuntimeIdentity,error){return api.RuntimeIdentity{Status:"running"},nil}
func(c compatibilityOptionalCauseClient)Inbox(context.Context,string)(api.Inbox,error){return api.Inbox{Total:7},nil}

// Source only, UNRUN; received graphs have no finite/durable native admission.
func TestCompatibilityMixedDashboardKeepsUsefulValuesAndOriginal(t *testing.T){
 cause:=errors.New("private-optional-original")
 state:=model{ctx:context.Background(),client:compatibilityOptionalCauseClient{cause:cause}}
 message:=state.refreshDashboard()().(dashboardMsg)
 if message.setup.State!="ready"||message.identity.Status!="running"||message.inbox.Total!=7||!reflect.DeepEqual(message.failures,[]string{"capabilities"})||len(message.causes)!=1||message.causes[0]!=cause{t.Fatal("optional failure erased useful successes or actual original")}
 updated,command:=state.Update(message);current:=updated.(model)
 if command!=nil||current.setup.State!="ready"||current.inbox.Total!=7||len(current.receivedFailures)!=1||!errors.Is(current.receivedFailures[0].Err,cause){t.Fatal("useful dashboard or private original lost")}
 if strings.Contains(current.View(),cause.Error()){t.Fatal("optional original published in UI")}
 encoded,err:=json.Marshal(message);if err!=nil||string(encoded)!="{}"{t.Fatal("dashboard original published in JSON")}
 healthy:=state;healthy.client=compatibilityOptionalCauseClient{};clean:=healthy.refreshDashboard()().(dashboardMsg);result,_:=healthy.Update(clean)
 if len(result.(model).receivedFailures)!=0||len(clean.failures)!=0{t.Fatal("optional nil success invented failure")}
}

func TestCompatibilityStaleMessagesKeepOriginalWithoutAdmission(t *testing.T){
 cause:=errors.New("private-stale-compatibility-original")
 messages:=[]tea.Msg{loadedMsg{epoch:99,err:cause},dashboardMsg{epoch:99,causes:[]error{cause}},historyMsg{epoch:99,err:cause},availabilityMsg{epoch:99,err:cause},lifecyclePreparedMsg{epoch:99,err:cause},storedOperationMsg{epoch:99,stored:&persistedOperation{OperationID:"original"},err:cause},operationMsg{submissionID:99,operation:api.Operation{ID:"returned-original"},attempted:true,cancellation:true,err:cause}}
 for _,message:=range messages{updated,command:=(model{}).Update(message);current:=updated.(model)
  if command!=nil||current.operation!=nil||current.pendingPreview!=nil||len(current.receivedFailures)!=1||len(current.operationFailures)!=0||!errors.Is(current.receivedFailures[0].Err,cause)||!reflect.DeepEqual(current.receivedFailures[0].Message,message){t.Fatal("stale graph lost or stale result admitted")}
  encoded,err:=json.Marshal(current.receivedFailures[0]);if err!=nil||string(encoded)!="{}"{t.Fatal("private compatibility graph published")}
 }
}

type compatibilityCauseStore struct{saveErr,clearErr error;record *persistedOperation;saves,clears int}
func(s *compatibilityCauseStore)Load()(*persistedOperation,error){return s.record,nil}
func(s *compatibilityCauseStore)Save(record persistedOperation)error{s.saves++;if s.saveErr!=nil{return s.saveErr};s.record=&record;return nil}
func(s *compatibilityCauseStore)Clear(persistedOperation)error{s.clears++;return s.clearErr}
func TestCompatibilityStorageFailureKeepsActualOwnerRecordAndOperation(t *testing.T){
 for _,phase:=range []string{"save","clear"}{cause:=errors.New("private-store-original");store:=&compatibilityCauseStore{};if phase=="save"{store.saveErr=cause}else{store.clearErr=cause}
  binding:=selectedOperatorTestContext("a").LegacyDigest();state:=model{operationStore:store,operation:&operationSubmission{id:6,connectionName:"original-profile",reconciliationContext:binding,operation:api.Operation{ID:"original-operation"}}}
  message:=operationMsg{submissionID:6,operation:api.Operation{ID:"original-operation",Outcome:"completed"}}
  updated,command:=state.Update(message);current:=updated.(model)
  if command!=nil||current.operation==nil||len(current.receivedFailures)!=1||current.receivedFailures[0].Err!=cause{t.Fatal("storage failure discarded retained operation or original")}
  retained,ok:=current.receivedFailures[0].Message.(compatibilityStorageFailure);if !ok||retained.Store!=store||retained.Record.OperationID!="original-operation"||retained.Record.ConnectionName!="original-profile"||retained.Record.Binding!=binding||!reflect.DeepEqual(retained.Message,message){t.Fatal("actual store/record/operation graph lost")}
  encoded,err:=json.Marshal(retained);if err!=nil||string(encoded)!="{}"{t.Fatal("storage graph published")}
 }
}
