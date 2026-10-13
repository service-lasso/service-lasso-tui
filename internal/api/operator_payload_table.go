package api

import "encoding/json"

// Numeric node IDs describe topology only. Source and action authority remain
// in the immutable fetched form and its family revision binding.
type OperatorPayloadTable struct{Root int `json:"root"`;Nodes []OperatorPayloadNode `json:"nodes"`}
type OperatorPayloadNode struct{
 ID int `json:"id"`;Types []string `json:"types"`
 Constraints OperatorPayloadConstraintMap `json:"constraints"`
 Properties []OperatorPayloadEdge `json:"properties"`;Required []string `json:"required"`;Items *int `json:"items"`
}
type OperatorPayloadEdge struct{Name string `json:"name"`;Node int `json:"node"`}
type OperatorPayloadConstraintMap map[string]json.RawMessage
func operatorActionSchemaValid(action OperatorAction)bool{if action.InputFields==nil{if !action.Availability.Available||!action.PayloadPolicy.Required{return true};for _,row:=range action.PayloadReferences{if row.PayloadRef!=nil&&action.ReferenceReady(*row.PayloadRef){return true}};return false};if !action.PayloadPolicy.Inline{return false};_,ok:=action.InputFields.Tree();return ok}
func operatorPayloadConstraintShape(value any)bool{ok,_:=operatorPayloadConstraintShapeCause(value);return ok}
func operatorPayloadConstraintShapeCause(value any)(bool,error){
 object,ok:=value.(map[string]any);if !ok{return false,nil};for key,child:=range object{if child==nil{return false,nil};switch key{
 case "enum":values,ok:=child.([]any);if !ok||len(values)==0||len(values)>32{return false,nil}
 case "minimum","maximum":if _,ok:=child.(json.Number);!ok{return false,nil}
 case "minLength","maxLength","minItems","maxItems":number,ok:=child.(json.Number);if !ok{return false,nil};value,err:=number.Float64();if err!=nil{return false,err};if value<0||float64(int64(value))!=value{return false,nil}
 case "additionalProperties":if value,ok:=child.(bool);!ok||value{return false,nil}
 default:return false,nil
 }};return true,nil
}
func(t OperatorPayloadTable)Tree()(*OperatorPayloadSchema,bool){root,ok,_:=t.TreeCause();return root,ok}
func(t OperatorPayloadTable)TreeCause()(*OperatorPayloadSchema,bool,error){
 if t.Root!=0||len(t.Nodes)==0||len(t.Nodes)>512{return nil,false,nil};next,properties:=0,0
 var visit func(int,int)(OperatorPayloadSchema,bool,error)
 visit=func(index,depth int)(OperatorPayloadSchema,bool,error){
  if depth>8||index!=next||index<0||index>=len(t.Nodes){return OperatorPayloadSchema{},false,nil};node:=t.Nodes[index];if node.ID!=index||node.Properties==nil||node.Required==nil||node.Constraints==nil{return OperatorPayloadSchema{},false,nil};next++
  schema:=OperatorPayloadSchema{Types:append([]string(nil),node.Types...),Properties:[]OperatorPayloadProperty{},Required:append([]string{},node.Required...)}
  constraints,ok,err:=operatorPayloadCompactConstraintsCause(node);if err!=nil{return OperatorPayloadSchema{},false,err};if !ok{return schema,false,nil};schema.Constraints=constraints
  last:="";for _,edge:=range node.Properties{properties++;if properties>64||edge.Name<=last{return schema,false,nil};last=edge.Name;child,ok,err:=visit(edge.Node,depth+1);if err!=nil{return OperatorPayloadSchema{},false,err};if !ok{return schema,false,nil};schema.Properties=append(schema.Properties,OperatorPayloadProperty{Name:edge.Name,Schema:child})}
  if node.Items!=nil{child,ok,err:=visit(*node.Items,depth+1);if err!=nil{return OperatorPayloadSchema{},false,err};if !ok{return schema,false,nil};schema.Items=&child};return schema,true,nil
 }
 root,ok,err:=visit(0,1);if err!=nil{return nil,false,err};if !ok||next!=len(t.Nodes){return nil,false,nil};ok,err=root.ValidRootCause();if err!=nil{return nil,false,err};if !ok{return nil,false,nil};return &root,true,nil
}
func operatorPayloadCompactConstraints(node OperatorPayloadNode)(OperatorPayloadConstraints,bool){c,ok,_:=operatorPayloadCompactConstraintsCause(node);return c,ok}
func operatorPayloadCompactConstraintsCause(node OperatorPayloadNode)(OperatorPayloadConstraints,bool,error){
 schema:=OperatorPayloadSchema{Types:node.Types};c:=OperatorPayloadConstraints{}
 if schema.Has("string"){minimum,maximum:=0,4096;c.MinLength=&minimum;c.MaxLength=&maximum}
 if schema.Has("integer")||schema.Has("number"){minimum,maximum:=float64(-9007199254740991),float64(9007199254740991);c.Minimum=&minimum;c.Maximum=&maximum}
 if schema.Has("array"){minimum,maximum:=0,128;c.MinItems=&minimum;c.MaxItems=&maximum}
 for key,raw:=range node.Constraints{
  if string(raw)=="null"{return c,false,nil}
  switch key{
  case "enum":if err:=decodeOperator(raw,&c.Enum,OperatorResponseBytes);err!=nil{return OperatorPayloadConstraints{},false,err};if c.Enum==nil{return c,false,nil}
  case "minimum":if c.Minimum==nil{return c,false,nil};if err:=decodeOperator(raw,c.Minimum,OperatorResponseBytes);err!=nil{return OperatorPayloadConstraints{},false,err}
  case "maximum":if c.Maximum==nil{return c,false,nil};if err:=decodeOperator(raw,c.Maximum,OperatorResponseBytes);err!=nil{return OperatorPayloadConstraints{},false,err}
  case "minLength":if c.MinLength==nil{return c,false,nil};if err:=decodeOperator(raw,c.MinLength,OperatorResponseBytes);err!=nil{return OperatorPayloadConstraints{},false,err}
  case "maxLength":if c.MaxLength==nil{return c,false,nil};if err:=decodeOperator(raw,c.MaxLength,OperatorResponseBytes);err!=nil{return OperatorPayloadConstraints{},false,err}
  case "minItems":if c.MinItems==nil{return c,false,nil};if err:=decodeOperator(raw,c.MinItems,OperatorResponseBytes);err!=nil{return OperatorPayloadConstraints{},false,err}
  case "maxItems":if c.MaxItems==nil{return c,false,nil};if err:=decodeOperator(raw,c.MaxItems,OperatorResponseBytes);err!=nil{return OperatorPayloadConstraints{},false,err}
  case "additionalProperties":if !schema.Has("object"){return c,false,nil};value:=false;if err:=decodeOperator(raw,&value,OperatorResponseBytes);err!=nil{return OperatorPayloadConstraints{},false,err};if value{return c,false,nil};c.AdditionalProperties=&value
  default:return c,false,nil
  }
 };if schema.Has("object")&&c.AdditionalProperties==nil{return c,false,nil};return c,true,nil
}
