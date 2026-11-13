package app

import (
 "bytes"
 "context"
 "encoding/json"
 "os"
 "path/filepath"
 "testing"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// File-store/source fixtures exercise ownership; they confer no native or
// authenticated issuer qualification.
func TestLegacySavePreservesDifferentOwnerAndExactOwnBytes(t *testing.T) {
 a:=persistedOperation{Version:2,OperationID:"operation-A",ConnectionName:"profile",Binding:selectedOperatorTestContext("a").LegacyDigest()}
 variants:=[]persistedOperation{a,a,a}
 variants[0].OperationID="operation-B"
 variants[1].ConnectionName="other-profile"
 variants[2].Binding=selectedOperatorTestContext("b").LegacyDigest()
 for _,b:=range variants {
  store:=fileOperationStore{path:filepath.Join(t.TempDir(),"operation.json")}
  raw,_:=json.Marshal(b)
  raw=append(append([]byte(" \n"),raw...),[]byte("\n ")...)
  if err:=os.WriteFile(store.path,raw,0600);err!=nil{t.Fatal(err)}
  if err:=store.Save(a);err==nil{t.Fatal("different owner replaced")}
  after,err:=os.ReadFile(store.path);if err!=nil||!bytes.Equal(after,raw){t.Fatal("different raw owner changed",err)}
  if err:=store.Save(b);err!=nil{t.Fatal("exact owner no-op denied",err)}
  after,err=os.ReadFile(store.path);if err!=nil||!bytes.Equal(after,raw){t.Fatal("no-op canonicalized original bytes",err)}
 }
}

func TestLegacySaveFirstPublicationAndLateTerminalReadback(t *testing.T) {
 store:=fileOperationStore{path:filepath.Join(t.TempDir(),"operation.json")}
 a:=persistedOperation{Version:2,OperationID:"operation-A",ConnectionName:"profile",Binding:selectedOperatorTestContext("a").LegacyDigest()}
 b:=a;b.OperationID="operation-B"
 if err:=store.Save(a);err!=nil{logRecordFailure(t,err);t.Fatal(err)}
 if err:=store.Clear(a);err!=nil{t.Fatal(err)}
 if err:=store.Save(b);err!=nil{t.Fatal(err)}
 original,err:=os.ReadFile(store.path);if err!=nil{t.Fatal(err)}
 m:=New(durableFake{},context.Background()).(model)
 m.operationStore=store
 m.operation=&operationSubmission{id:7,client:durableFake{},connectionName:a.ConnectionName,reconciliationContext:a.Binding,operation:api.Operation{ID:a.OperationID}}
 updated,command:=m.Update(operationMsg{submissionID:7,operation:api.Operation{ID:a.OperationID,Outcome:"completed"}})
 got:=updated.(model)
 if got.err==nil||got.operation==nil||got.operation.operation.ID!=a.OperationID||command!=nil{t.Fatal("late terminal persistence failure hidden or replay scheduled")}
 after,err:=os.ReadFile(store.path);if err!=nil||!bytes.Equal(after,original){t.Fatal("late A readback overwrote or cleared B",err)}
 if err:=store.Clear(a);err==nil{t.Fatal("late clear admitted")}
 after,err=os.ReadFile(store.path);if err!=nil||!bytes.Equal(after,original){t.Fatal("late clear changed B",err)}
}

func TestLegacySaveDoesNotReplaceMigratedRecord(t *testing.T) {
 store:=fileOperationStore{path:filepath.Join(t.TempDir(),"operation.json")}
 v3:=api.OperatorSavedV3{Version:3,OperationID:"operation-B",ServiceID:"service",ActionID:"service_start",Context:selectedOperatorTestContext("a")}
 raw,err:=json.Marshal(v3);if err!=nil{t.Fatal(err)}
 if _,err:=api.DecodeOperatorSavedV3(raw);err!=nil{t.Fatal("invalid fixture",err)}
 if err:=os.WriteFile(store.path,raw,0600);err!=nil{t.Fatal(err)}
 a:=persistedOperation{Version:2,OperationID:"operation-A",ConnectionName:"profile",Binding:selectedOperatorTestContext("a").LegacyDigest()}
 if err:=store.Save(a);err==nil{t.Fatal("migrated owner replaced")}
 after,err:=os.ReadFile(store.path);if err!=nil||!bytes.Equal(after,raw){t.Fatal("migrated raw bytes changed",err)}
}
