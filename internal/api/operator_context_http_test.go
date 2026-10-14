package api

import(
 "context"
 "encoding/json"
 "errors"
 "net/http"
 "net/http/httptest"
 "strings"
 "testing"
)

// Actual HTTP client codec cases; this server is not the Core HMAC/auth owner.
func TestOperatorContextOriginalEnvelopeAndTypedErrors(t *testing.T){
 binding:="slrc_"+strings.Repeat("a",64);expected:=OperatorContext{InstanceBinding:binding,WorkspaceBinding:binding,ActorBinding:binding,ClientBinding:binding}
 cases:=[]struct{name string;status int;body any;code string;success bool}{
  {name:"original context",status:200,body:struct{ContractVersion string `json:"contractVersion"`;Context OperatorContext `json:"context"`}{"service-lasso-durable-reconciliation-context.v1",expected},success:true},
  {name:"selected finite permission error",status:403,body:operatorErrorEnvelope{ContractVersion:OperatorContract,RequestID:"request",Error:operatorErrorBody{Code:"permission_denied",MessageKey:"operator.error.permission_denied",Retryability:"never",FieldErrors:[]operatorFieldError{}}},code:"permission_denied"},
  {name:"selected finite storage read retry",status:503,body:operatorErrorEnvelope{ContractVersion:OperatorContract,RequestID:"request",Error:operatorErrorBody{Code:"storage_unavailable",MessageKey:"operator.error.storage_unavailable",Retryability:"read_retry",FieldErrors:[]operatorFieldError{}}},code:"storage_unavailable"},
  {name:"wrong retry cannot become typed authority",status:403,body:operatorErrorEnvelope{ContractVersion:OperatorContract,RequestID:"request",Error:operatorErrorBody{Code:"permission_denied",MessageKey:"operator.error.permission_denied",Retryability:"read_retry",FieldErrors:[]operatorFieldError{}}}},
  {name:"context GET has no field selector grant",status:403,body:operatorErrorEnvelope{ContractVersion:OperatorContract,RequestID:"request",Error:operatorErrorBody{Code:"permission_denied",MessageKey:"operator.error.permission_denied",Retryability:"never",FieldErrors:[]operatorFieldError{{Selector:"actorId",Code:"permission_denied"}}}}},
  {name:"unknown context member",status:200,body:map[string]any{"contractVersion":"service-lasso-durable-reconciliation-context.v1","context":expected,"actorId":"not-authority"}},
  {name:"success bound",status:200,body:map[string]any{"padding":strings.Repeat("x",OperatorContextBytes)}},
  {name:"error bound",status:503,body:map[string]any{"padding":strings.Repeat("x",OperatorErrorBytes)}},
 }
 for _,test:=range cases{t.Run(test.name,func(t *testing.T){server:=httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter,r *http.Request){if r.Method!="GET"||r.URL.Path!="/api/operator/v2/reconciliation-context"||r.URL.RawQuery!=""||r.Header.Get("x-service-lasso-admin-token")!="codec-only-token"{t.Error("context request binding")};w.WriteHeader(test.status);json.NewEncoder(w).Encode(test.body)}));defer server.Close()
  client,err:=NewClient(server.URL,server.Client(),"codec-only-token");if err!=nil{t.Fatal(err)};client.authMode=AuthModeLocalAdmin
  actual,err:=client.OperatorContext(context.Background());var public *OperatorPublicError
  if test.success{if err!=nil||actual!=expected{t.Fatal("original context changed")};return};if err==nil{t.Fatal("invalid response admitted")};if test.code!=""{if !errors.As(err,&public)||public.Code!=test.code||public.RequestID!="request"{t.Fatal("typed producer error lost")}}else if errors.As(err,&public){t.Fatal("invalid envelope gained typed authority")}
 })}
}
