package app

import (
	"context"
	"errors"
	"os"
	"path/filepath"
	"strings"
	"testing"
	"time"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/service-lasso/service-lasso-tui/internal/api"
)

func currentOperation() api.Operation { return api.Operation{ID:"mcp-operation-12345678-1234-1234-1234-123456789abc",Action:"service_start",Status:"running",Phase:"preflight",Ownership:"own",Targets:[]string{"echo"}} }
func currentPreview() *api.LifecyclePreview { return &api.LifecyclePreview{Action:"start",ServiceID:"echo",Targets:[]string{"dependency","echo"},Effects:[]string{"Start service."}} }

func runBatch(t *testing.T, command tea.Cmd) []tea.Msg {
	t.Helper(); if command==nil { return nil }; message:=command(); batch,ok:=message.(tea.BatchMsg); if !ok{return []tea.Msg{message}}; var result []tea.Msg
	for _, child:=range batch {if child!=nil {result=append(result,child())}};return result
}

func TestLostAcceptedSubmitResponseHasNoIDNoRecoveryReadsAndNoReplay(t *testing.T){
	client:=&recordingDurableClient{submitErr:errors.New("SECRET https://private/token /private/path \x1b[2J")}
	m:=preparedModel(t,client); m.operationStore=&memoryOperationStore{}
	updated,submit:=m.Update(key('y')); result:=submit().(operationMsg)
	if !result.attempted {t.Fatal("actual submit boundary not recorded")}
	updated,_=updated.(model).Update(result); got:=updated.(model)
	if got.operation==nil || !got.operation.uncertainWithoutID || got.operation.operation.ID!="" {t.Fatal("lost response invented identity or discarded uncertainty")}
	for _,sentinel:=range []string{"SECRET","private","\x1b"}{if strings.Contains(got.View(),sentinel){t.Fatal("uncertain error leaked")}}
	if !strings.Contains(got.View(),"no operation ID") || !strings.Contains(got.View(),"Core correlation support") {t.Fatal("no-ID state promises recovery")}
	for _,input:=range []rune{'y','s','z','r','r'} {next,command:=got.Update(key(input));got=next.(model);for _,message:=range runBatch(t,command){next,_=got.Update(message);got=next.(model)}}
	_,_,submits,reads,cancels:=client.calls();if submits!=1 || reads!=0 || cancels!=0 {t.Fatalf("after uncertainty submit/read/cancel=%d/%d/%d",submits,reads,cancels)}
	if got.operationStore.(*memoryOperationStore).value!=nil {t.Fatal("no-ID request persisted")}
}

func TestConfirmationBoundaryRechecksWithoutChangingPreview(t *testing.T){
	client:=&recordingDurableClient{availability:[]api.LifecycleAvailability{{Action:"start",Available:false,Reason:"insufficient_scope",RequiresConfirmation:true}}}
	m:=preparedModel(t,client);updated,command:=m.Update(key('y'));message:=command().(operationMsg)
	if message.attempted || message.err==nil {t.Fatal("availability denial reached mutation")}
	updated,_=updated.(model).Update(message);if updated.(model).operation!=nil {t.Fatal("never-submitted action classified as lost accepted response")}
	availability,previews,submits,_,_:=client.calls();if availability!=1 || previews!=0 || submits!=0 {t.Fatal("boundary recheck changed preview or posted mutation")}
}

