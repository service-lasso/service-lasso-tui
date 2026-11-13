package app

import (
 "encoding/json"
 "fmt"
 "sort"
 "strings"

 "github.com/service-lasso/service-lasso-tui/internal/api"
)

func(m operatorModel)declaredInspectionAction()*api.OperatorAction{
 if m.journey!="declared_actions"||m.focus!=1||m.loading||!m.pageSelectionCurrent()||m.formReadFailed()||m.actions==nil||!m.actions.Availability.Available||m.row<0||m.row>=len(m.actions.Actions)||!m.draft.Sent&&!m.draft.Retired&&(m.draft.Dirty||m.draft.Preview!=nil){return nil};return &m.actions.Actions[m.row]
}
func(m operatorModel)declaredInspectionCurrent()bool{action:=m.declaredInspectionAction();return m.declaredReadReview&&action!=nil&&m.declaredReadBinding==m.pageSelection()&&m.declaredReadAction==action.ActionID}
func(m operatorModel)declaredInspectionRows()[]string{
 action:=m.declaredInspectionAction();if action==nil{return nil}
 policy:=action.PayloadPolicy
 original:=[]string{"Read-only declaration: "+action.ActionID,action.Description,fmt.Sprintf("Payload: inline=%t references=%t mixed=%t required=%t",policy.Inline,policy.References,policy.AllowMixed,policy.Required),"Required profile: "+action.RequiredProfile,"Required scopes: "+strings.Join(action.RequiredScopes,", "),"Declared permission: "+action.DeclaredPermission,"Action: "+operatorReason(action.Availability)}
 if action.InputFields==nil{status:="not declared";if !action.Availability.Available&&policy.Inline{status="unavailable"};original=append(original,"Payload schema: "+status)}else if _,ok:=action.InputFields.Tree();!ok{original=append(original,"Payload schema: unsupported")}else{
  original=append(original,fmt.Sprintf("Payload schema root: node %d",action.InputFields.Root))
  for _,node:=range action.InputFields.Nodes{
   original=append(original,fmt.Sprintf("Node %d: %s",node.ID,strings.Join(node.Types," | ")))
   if len(node.Required)>0{original=append(original,"  Required properties: "+strings.Join(node.Required,", "))}
   for _,edge:=range node.Properties{original=append(original,fmt.Sprintf("  Property %s -> node %d",edge.Name,edge.Node))}
   if node.Items!=nil{original=append(original,fmt.Sprintf("  Array items -> node %d",*node.Items))}
   keys:=[]string{};for key:=range node.Constraints{keys=append(keys,key)};sort.Strings(keys)
   for _,key:=range keys{value:=string(node.Constraints[key]);if key=="enum"{var values []json.RawMessage;if json.Unmarshal(node.Constraints[key],&values)!=nil{continue};value=fmt.Sprintf("%d declared choices (values withheld)",len(values))};original=append(original,"  Constraint "+key+": "+value)}
  }
 }
 original=append(original,"Payload references (opaque tokens withheld):")
 if len(action.PayloadReferences)==0{original=append(original,"  No declared references")};for _,reference:=range action.PayloadReferences{original=append(original,"  "+reference.Label+": "+operatorReason(reference.Availability))}
 original=append(original,"Schedule/concurrency/failure metadata: unavailable in this contract","Alt+PgUp/PgDn or Up/Down inspect; Alt+Home/Escape return; r refresh")
 rows:=[]string{};for _,line:=range original{rows=append(rows,operatorReviewWrap(line,m.width)...)};return rows
}
func(m *operatorModel)moveDeclaredInspection(down bool){
 action:=m.declaredInspectionAction();if action==nil{return};rows:=m.declaredInspectionRows();if len(rows)==0{return}
 if !m.declaredInspectionCurrent(){m.declaredReadReview=true;m.declaredReadAction=action.ActionID;m.declaredReadBinding=m.pageSelection();m.declaredReadOffset=0;return}
 step:=m.height-2;if step<1{step=1};if down{m.declaredReadOffset+=step}else{m.declaredReadOffset-=step};if m.declaredReadOffset<0{m.declaredReadOffset=0};if m.declaredReadOffset>=len(rows){m.declaredReadOffset=len(rows)-1}
}
