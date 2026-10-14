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

// Synthetic selected v2 HTTP sources, UNRUN. No current Core producer, ABA,
// all-writer authority, durable capture or installed/native witness is supplied.
func metadataBindingFixture(t *testing.T,w http.ResponseWriter,r *http.Request)bool{
 t.Helper();if r.Method!="GET"{return false}
 switch r.URL.Path{
 case "/api/operator/v2/reconciliation-context":json.NewEncoder(w).Encode(struct{ContractVersion string `json:"contractVersion"`;Context api.OperatorContext `json:"context"`}{"service-lasso-durable-reconciliation-context.v1",selectedOperatorTestContext("a")});return true
 case "/api/operator/v2/services/root":_,_,detail:=terminalRefreshFixture(t);json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorDetail]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:detail});return true
 };return false
}
func TestOperatorMetadataCacheRetainsEvidenceButFailedOrForeignReadIsUnknown(t *testing.T){
 _,_,detail:=terminalRefreshFixture(t);favorite:=true;revision:="revision";position:=api.OperatorPosition{X:5,Y:9}
 key:=operatorMetaKey{1,selectedOperatorTestContext("a"),"root",detail.Summary.Generation}
 ready:=operatorMetaObservation{Key:key,Value:api.OperatorMeta{ServiceID:"root",SourceRevision:&revision,Favorite:&favorite,Position:&position,Availability:api.OperatorAvailability{Supported:true,Available:true}}}
 model:=operatorModel{epoch:1,binding:key.Binding,journey:"overview",focus:1,services:operatorPageState[api.OperatorService]{Rows:[]api.OperatorService{detail.Summary}}};model.acceptPageSelection(model.ticket());ready.Ticket=model.ticket();model.acceptMetadata(ready)
 if model.favoriteLabel("root",key.Generation)!="true"||model.layoutLabel("root")!="5,9"{t.Fatal("ready actual preference values not used")}
 model.acceptMetadata(operatorMetaObservation{Key:key,Err:&api.OperatorPublicError{Code:"source_unavailable"}})
 encoded,err:=json.Marshal(model.metadataCurrent["root"]);if err!=nil{t.Fatal(err)};for _,private:=range []string{key.Binding.InstanceBinding,key.Binding.WorkspaceBinding,key.Binding.ActorBinding,key.Binding.ClientBinding,`"Binding"`,`"Ticket"`,`"Err"`,"source_unavailable"}{if strings.Contains(string(encoded),private){t.Fatal("private cache context/error entered JSON")}}
 if model.favoriteLabel("root",key.Generation)!="unknown"||model.layoutLabel("root")!="unknown"||model.metadata[key].Value.Favorite!=ready.Value.Favorite{t.Fatal("failed read donated ready state or erased private evidence")}
 reason:="source_corrupt";model.acceptMetadata(operatorMetaObservation{Key:key,Value:api.OperatorMeta{ServiceID:"root",Availability:api.OperatorAvailability{Supported:true,ReasonCode:&reason}}})
 if model.metadata[key].Value.SourceRevision!=ready.Value.SourceRevision||model.favoriteLabel("root",key.Generation)!="unknown"{t.Fatal("corrupt read erased original revision or became false")}
 model.acceptMetadata(ready);for _,change:=range []string{"epoch","context","generation"}{copy:=model;generation:=key.Generation;switch change{case "epoch":copy.epoch++;case "context":copy.binding=selectedOperatorTestContext("b");case "generation":generation="replacement"};if copy.favoriteLabel("root",generation)!="unknown"{t.Fatal("foreign binding imported preferences")}}
 favorite=false;ready.Value.Favorite=&favorite;model.acceptMetadata(ready)
 updated,_:=model.Update(tea.KeyMsg{Type:tea.KeyRunes,Runes:[]rune{'F'}});model=updated.(operatorModel)
 if !model.favoritesOnly||len(model.overviewServices())!=0{t.Fatal("local favorites filter ignored canonical false")}
 model.acceptMetadata(operatorMetaObservation{Key:key,Err:&api.OperatorPublicError{Code:"source_unavailable"}})
 if len(model.overviewServices())!=1||!strings.Contains(strings.Join(model.rowLines(),"\n"),"favorite=unknown"){t.Fatal("unknown favorite disappeared as false")}
}
func TestOperatorMetadataCompositionRejectsObservedGenerationAndContextDrift(t *testing.T){
 for _,drift:=range []string{"none","generation","context"}{t.Run(drift,func(t *testing.T){
  _,_,detail:=terminalRefreshFixture(t);details,contexts:=0,0;binding:=selectedOperatorTestContext("a");favorite:=true;revision:="revision"
  server:=httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter,r *http.Request){if r.Method!="GET"{t.Error("metadata composition mutated");w.WriteHeader(400);return};switch r.URL.Path{
   case "/api/operator/v2/reconciliation-context":current:=binding;if contexts>0&&drift=="context"{current=selectedOperatorTestContext("b")};contexts++;json.NewEncoder(w).Encode(struct{ContractVersion string `json:"contractVersion"`;Context api.OperatorContext `json:"context"`}{"service-lasso-durable-reconciliation-context.v1",current})
   case "/api/operator/v2/services/root":current:=detail;if details>0&&drift=="generation"{current.Summary.Generation="replacement"};details++;json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorDetail]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:current})
   case "/api/operator/v2/services/root/meta":json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorMeta]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:api.OperatorMeta{ServiceID:"root",SourceRevision:&revision,Favorite:&favorite,Availability:api.OperatorAvailability{Supported:true,Available:true}}})
   default:t.Error("invented metadata route");w.WriteHeader(404)
  }}));defer server.Close();client,err:=api.NewClient(server.URL,server.Client(),"source-token");if err!=nil{t.Fatal(err)}
  observation:=operatorReadMetadata(context.Background(),client,operatorTicket{Connection:1,Binding:binding},"root")
  if drift=="none"{if observation.Err!=nil||observation.Key.Generation!=detail.Summary.Generation||!observation.Value.Availability.Available{t.Fatal("same-binding canonical read rejected")}}else if observation.Err==nil||observation.Value.Favorite!=nil{t.Fatal("observed drift published preference authority")}
 })}
}
func TestOperatorGraphLayoutRequiresOwnCurrentAcquisitionAndRejectsStaleReply(t *testing.T){
 binding:=selectedOperatorTestContext("a");favorite:=true;revision:="revision";position:=api.OperatorPosition{X:7,Y:11}
 model:=operatorModel{epoch:1,selection:2,request:3,journey:"graph",binding:binding,focus:1}
 ticket:=model.ticket();observation:=operatorMetaObservation{Key:operatorMetaKey{1,binding,"root","generation"},Ticket:ticket,Value:api.OperatorMeta{ServiceID:"root",SourceRevision:&revision,Favorite:&favorite,Position:&position,Availability:api.OperatorAvailability{Supported:true,Available:true}}}
 page:=api.OperatorPage[api.OperatorGraphGroup]{SnapshotID:"snapshot",SourceRevision:"graph-revision",ExpiresAt:"2099-10-08T00:00:00.000Z",Rows:[]api.OperatorGraphGroup{{Node:api.OperatorNode{ServiceID:"root",Role:"service",Presence:"present",Availability:api.OperatorAvailability{Supported:true,Available:true}},Edges:[]api.OperatorEdge{},Complete:true}}}
 message:=operatorLoaded{Ticket:ticket,Data:operatorGraphRead{Page:page,Metadata:[]operatorMetaObservation{observation}}}
 updated,_:=model.Update(message);model=updated.(operatorModel)
 if model.layoutLabel("root")!="7,11"{t.Fatal("own current graph acquisition lost layout")}
 model.request++;if model.layoutLabel("root")!="unknown"{t.Fatal("continuation borrowed previous acquisition generation")}
 updated,_=model.Update(message);model=updated.(operatorModel);if model.layoutLabel("root")!="unknown"{t.Fatal("stale reply rebound graph preferences")}
 model.invalidate();if model.metadataCurrent!=nil||len(model.metadata)!=1{t.Fatal("refresh borrowed current preferences or erased private cache evidence")}
 model.binding=selectedOperatorTestContext("b");updated,_=model.Update(message);model=updated.(operatorModel);if model.layoutLabel("root")!="unknown"{t.Fatal("old-context graph reply donated layout")}
}
func TestOperatorOverviewLoadAcceptsMetadataAndUsesOnlyCanonicalFilterQuery(t *testing.T){
 _,discovery,detail:=terminalRefreshFixture(t);discovery.Capabilities[0].ID="read_overview";metaCapability:=discovery.Capabilities[0];metaCapability.ID="service_meta_read";discovery.Capabilities=append(discovery.Capabilities,metaCapability)
 favorite:=false;revision:="revision";metaReads:=0
 server:=httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter,r *http.Request){if metadataBindingFixture(t,w,r){return};switch r.URL.Path{
  case "/api/operator/v2/services":if r.Method!="GET"||r.URL.Query().Get("filter")!="all"||r.URL.Query().Has("favorite")||r.URL.Query().Has("favorites"){t.Error("fabricated favorite query");w.WriteHeader(400);return};page:=api.OperatorPage[api.OperatorService]{SnapshotID:"snapshot",SourceRevision:"list-revision",ExpiresAt:"2099-10-08T00:00:00.000Z",Rows:[]api.OperatorService{detail.Summary}};json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorPage[api.OperatorService]]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:page})
  case "/api/operator/v2/services/root/meta":metaReads++;json.NewEncoder(w).Encode(api.OperatorEnvelope[api.OperatorMeta]{ContractVersion:api.OperatorContract,RequestID:"request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:api.OperatorMeta{ServiceID:"root",SourceRevision:&revision,Favorite:&favorite,Availability:api.OperatorAvailability{Supported:true,Available:true}}})
  default:t.Error("invented route");w.WriteHeader(404)
 }}));defer server.Close();client,err:=api.NewClient(server.URL,server.Client(),"source-token");if err!=nil{t.Fatal(err)}
 model:=operatorModel{client:client,ctx:context.Background(),epoch:1,journey:"overview",focus:1,binding:selectedOperatorTestContext("a"),discovery:&discovery}
 message:=model.load(nil)().(operatorLoaded);if message.Err!=nil{t.Fatal(message.Err)};updated,_:=model.Update(message);model=updated.(operatorModel)
 if metaReads!=1||!model.pageSelectionCurrent()||len(model.services.Rows)!=1||model.favoriteLabel("root",detail.Summary.Generation)!="false"{t.Fatal("actual wrapper/reducer metadata acceptance failed")}
 updated,_=model.Update(tea.KeyMsg{Type:tea.KeyRunes,Runes:[]rune{'F'}});model=updated.(operatorModel);if len(model.overviewServices())!=0||metaReads!=1{t.Fatal("local filter dispatched a read or ignored false")}
}
