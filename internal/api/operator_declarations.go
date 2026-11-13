package api

// Declaration admission retains the exact fetched permission/payload policy.
// The server remains the final owner of manifest validation and execution.
func OperatorSetupInputValid(form OperatorSetupForm,input OperatorSetupInput)bool{
 // Selected setup owns the empty Values/inputFields baseline. The actual
 // ServiceSetupStep has no declarative public input schema; action payload
 // schemas and private command/env/fingerprint fields cannot extend setup.
 if !form.Availability.Available||input.Values==nil||len(input.Values)!=0{return false}
 for _,step:=range form.Steps{
  if step.StepID!=input.StepID{continue};if !step.Availability.Available{return false}
  return true
 };return false
}
func OperatorDeclaredInputValid(form OperatorActionForm,input OperatorDeclaredInput)bool{ok,_:=OperatorDeclaredInputValidCause(form,input);return ok}
func OperatorDeclaredInputValidCause(form OperatorActionForm,input OperatorDeclaredInput)(bool,error){
 if !form.Availability.Available{return false,nil}
 for _,action:=range form.Actions{if action.InputFields!=nil{_,_,err:=action.InputFields.TreeCause();if err!=nil{return false,err}}}
 if !form.Valid(){return false,nil}
 for _,action:=range form.Actions{
  if action.ActionID!=input.DeclaredActionID{continue};if !action.Availability.Available{return false,nil}
  inline:=len(input.Payload)>0;reference:=input.PayloadRef!=nil
  policy:=action.PayloadPolicy
  if inline&&!policy.Inline||reference&&!action.ReferenceReady(*input.PayloadRef)||inline&&reference&&!policy.AllowMixed||policy.Required&&!inline&&!reference{return false,nil}
  if !inline{return true,nil}
  if action.InputFields==nil{return false,nil};schema,ok,err:=action.InputFields.TreeCause();if err!=nil{return false,err};if !ok{return false,nil};if reference{return schema.OverlayValidCause(input.Payload)};return schema.ValueValidCause(input.Payload)
 };return false,nil
}
