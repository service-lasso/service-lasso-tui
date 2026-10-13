package api

// A token is usable only as an exact member of its fetched action's roster.
// Syntax and policy alone never grant reference authority.
func (a OperatorAction) ReferenceReady(token string) bool {
 if !a.Availability.Available || !a.PayloadPolicy.References { return false }
 for _,row:=range a.PayloadReferences { if row.Availability.Available && row.PayloadRef!=nil && *row.PayloadRef==token { return true } }
 return false
}
func operatorActionReferencesValid(a OperatorAction) bool {
 if a.PayloadReferences==nil || len(a.PayloadReferences)>128 { return false }
 if !a.PayloadPolicy.References && len(a.PayloadReferences)!=0 { return false }
 seen:=map[string]bool{}
 for _,row:=range a.PayloadReferences {
  if row.Label=="" || !operatorDisplay(row.Label,128) || !row.Availability.Valid() { return false }
  if row.Availability.Available {
   if row.PayloadRef==nil || !OperatorID(*row.PayloadRef) || seen[*row.PayloadRef] { return false };seen[*row.PayloadRef]=true
  } else if row.PayloadRef!=nil { return false }
 }
 return true
}
func operatorActionRosterValid(actions []OperatorAction) bool {
 if actions==nil || len(actions)>128 { return false }
 ids,tokens:=map[string]bool{},map[string]bool{};count:=0
 for _,action:=range actions {
  if !operatorActionValid(action) || ids[action.ActionID] { return false };ids[action.ActionID]=true
  count+=len(action.PayloadReferences);if count>128 { return false }
  for _,row:=range action.PayloadReferences { if row.PayloadRef!=nil { if tokens[*row.PayloadRef] { return false };tokens[*row.PayloadRef]=true } }
 }
 return true
}
func (f OperatorActionForm) Valid() bool {
 if !OperatorServiceID(f.ServiceID)||!OperatorID(f.DefinitionRevision)||!f.Availability.Valid()||!f.Availability.Available&&len(f.Actions)!=0||!operatorActionRosterValid(f.Actions){return false}
 for _,action:=range f.Actions{if action.DefinitionRevision!=f.DefinitionRevision{return false}}
 return true
}
