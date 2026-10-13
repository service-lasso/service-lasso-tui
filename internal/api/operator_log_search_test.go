package api

import (
 "context"
 "encoding/json"
 "net/http"
 "net/http/httptest"
 "strings"
 "testing"
)

// Synthetic HTTP codec source only, UNRUN; this is not a Core log issuer.
func TestOperatorLogSearchUsesExactSelectedQueryAndRetainsPaging(t *testing.T){
 search:="界 + / & ?";cursor:="opaque cursor";requests:=0
 server:=httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter,r *http.Request){
  requests++;q:=r.URL.Query();expected:=search;if requests==2{expected=""}
  if r.Method!="GET"||r.URL.Path!="/api/operator/v2/services/root/logs"||q.Get("search")!=expected||q.Get("stream")!="stdout"||q.Get("limit")!="25"||q.Get("cursor")!=cursor||q.Has("q")||q.Has("query")||q.Has("sourceHandle")||q.Has("follow")||r.Header.Get("x-service-lasso-admin-token")!="source-token"{t.Error("changed exact selected search/source/paging contract")}
  page:=OperatorPage[OperatorLogRow]{SnapshotID:"snapshot",SourceRevision:"source-revision",Rows:[]OperatorLogRow{{RowID:"row",ServiceID:"root",Stream:"stdout",RunID:"run",Time:"2026-10-09T00:00:00.000Z",Text:"public safe line"}},ExpiresAt:"2099-10-09T00:00:00.000Z"}
  json.NewEncoder(w).Encode(OperatorEnvelope[OperatorPage[OperatorLogRow]]{ContractVersion:OperatorContract,RequestID:"request",GeneratedAt:"2026-10-09T00:00:00.000Z",Data:page})
 }));defer server.Close()
 client,err:=NewClient(server.URL,server.Client(),"source-token");if err!=nil{t.Fatal(err)}
 page,err:=client.OperatorSearchLogs(context.Background(),"root","stdout",search,&cursor);if err!=nil||len(page.Rows)!=1{t.Fatal("selected literal query failed codec admission")}
 if _,err=client.OperatorLogs(context.Background(),"root","stdout",&cursor);err!=nil||requests!=2{t.Fatal("original no-search caller changed")}
}
func TestOperatorLogSearchRejectsInvalidUTF8AndOverByteLimitBeforeRequest(t *testing.T){
 requests:=0;server:=httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter,r *http.Request){requests++;w.WriteHeader(500)}));defer server.Close();client,err:=NewClient(server.URL,server.Client(),"source-token");if err!=nil{t.Fatal(err)}
 for _,search:=range []string{strings.Repeat("界",86),string([]byte{0xff}),strings.Repeat("x",257)}{if _,err:=client.OperatorSearchLogs(context.Background(),"root","combined",search,nil);err==nil{t.Fatal("invalid search admitted")}}
 if requests!=0{t.Fatal("invalid search acquired a source")}
}