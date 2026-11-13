package app

import("context";"encoding/json";"errors";"io";"net/http";"reflect";"strings";"testing";"github.com/service-lasso/service-lasso-tui/internal/api")

// Source only, UNRUN. Partial observations supply no ready metadata authority.
func TestMetadataLoadKeepsPartialReadGraphOnFailedSecondDetail(t *testing.T){
 _,discovery,detail:=terminalRefreshFixture(t);binding:=selectedOperatorTestContext("a");cause:=errors.New("private-second-detail-original")
 reason:="source_unavailable";meta:=api.OperatorMeta{ServiceID:"root",Availability:api.OperatorAvailability{Supported:true,ReasonCode:&reason}}
 if !meta.Valid(){t.Fatal("unavailable metadata fixture violates codec")}
 discovery.Capabilities=append(discovery.Capabilities,api.OperatorCapability{ID:"service_meta_read",Availability:api.OperatorAvailability{Supported:true,Available:true},RequiredProfile:"observer",RequiredScopes:[]string{"service-lasso:read"},ConfirmationPolicy:"none",BindingRequirements:[]string{}})
 contexts,details,metadata:=0,0,0
 transport:=terminalRefreshTransport(func(request *http.Request)(*http.Response,error){
  if request.Method!="GET"{t.Fatal("metadata read dispatched mutation")};var value any
  switch request.URL.Path{
  case "/api/operator/v2/reconciliation-context":contexts++;value=struct{ContractVersion string `json:"contractVersion"`;Context api.OperatorContext `json:"context"`}{"service-lasso-durable-reconciliation-context.v1",binding}
  case "/api/operator/v2/services/root":details++;if details==2{return nil,cause};value=api.OperatorEnvelope[api.OperatorDetail]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:detail}
  case "/api/operator/v2/services/root/meta":metadata++;value=api.OperatorEnvelope[api.OperatorMeta]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:meta}
  default:t.Fatal("metadata read guessed route")}
  raw,err:=json.Marshal(value);if err!=nil{t.Fatal(err)};return &http.Response{StatusCode:200,Header:http.Header{},Body:io.NopCloser(strings.NewReader(string(raw)))},nil
 })
 client,err:=api.NewClient("http://127.0.0.1:1",&http.Client{Transport:transport},"source-token");if err!=nil{t.Fatal(err)}
 state:=operatorModel{client:client,ctx:context.Background(),epoch:1,binding:binding,service:"root",journey:"metadata",discovery:&discovery}
 command:=state.load(nil);if command==nil{t.Fatal("admitted metadata read absent")};message:=command().(operatorLoaded)
 read,ok:=message.Data.(operatorMetadataRead);if !ok||!errors.Is(message.Err,cause)||!errors.Is(read.Observation.Err,cause)||read.Observation.Ticket!=message.Ticket||read.Observation.Before==nil||!reflect.DeepEqual(*read.Observation.Before,detail)||read.Observation.Candidate==nil||!reflect.DeepEqual(*read.Observation.Candidate,meta)||read.Observation.After!=nil{t.Fatal("failed load lost actual partial observations or original")}
 updated,follow:=state.Update(message);current:=updated.(operatorModel)
 if follow!=nil||current.meta!=nil||len(current.metadataCurrent)!=0||len(current.receivedFailures)!=1{t.Fatal("failed metadata became ready authority or lost retained message")}
 retained:=current.receivedFailures[0].Message.(operatorLoaded).Data.(operatorMetadataRead)
 if retained.Observation.Before!=read.Observation.Before||retained.Observation.Candidate!=read.Observation.Candidate{t.Fatal("actual private observation owners replaced")}
 encoded,err:=json.Marshal(current.receivedFailures);if err!=nil||string(encoded)!="[{}]"{t.Fatal("received graph published")}
 encoded,err=json.Marshal(read.Observation);if err!=nil||strings.Contains(string(encoded),"Before")||strings.Contains(string(encoded),"Candidate")||strings.Contains(string(encoded),cause.Error()){t.Fatal("partial private graph published")}
 if contexts!=1||details!=2||metadata!=1{t.Fatal("original Detail/Meta/failed Detail path not reached")}
}
