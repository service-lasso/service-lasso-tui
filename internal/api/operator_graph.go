package api

// This validates one closed public fragment. Snapshot/cursor ownership and
// continuity across pages remain separate from this DTO admission.
func operatorGraphGroupValid(group OperatorGraphGroup)bool{
 if !operatorNodeValid(group.Node)||group.Edges==nil||len(group.Edges)>16||group.EdgeOffset<0||group.EdgeTotal<0||group.EdgeTotal>4096||group.EdgeOffset+len(group.Edges)>group.EdgeTotal||group.Complete!=(group.EdgeOffset+len(group.Edges)==group.EdgeTotal){return false}
 if len(group.Edges)==0{return group.EdgeOffset==0&&group.EdgeTotal==0&&group.Complete}
 var previous *OperatorEdge
 for _,edge:=range group.Edges{
  if !operatorEdgeValid(edge)||edge.ToServiceID!=group.Node.ServiceID{return false}
  // Canonical service IDs and closed kinds compare by original UTF8 bytes.
  // State is not part of edge identity; differing-state duplicates also fail.
  if previous!=nil&&(edge.FromServiceID<previous.FromServiceID||edge.FromServiceID==previous.FromServiceID&&edge.Kind<=previous.Kind){return false}
  current:=edge;previous=&current
 }
 return true
}
