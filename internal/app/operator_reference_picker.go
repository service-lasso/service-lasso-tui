package app

import "github.com/service-lasso/service-lasso-tui/internal/api"

// The picker uses the exact fetched roster. Labels are display text only;
// neither a label nor a typed basename becomes a token.
func (m *operatorModel) pickPayloadReference(advance bool) bool {
 rows:=m.draft.PayloadReferences
 if len(rows)==0 { m.errorText="No payload references are available for this action.";return false }
 start:=m.draft.ReferenceIndex
 if start<0||start>=len(rows){start=0}
 if advance { start=(start+1)%len(rows) }
 for offset:=0;offset<len(rows);offset++ {
  index:=(start+offset)%len(rows);row:=rows[index]
  if !row.Availability.Available||row.PayloadRef==nil{continue}
  token:=*row.PayloadRef;m.draft.PayloadRef=&token;m.draft.ReferenceIndex=index;m.draft.ReferenceMode=true;m.draft.ChoiceReview=false;m.draft.ChoiceOffset=0;if !m.draft.PayloadPolicy.AllowMixed{m.draft.InlinePayload=false};m.draft.Selector="Payload reference";m.setEditorText(row.Label);return true
 }
 m.errorText="No payload references are currently available for this action.";return false
}
func (m operatorModel) selectedPayloadReferenceValid() bool {
 if m.draft.PayloadRef==nil{return false}
 for _,row:=range m.draft.PayloadReferences{if row.Availability.Available&&row.PayloadRef!=nil&&*row.PayloadRef==*m.draft.PayloadRef{return true}}
 return false
}
func (m *operatorModel) startPayloadOverlay() bool {
 if !m.selectedPayloadReferenceValid()||!m.draft.PayloadPolicy.AllowMixed||m.actions==nil{return false}
 for _,action:=range m.actions.Actions{
  if action.ActionID!=m.draft.DeclaredID||action.InputFields==nil{continue};schema,ok,err:=action.InputFields.TreeCause();if err!=nil{return m.editorFailure(m.draft,api.OperatorInvalidRequestCause(err))};if !ok{return false}
  node:=newOperatorPayloadOverlay(schema);if node==nil{return false};m.draft.Payload=node;m.draft.PayloadIndex=0;m.draft.InlinePayload=true;m.draft.ReferenceMode=false;m.draft.ChoiceReview=false;m.draft.ChoiceOffset=0;m.showPayloadFocus();return true
 };return false
}
func (m operatorModel) payloadReferenceRows() []string {
 if m.draft.Action!="declared_action_run"||!m.draft.PayloadPolicy.References{return nil}
 rows:=[]string{"Alt+p / Tab in reference picker: next available stored payload"}
 if m.draft.PayloadRef!=nil{for _,row:=range m.draft.PayloadReferences{if row.PayloadRef!=nil&&row.Availability.Available&&*row.PayloadRef==*m.draft.PayloadRef{rows=append(rows,"Selected stored payload: "+row.Label);break}}}
 if m.draft.PayloadPolicy.AllowMixed{rows=append(rows,"Alt+m: replace inline draft with an empty property overlay")}
 return rows
}
