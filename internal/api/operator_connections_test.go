package api

import(
 "encoding/json"
 "errors"
 "os"
 "path/filepath"
 "testing"
)

func TestOperatorReaderAdmissionReportsItsOwnRequirement(t *testing.T){
 _,err:=NewOperatorClientWithAuth("https://core.example.invalid",nil,"codec-only",AuthModeOAuthBearer,"service-lasso:lifecycle:write")
 var configured *ConfigurationError
 if !errors.As(err,&configured)||configured.Kind!=ConfigurationErrorOperatorReaderAdmission||configured.Error()!="non-loopback Service Lasso API requires explicit oauth-bearer general-read scope"{t.Fatal("v2 reader diagnosis requested unrelated lifecycle authority")}
 _,err=NewClientWithAuth("https://core.example.invalid",nil,"codec-only",AuthModeOAuthBearer,"service-lasso:read")
 if !errors.As(err,&configured)||configured.Kind!=ConfigurationErrorRemoteProfileAdmission||configured.Error()!="non-loopback Service Lasso API requires explicit oauth-bearer lifecycle scopes"{t.Fatal("legacy admission diagnosis changed")}
}

// Transport/profile admission source cases, not OAuth token or Core permission
// proof. Scope metadata cannot grant discovery, reads or mutations on a server.
func operatorReaderConfig(t *testing.T,profiles map[string]ConnectionProfile)string{
 t.Helper();raw,err:=json.Marshal(connectionFile{Default:"reader",Profiles:profiles});if err!=nil{t.Fatal(err)};path:=filepath.Join(t.TempDir(),"connections.json");if err=os.WriteFile(path,raw,0600);err!=nil{t.Fatal(err)};return path
}

func TestOperatorRemoteReaderLaunchAndSwitchPreserveMode(t *testing.T){
 profiles:=map[string]ConnectionProfile{
  "reader":{URL:"https://core.example.invalid",TokenEnv:"READER_TOKEN",AuthMode:AuthModeOAuthBearer,Scopes:[]string{"service-lasso:read"}},
  "other":{URL:"https://other.example.invalid",TokenEnv:"OTHER_TOKEN",AuthMode:AuthModeOAuthBearer,Scopes:[]string{"service-lasso:read"}},
  "incomplete":{TokenEnv:"NEVER_TOKEN",AuthMode:AuthModeOAuthBearer,Scopes:[]string{"service-lasso:read"}},
 }
 reads:=map[string]int{};env:=func(name string)string{reads[name]++;switch name{case "READER_TOKEN":return "codec-only-reader";case "OTHER_TOKEN":return "codec-only-other";case "NEVER_TOKEN":t.Error("inadmissible profile read credential")};return ""}
 manager,client,err:=ResolveOperatorConnections(ConnectionOptions{ConfigPath:operatorReaderConfig(t,profiles),Env:env});if err!=nil||client==nil||!manager.operatorV2||manager.Current()!="reader"||reads["READER_TOKEN"]!=1{t.Fatal("v2 general reader launch denied")}
 if client.authMode!=AuthModeOAuthBearer||client.baseURL!=profiles["reader"].URL{t.Fatal("transport profile changed")}
 switched,err:=manager.Switch("other");if err!=nil||switched==nil||manager.Current()!="other"||reads["OTHER_TOKEN"]!=1{t.Fatal("v2 reader switch lost mode")}
 if _,err=manager.Switch("incomplete");err==nil||manager.Current()!="other"||reads["NEVER_TOKEN"]!=0{t.Fatal("denied switch changed selection or read credential")}
 if _,err=manager.Switch("missing");err==nil||manager.Current()!="other"{t.Fatal("unknown switch changed selection")}
}

func TestOperatorInvalidRemoteProfilesPrecedeCredentialLookup(t *testing.T){
 for _,profile:=range []ConnectionProfile{
  {URL:"https://core.example.invalid",TokenEnv:"NEVER_TOKEN",AuthMode:AuthModeOAuthBearer,Scopes:[]string{"service-lasso:lifecycle:write"}},
  {URL:"https://core.example.invalid",TokenEnv:"NEVER_TOKEN",AuthMode:AuthModeLocalAdmin,Scopes:[]string{"service-lasso:read"}},
  {URL:"http://core.example.invalid",TokenEnv:"NEVER_TOKEN",AuthMode:AuthModeOAuthBearer,Scopes:[]string{"service-lasso:read"}},
 }{read:=false;_,_,err:=ResolveOperatorConnections(ConnectionOptions{ConfigPath:operatorReaderConfig(t,map[string]ConnectionProfile{"reader":profile}),Env:func(name string)string{if name=="NEVER_TOKEN"{read=true};return ""}});if err==nil||read{t.Fatal("inadmissible remote profile reached credential")}}
}

func TestOperatorReaderDoesNotChangeLegacyV1Admission(t *testing.T){
 reader:=ConnectionProfile{URL:"https://core.example.invalid",TokenEnv:"READER_TOKEN",AuthMode:AuthModeOAuthBearer,Scopes:[]string{"service-lasso:read"}}
 path:=operatorReaderConfig(t,map[string]ConnectionProfile{"reader":reader});read:=false
 if _,_,err:=ResolveConnections(ConnectionOptions{ConfigPath:path,Env:func(name string)string{if name=="READER_TOKEN"{read=true;return "codec-only"};return ""}});err==nil||read{t.Fatal("legacy both-scope admission relaxed")}
 if _,err:=NewClientWithAuth(reader.URL,nil,"codec-only",AuthModeOAuthBearer,reader.Scopes...);err==nil{t.Fatal("legacy literal constructor relaxed")}
 if _,err:=NewOperatorClientWithAuth(reader.URL,nil,"codec-only",AuthModeOAuthBearer,reader.Scopes...);err!=nil{t.Fatal("v2 literal reader denied")}
 if _,err:=NewClientWithAuth(reader.URL,nil,"codec-only",AuthModeOAuthBearer,"service-lasso:read","service-lasso:lifecycle:write");err!=nil{t.Fatal("legacy positive admission lost")}
 reader.Scopes=append(reader.Scopes,"service-lasso:lifecycle:write");read=false
 legacy,_,err:=ResolveConnections(ConnectionOptions{ConfigPath:operatorReaderConfig(t,map[string]ConnectionProfile{"reader":reader}),Env:func(name string)string{if name=="READER_TOKEN"{read=true;return "codec-only"};return ""}})
 if err!=nil||legacy==nil||legacy.operatorV2||!read{t.Fatal("legacy positive resolver changed")}
}

func TestOperatorRemoteOverrideRejectedBeforeCredential(t *testing.T){
 reader:=ConnectionProfile{URL:"https://core.example.invalid",TokenEnv:"READER_TOKEN",AuthMode:AuthModeOAuthBearer,Scopes:[]string{"service-lasso:read"}};read:=false
 _,_,err:=ResolveOperatorConnections(ConnectionOptions{ConfigPath:operatorReaderConfig(t,map[string]ConnectionProfile{"reader":reader}),APIURL:"http://insecure.example.invalid",TokenEnv:"NEVER_TOKEN",Env:func(name string)string{if name=="NEVER_TOKEN"||name=="READER_TOKEN"{read=true};return ""}})
 if err==nil||read{t.Fatal("insecure override read credential")}
}
