package api

import (
 "bytes"
 "context"
 "encoding/json"
 "net/http"
 "net/http/httptest"
 "testing"
)

// HTTP response-codec source regressions. This fixture is not Core, an
// authenticated issuer, a protected installed archive or native acceptance.
func TestOperatorGraphHTTPResponseCodecAdmission(t *testing.T){
 edge:=func(from,kind,state string)OperatorEdge{return OperatorEdge{FromServiceID:from,ToServiceID:"destination",Kind:kind,State:state}}
 cursor:="cursor /? opaque";next:="next"
 cases:=[]struct{name string;edges []OperatorEdge;offset,total int;complete bool;cursor,next *string;unknown bool;want bool}{
  {name:"same source distinct sorted kinds",edges:[]OperatorEdge{edge("source","provider","satisfied"),edge("source","service","blocked")},total:2,complete:true,want:true},
  {name:"differing state duplicate",edges:[]OperatorEdge{edge("source","service","satisfied"),edge("source","service","blocked")},total:2,complete:true},
  {name:"reverse kind",edges:[]OperatorEdge{edge("source","service","satisfied"),edge("source","provider","satisfied")},total:2,complete:true},
  {name:"reverse source",edges:[]OperatorEdge{edge("z","service","satisfied"),edge("a","service","satisfied")},total:2,complete:true},
  {name:"isolated complete node",edges:[]OperatorEdge{},complete:true,want:true},
  {name:"initial split fragment",edges:[]OperatorEdge{edge("source","service","satisfied")},total:2,next:&next,want:true},
  {name:"continuation final fragment",edges:[]OperatorEdge{edge("source","service","satisfied")},offset:16,total:17,complete:true,cursor:&cursor,want:true},
  {name:"empty nonisolated fragment",edges:[]OperatorEdge{},total:1,next:&next},
  {name:"unknown group member",edges:[]OperatorEdge{},complete:true,unknown:true},
 }
 for _,test:=range cases{t.Run(test.name,func(t *testing.T){
  group:=OperatorGraphGroup{Node:OperatorNode{ServiceID:"destination",Role:"service",Presence:"present",Availability:OperatorAvailability{Supported:true,Available:true}},Edges:test.edges,EdgeOffset:test.offset,EdgeTotal:test.total,Complete:test.complete}
  page:=OperatorPage[OperatorGraphGroup]{SnapshotID:"snapshot",SourceRevision:"revision",Rows:[]OperatorGraphGroup{group},NextCursor:test.next,ExpiresAt:"2099-10-08T00:00:00.000Z"}
  envelope:=OperatorEnvelope[OperatorPage[OperatorGraphGroup]]{ContractVersion:OperatorContract,RequestID:"codec-request",GeneratedAt:"2026-10-08T00:00:00.000Z",Data:page}
  raw,err:=json.Marshal(envelope);if err!=nil{t.Fatal(err)}
  if test.unknown{raw=bytes.Replace(raw,[]byte(`"edgeTotal":0`),[]byte(`"edgeTotal":0,"extra":true`),1)}
  server:=httptest.NewServer(http.HandlerFunc(func(writer http.ResponseWriter,request *http.Request){
   if request.Method!="GET"||request.URL.Path!="/api/operator/v2/graph"||request.URL.Query().Get("limit")!="10"{t.Error("wrong actual graph route/limit")}
   expected:="";if test.cursor!=nil{expected=*test.cursor};if request.URL.Query().Get("cursor")!=expected{t.Error("opaque cursor changed")}
   writer.Header().Set("Content-Type","application/json");if _,err:=writer.Write(raw);err!=nil{t.Error(err)}
  }));defer server.Close()
  client,err:=NewClient(server.URL,server.Client(),"codec-only-token");if err!=nil{t.Fatal(err)}
  result,err:=client.OperatorGraph(context.Background(),test.cursor)
  if (err==nil)!=test.want{t.Fatalf("graph admission mismatch: want %v, error %v",test.want,err)}
  if test.want{
   if len(result.Rows)!=1||len(result.Rows[0].Edges)!=len(test.edges)||result.Rows[0].EdgeOffset!=test.offset||result.Rows[0].EdgeTotal!=test.total||result.Rows[0].Complete!=test.complete||result.Rows[0].Node!=group.Node{t.Fatal("admitted fragment changed")}
   for i,original:=range test.edges{if result.Rows[0].Edges[i]!=original{t.Fatal("admitted edge identity/state changed")}}
  }
 })}
}
