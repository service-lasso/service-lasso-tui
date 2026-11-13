package app

import (
 "context"
 "strconv"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// These are client observations, never metadata revision issuers or ABA proof.
// Only an authentic selected v2 producer can grant ready metadata authority.
// Retained generation-key storage still needs the selected finite diagnostic
// owner/lifecycle. This in-memory cache is not durable capture or capacity proof.
type operatorMetaKey struct{Connection uint64;Binding api.OperatorContext `json:"-"`;Service,Generation string}
type operatorMetaObservation struct{Key operatorMetaKey;Value api.OperatorMeta;Err error `json:"-"`;Ticket operatorTicket `json:"-"`;Before,After *api.OperatorDetail `json:"-"`;Candidate *api.OperatorMeta `json:"-"`}
type operatorMetadataRead struct{Observation operatorMetaObservation}
type operatorOverviewRead struct{Page api.OperatorPage[api.OperatorService];Metadata []operatorMetaObservation}
type operatorGraphRead struct{Page api.OperatorPage[api.OperatorGraphGroup];Metadata []operatorMetaObservation}
func operatorMetadataObservationBound(observation operatorMetaObservation,ticket operatorTicket)bool{
 if observation.Ticket!=ticket||observation.Key.Connection!=ticket.Connection||observation.Key.Binding!=ticket.Binding||!api.OperatorServiceID(observation.Key.Service){return false}
 if observation.Err!=nil{return true}
 return api.OperatorID(observation.Key.Generation)&&observation.Value.Valid()&&observation.Value.ServiceID==observation.Key.Service
}

func operatorReadMetadata(ctx context.Context,client *api.Client,ticket operatorTicket,id string)operatorMetaObservation{
 observation:=operatorMetaObservation{Key:operatorMetaKey{Connection:ticket.Connection,Binding:ticket.Binding,Service:id},Ticket:ticket}
 fail:=func(err error)operatorMetaObservation{observation.Err=err;return observation}
 if err:=operatorSameContext(ctx,client,ticket.Binding);err!=nil{return fail(err)}
 before,err:=client.OperatorDetail(ctx,id);if err!=nil{return fail(err)}
 observation.Before=&before
 observation.Key.Generation=before.Summary.Generation
 value,err:=client.OperatorMeta(ctx,id);if err!=nil{return fail(err)}
 observation.Candidate=&value
 after,err:=client.OperatorDetail(ctx,id);if err!=nil{return fail(err)}
 observation.After=&after
 if before.Summary.Generation!=after.Summary.Generation||before.Summary.ModelEpoch!=after.Summary.ModelEpoch{return fail(&api.OperatorPublicError{Code:"revision_conflict"})}
 if err:=operatorSameContext(ctx,client,ticket.Binding);err!=nil{return fail(err)}
 observation.Value=value;return observation
}
func(m *operatorModel)acceptMetadata(observation operatorMetaObservation){
 if m.metadata==nil{m.metadata=map[operatorMetaKey]operatorMetaObservation{}}
 // Failure never overwrites the privately retained last successful values.
 if observation.Err==nil&&observation.Value.Availability.Available{m.metadata[observation.Key]=observation}
 if m.metadataCurrent==nil{m.metadataCurrent=map[string]operatorMetaObservation{}}
 m.metadataCurrent[observation.Key.Service]=observation
}
func(m operatorModel)observedMetadata(id,generation string)(api.OperatorMeta,bool){
 observation,ok:=m.metadataCurrent[id]
 if !ok||observation.Err!=nil||observation.Key.Connection!=m.epoch||observation.Key.Binding!=m.binding||generation!=""&&observation.Key.Generation!=generation||!observation.Value.Availability.Available{return api.OperatorMeta{},false}
 return observation.Value,true
}
func(m operatorModel)overviewServices()[]api.OperatorService{
 rows:=[]api.OperatorService{}
 for _,service:=range m.services.Rows{meta,ready:=m.observedMetadata(service.ServiceID,service.Generation);if m.favoritesOnly&&ready&&meta.Favorite!=nil&&!*meta.Favorite{continue};rows=append(rows,service)}
 return rows
}
func(m operatorModel)favoriteLabel(id,generation string)string{meta,ready:=m.observedMetadata(id,generation);if !ready||meta.Favorite==nil{return "unknown"};return strconv.FormatBool(*meta.Favorite)}
func(m operatorModel)layoutLabel(id string)string{observation,ok:=m.metadataCurrent[id];if !ok||observation.Ticket!=m.ticket(){return "unknown"};meta,ready:=m.observedMetadata(id,observation.Key.Generation);if !ready{return "unknown"};if meta.Position==nil{return "automatic"};return strconv.Itoa(meta.Position.X)+","+strconv.Itoa(meta.Position.Y)}