func TestHistorySelectionGenerationAndProfileRace(t *testing.T){
	m:=New(fakeClient{},context.Background()).(model);m.screen=detailScreen;m.services=[]api.Service{{ID:"A"},{ID:"B"}};m.selectedServiceID="A"
	_ = m.loadHistory();firstGeneration:=m.historyGeneration
	m.selectedServiceID="B";_ = m.loadHistory();if !m.historyLoading || m.history.Entries!=0 {t.Fatal("history not reset for B")}
	for _,late:=range []historyMsg{
		{epoch:m.connectionEpoch,generation:firstGeneration,serviceID:"A",history:api.HealthHistory{ServiceID:"A",Entries:9}},
		{epoch:m.connectionEpoch,generation:m.historyGeneration-1,serviceID:"B",history:api.HealthHistory{ServiceID:"B",Entries:9}},
		{epoch:m.connectionEpoch+1,generation:m.historyGeneration,serviceID:"B",history:api.HealthHistory{ServiceID:"B",Entries:9}},
	}{updated,_:=m.Update(late);if updated.(model).history.Entries!=0 || !updated.(model).historyLoading {t.Fatal("late/foreign history replaced B")}}
	updated,_:=m.Update(historyMsg{epoch:m.connectionEpoch,generation:m.historyGeneration,serviceID:"B",history:api.HealthHistory{ServiceID:"wrong",Entries:9}})
	if !updated.(model).historyUnavailable {t.Fatal("wrong response service admitted")}
	updated,_=m.Update(historyMsg{epoch:m.connectionEpoch,generation:m.historyGeneration,serviceID:"B",history:api.HealthHistory{ServiceID:"B",Entries:2}})
	if updated.(model).historyLoading || updated.(model).history.Entries!=2 {t.Fatal("current B history rejected")}
}

func TestAvailabilityReasonsLoadingAndLateSelectedReply(t *testing.T){
	m:=New(fakeClient{},context.Background()).(model);m.screen=detailScreen;m.services=[]api.Service{{ID:"echo"}};m.selectedServiceID="echo";m.availabilityService="echo";m.availabilityGeneration=2;m.availabilityLoading=true
	for _,late:=range []availabilityMsg{{epoch:1,generation:2,serviceID:"echo"},{epoch:0,generation:1,serviceID:"echo"},{epoch:0,generation:2,serviceID:"other"}} {updated,_:=m.Update(late);if !updated.(model).availabilityLoading {t.Fatal("late availability accepted")}}
	updated,_:=m.Update(availabilityMsg{epoch:0,generation:2,serviceID:"echo",actions:[]api.LifecycleAvailability{{Action:"start",Available:true,RequiresConfirmation:true},{Action:"stop",Reason:"insufficient_scope"},{Action:"reload",Reason:"SECRET/private/path"}}})
	view:=updated.(model).View();if !strings.Contains(view,"start: available") || !strings.Contains(view,"stop: disabled: permission denied") || strings.Contains(view,"SECRET") || strings.Contains(view,"private") {t.Fatalf("projection=%s",view)}
}

func TestRefreshGenerationFrozenPreviewAndPostTerminalRead(t *testing.T){
	client:=&recordingDurableClient{operation:currentOperation()};m:=New(client,context.Background()).(model);m.services=[]api.Service{{ID:"echo"}};m.selectedServiceID="echo";m.refreshGeneration=2
	updated,_:=m.Update(loadedMsg{epoch:0,generation:1,services:[]api.Service{{ID:"other"}}});if updated.(model).selectedServiceID!="echo" {t.Fatal("old refresh replaced selected service")}
	m.pendingAction="start";m.pendingPreview=currentPreview();updated,_=m.Update(loadedMsg{epoch:0,generation:2,services:[]api.Service{{ID:"other"}}});if updated.(model).selectedServiceID!="echo" {t.Fatal("refresh changed frozen target")}
	m.pendingAction="";m.pendingPreview=nil;m.operationStore=&memoryOperationStore{};m.operation=&operationSubmission{id:3,client:client,operation:currentOperation(),preview:currentPreview()}
	terminal:=currentOperation();terminal.Status,terminal.Outcome,terminal.Progress="succeeded","succeeded",100
	updated,command:=m.Update(operationMsg{submissionID:3,operation:terminal});got:=updated.(model)
	if command==nil || got.operation!=nil || !got.stale || !got.loading || got.refreshGeneration!=3 {t.Fatal("terminal did not require authoritative service refresh")}
	_ = command();_,_,submits,_,cancels:=client.calls();if submits!=0 || cancels!=0 {t.Fatal("terminal refresh mutated")}
	m.lastSnapshot=time.Now().Add(-time.Minute);if !strings.Contains(m.View(),"(stale)") {t.Fatal("old snapshot called current")}
}

