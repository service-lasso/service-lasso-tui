package app

import (
 "strings"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// Source fixtures use the actual closed opaque context and original Go v1
// digest producer. These are valid codec inputs, never issuer/native proof.
func selectedOperatorTestContext(actorHex string)api.OperatorContext{
 return api.OperatorContext{InstanceBinding:"slrc_"+strings.Repeat("1",64),WorkspaceBinding:"slrc_"+strings.Repeat("2",64),ActorBinding:"slrc_"+strings.Repeat(actorHex,64),ClientBinding:"slrc_"+strings.Repeat("4",64)}
}
