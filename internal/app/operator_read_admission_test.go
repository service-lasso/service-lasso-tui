package app

import (
 "testing"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// These isolated reducer capability cases confer no authenticated issuer
// authority and do not replace complete discovery decode/native acceptance.
func TestOperatorMissingReadCapabilityStopsLoad(t *testing.T){
 model:=operatorModel{journey:"overview",discovery:&api.OperatorDiscovery{Capabilities:[]api.OperatorCapability{}},loading:true}
 if cmd:=model.load(nil);cmd!=nil||model.loading{t.Fatal("missing advertised read capability dispatched a request")}
 if model.errorText!=api.OperatorMessage("unsupported_capability"){t.Fatal("missing read capability was treated as ready")}
}
func TestOperatorReadAdmissionRetainsActualAvailabilityReason(t *testing.T){
 reason:="permission_denied"
 discovery:=&api.OperatorDiscovery{Capabilities:[]api.OperatorCapability{{ID:"read_overview",Availability:api.OperatorAvailability{Supported:true,ReasonCode:&reason}}}}
 if allowed,code:=operatorReadAdmission(discovery,"overview");allowed||code!=reason{t.Fatal("actual permission denial was discarded")}
 discovery.Capabilities[0].Availability=api.OperatorAvailability{Supported:true,Available:true}
 if allowed,code:=operatorReadAdmission(discovery,"overview");!allowed||code!=""{t.Fatal("advertised available read rejected")}
 if allowed,_:=operatorReadAdmission(discovery,"graph");allowed{t.Fatal("another read capability supplied graph authority")}
}