func TestCurrentSubmissionRejectsForeignReadbackAndAdmitsPreviewExpansion(t *testing.T){
	for _,fault:=range []string{"action","target","ownership","id","context"} {t.Run(fault,func(t *testing.T){
		m:=New(durableFake{},context.Background()).(model);m.operationStore=&memoryOperationStore{};m.operation=&operationSubmission{id:3,client:durableFake{},operation:currentOperation(),preview:currentPreview(),reconciliationContext:"context-A"}
		op:=currentOperation();binding:="context-A";switch fault {case "action":op.Action="service_stop";case "target":op.Targets=[]string{"other"};case "ownership":op.Ownership="other";case "id":op.ID="different";case "context":binding="context-B"}
		updated,command:=m.Update(operationMsg{submissionID:3,operation:op,binding:binding});if updated.(model).err==nil || command!=nil || updated.(model).operation.operation.Action!="service_start" {t.Fatal("foreign tuple accepted")}
	})}
	m:=New(durableFake{},context.Background()).(model);m.operationStore=&memoryOperationStore{};m.operation=&operationSubmission{id:3,client:durableFake{},operation:currentOperation(),preview:currentPreview()};expanded:=currentOperation();expanded.Targets=[]string{"dependency","echo"};updated,_:=m.Update(operationMsg{submissionID:3,operation:expanded});if updated.(model).err!=nil {t.Fatal("frozen preview dependency expansion denied")}
}

func TestRestoredReadTargetsAreServerReportedAndCancellationClosed(t *testing.T){
	m:=New(durableFake{},context.Background()).(model);m.screen=detailScreen;m.operationStore=&memoryOperationStore{};m.operation=&operationSubmission{id:4,client:durableFake{},operation:api.Operation{ID:currentOperation().ID}}
	for _,targets:=range [][]string{{"echo"},{"dependency","echo"},{"dependency"},{}} {
		op:=currentOperation();op.Targets=targets;if len(targets)==0 {op.Status,op.Outcome,op.Progress="skipped","skipped",100}
		updated,_:=m.Update(operationMsg{submissionID:4,operation:op});m=updated.(model);if m.err!=nil {t.Fatal("genuine server target transition denied")};if m.operation==nil {break}
		if !strings.Contains(m.View(),"correlation is unproved") {t.Fatal("restoration claims preview correlation")}
		m.operation.operation.CancellationSupported=true;_,cancel:=m.Update(key('z'));if cancel!=nil {t.Fatal("uncorrelated restored cancellation posted")}
	}
}

func TestCancellationErrorCannotReplayAndUnsupportedNeverPosts(t *testing.T){
	for _,supported:=range []bool{false,true} {
		client:=&recordingDurableClient{cancelErr:errors.New("SECRET /private/path")};op:=currentOperation();op.CancellationSupported=supported
		m:=New(client,context.Background()).(model);m.operation=&operationSubmission{id:4,client:client,operation:op,preview:currentPreview()}
		updated,command:=m.Update(key('z'));if !supported {if command!=nil {t.Fatal("unsupported cancellation posted")};continue};if command==nil {t.Fatal("advertised cancellation unavailable")}
		updated,_=updated.(model).Update(command());_,retry:=updated.(model).Update(key('z'));if retry!=nil {t.Fatal("uncertain cancel replayed")};_,_,_,_,cancels:=client.calls();if cancels!=1 {t.Fatal("cancellation count not one")}
	}
}

func TestStoredMetadataIOFailureHasNoPrivatePath(t *testing.T){
	root:=t.TempDir();blocked:=filepath.Join(root,"SECRET-private-file");if err:=os.WriteFile(blocked,[]byte("owned"),0600);err!=nil{t.Fatal(err)}
	store:=fileOperationStore{path:filepath.Join(blocked,"record.json")};err:=store.Save(persistedOperation{Version:2,OperationID:"opaque",ConnectionName:"local",Binding:selectedOperatorTestContext("a").LegacyDigest()});if err==nil || strings.Contains(err.Error(),root) || strings.Contains(err.Error(),"SECRET") {t.Fatalf("storage error=%v",err)}
}

func TestSameServicePreparationFromCancelledGenerationCannotReplacePreview(t *testing.T){
	client:=&recordingDurableClient{availability:[]api.LifecycleAvailability{{Action:"start",Available:true,RequiresConfirmation:true}},preview:*currentPreview()}
	m:=New(client,context.Background()).(model);m.services=[]api.Service{{ID:"echo"}};m.selectedServiceID="echo";m.screen=detailScreen
	first:=m.beginPendingAction("start");firstMessage:=first();updated,_:=m.Update(tea.KeyMsg{Type:tea.KeyEsc});m=updated.(model);second:=m.beginPendingAction("start");updated,_=m.Update(firstMessage);if !updated.(model).preparingAction || updated.(model).pendingPreview!=nil {t.Fatal("old preview used after cancellation")};updated,_=updated.(model).Update(second());if updated.(model).pendingPreview==nil {t.Fatal("current preparation rejected")}
}

func TestKnownContextChangeBlocksReadAndCancelWithoutMutation(t *testing.T){
	client:=&recordingDurableClient{reconciliation:selectedOperatorTestContext("b").LegacyDigest()}
	submission:=operationSubmission{id:1,client:client,operation:currentOperation(),preview:currentPreview(),reconciliationContext:selectedOperatorTestContext("a").LegacyDigest()}
	for _,command:=range []tea.Cmd{readOperation(submission,context.Background()),cancelOperation(submission,context.Background())}{message:=command().(operationMsg);if message.err==nil {t.Fatal("context mismatch admitted")}}
	_,_,submits,reads,cancels:=client.calls();if submits!=0 || reads!=0 || cancels!=0 {t.Fatal("changed actor used original operation")}
	m:=preparedModel(t,client);m.pendingPreview.ReconciliationContext=selectedOperatorTestContext("a").LegacyDigest();updated,command:=m.Update(key('y'));message:=command().(operationMsg);if message.attempted || message.err==nil {t.Fatal("changed context posted frozen confirmation")};_,_=updated.(model).Update(message)
}

func TestLateOperationReadGenerationCannotReplaceCurrentProgress(t *testing.T){
	m:=New(durableFake{},context.Background()).(model);m.operationStore=&memoryOperationStore{};m.operation=&operationSubmission{id:1,client:durableFake{},operation:currentOperation(),preview:currentPreview(),readGeneration:2}
	old:=currentOperation();old.Phase="old_phase";updated,command:=m.Update(operationMsg{submissionID:1,generation:1,operation:old});if command!=nil || updated.(model).operation.operation.Phase!="preflight" {t.Fatal("late operation read overwrote progress")}
}

func TestCoreInterruptedExpiryIsUncertainAndNeverReplayed(t *testing.T){
	m:=New(durableFake{},context.Background()).(model);m.operationStore=&memoryOperationStore{};m.operation=&operationSubmission{id:1,client:durableFake{},operation:currentOperation(),preview:currentPreview()}
	op:=currentOperation();op.Status,op.Outcome,op.Phase,op.Progress="failed","failed","interrupted",100
	updated,_:=m.Update(operationMsg{submissionID:1,operation:op});if !strings.Contains(updated.(model).lastResult,"authoritative mutation outcome unavailable") || !strings.Contains(updated.(model).lastResult,"failed/interrupted") {t.Fatal("expiry uncertainty relabelled as proven mutation failure")}
}
