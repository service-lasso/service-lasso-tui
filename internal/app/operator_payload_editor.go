package app

import (
 "encoding/json"
 "math"
 "sort"
 "strconv"
 "strings"

 tea "github.com/charmbracelet/bubbletea"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

type operatorPayloadNodeDraft struct{
 // Populated only in a private captured-value tree, preserving each original
 // node owner even after that node is removed from the editable draft.
 EvidenceOwner *operatorPayloadNodeDraft `json:"-"`
 Schema *api.OperatorPayloadSchema
 OverlaySchema *api.OperatorPayloadSchema
 Present,Required bool
 Variant string
 VariantCursor int
 Text string
 EnumIndex int
 EnumCursor int
 PropertyIndex int
 Properties map[string]*operatorPayloadNodeDraft
 Items []*operatorPayloadNodeDraft
}
type operatorPayloadFocus struct{Node,Parent *operatorPayloadNodeDraft;Path string;Item int}
func newOperatorPayloadNode(schema *api.OperatorPayloadSchema,required bool)*operatorPayloadNodeDraft{
 count:=0;node,ok:=operatorPayloadNodeWithin(schema,required,&count);if !ok{return nil};return node
}
// Starting an overlay is an explicit draft replacement. Only this root copy
// loses required/enum checks; child schemas and final server authority remain.
func newOperatorPayloadOverlay(schema *api.OperatorPayloadSchema)*operatorPayloadNodeDraft{
 if schema==nil||!schema.ValidRoot(){return nil};root:=*schema;root.Required=[]string{};root.Constraints.Enum=nil
 node:=newOperatorPayloadNode(&root,true);if node!=nil{node.OverlaySchema=schema};return node
}
func operatorPayloadNodeWithin(schema *api.OperatorPayloadSchema,required bool,count *int)(*operatorPayloadNodeDraft,bool){
 node:=&operatorPayloadNodeDraft{Schema:schema,Present:required,Required:required,Variant:schema.Types[0]};if !required{return node,true};choice:=operatorPayloadMinimum(schema);if choice.cost>8192-*count{return nil,false};node.VariantCursor=choice.variant;node.Variant=schema.Types[choice.variant];node.EnumIndex=choice.enum;node.EnumCursor=choice.enum;return node,node.rebuildWithin(count)
}
type operatorPayloadMinimumChoice struct{variant,enum,cost int}
func operatorPayloadMinimum(schema *api.OperatorPayloadSchema)operatorPayloadMinimumChoice{
 memo:=map[*api.OperatorPayloadSchema]operatorPayloadMinimumChoice{};var visit func(*api.OperatorPayloadSchema)operatorPayloadMinimumChoice
 visit=func(current *api.OperatorPayloadSchema)operatorPayloadMinimumChoice{if cached,ok:=memo[current];ok{return cached};best:=operatorPayloadMinimumChoice{cost:8193}
  if choices:=current.Constraints.Enum;choices!=nil{for index,raw:=range choices{cost:=operatorPayloadEnumExtent(raw);if cost<best.cost{best=operatorPayloadMinimumChoice{enum:index,cost:cost}}};memo[current]=best;return best}
  for index,kind:=range current.Types{cost:=1;switch kind{
  case "integer":c:=current.Constraints;if c.Minimum!=nil&&c.Maximum!=nil&&math.Ceil(*c.Minimum)>math.Floor(*c.Maximum){cost=8193}
  case "object":for i:=range current.Properties{property:=&current.Properties[i];required:=false;for _,name:=range current.Required{if name==property.Name{required=true}};if required{cost+=visit(&property.Schema).cost}}
  case "array":if current.Items==nil||current.Constraints.MinItems==nil{cost=8193}else{cost+=*current.Constraints.MinItems*visit(current.Items).cost}
  };if cost<best.cost{best=operatorPayloadMinimumChoice{variant:index,cost:cost}}
  };memo[current]=best;return best
 };return visit(schema)
}
func(node *operatorPayloadNodeDraft)rebuildWithin(count *int)bool{
 *count++;if *count>8192{return false}
 node.Text="";node.Properties=map[string]*operatorPayloadNodeDraft{};node.Items=nil
 if choices:=node.Schema.Constraints.Enum;choices!=nil{choice:=operatorPayloadMinimum(node.Schema);node.EnumIndex=choice.enum;node.EnumCursor=choice.enum;*count+=operatorPayloadEnumExtent(choices[node.EnumIndex])-1;return *count<=8192}
 switch node.Variant{
 case "boolean":node.Text="false"
 case "integer","number":value:=float64(0);c:=node.Schema.Constraints;if c.Minimum!=nil&&value<*c.Minimum{value=*c.Minimum};if c.Maximum!=nil&&value>*c.Maximum{value=*c.Maximum};if node.Variant=="integer"{value=math.Ceil(value);if c.Maximum!=nil&&value>*c.Maximum{value=math.Floor(*c.Maximum)};if c.Minimum!=nil&&value<*c.Minimum{return false}};node.Text=strconv.FormatFloat(value,'g',-1,64)
 case "object":for i:=range node.Schema.Properties{property:=&node.Schema.Properties[i];required:=false;for _,name:=range node.Schema.Required{if name==property.Name{required=true}};if !required{continue};child,ok:=operatorPayloadNodeWithin(&property.Schema,true,count);if !ok{return false};node.Properties[property.Name]=child};node.PropertyIndex=0;node.selectProperty(0)
 case "array":minimum:=0;if node.Schema.Constraints.MinItems!=nil{minimum=*node.Schema.Constraints.MinItems};for i:=0;i<minimum;i++{child,ok:=operatorPayloadNodeWithin(node.Schema.Items,true,count);if !ok{return false};node.Items=append(node.Items,child)}
 }
 return true
}
// Each present object keeps at most one absent property wrapper. Other absent
// declarations stay in the shared schema, so draft/focus metadata is bounded
// by twice the 8192 present-value ceiling rather than repeated schema expansion.
func(node *operatorPayloadNodeDraft)selectProperty(index int){
 if node.Variant!="object"||len(node.Schema.Properties)==0{return};for name,child:=range node.Properties{if !child.Present&&!child.Required{delete(node.Properties,name)}};node.PropertyIndex=index%len(node.Schema.Properties);property:=&node.Schema.Properties[node.PropertyIndex];if node.Properties[property.Name]==nil{node.Properties[property.Name]=&operatorPayloadNodeDraft{Schema:&property.Schema,Variant:property.Schema.Types[0]}}
}
func operatorPayloadEnumExtent(raw json.RawMessage)int{count,err:=operatorPayloadEnumExtentCause(raw);if err!=nil{return 8193};return count}
func operatorPayloadEnumExtentCause(raw json.RawMessage)(int,error){var value any;if err:=json.Unmarshal(raw,&value);err!=nil{return 0,err};var count func(any)int;count=func(value any)int{total:=1;switch v:=value.(type){case map[string]any:for _,child:=range v{total+=count(child)};case []any:for _,child:=range v{total+=count(child)}};return total};return count(value),nil}
func(node *operatorPayloadNodeDraft)extent()int{if !node.Present{return 0};if choices:=node.Schema.Constraints.Enum;choices!=nil{return operatorPayloadEnumExtent(choices[node.EnumIndex])};count:=1;for _,child:=range node.Properties{count+=child.extent()};for _,child:=range node.Items{count+=child.extent()};return count}
func(m *operatorModel)rebuildPayloadNode(node *operatorPayloadNodeDraft,variant string)bool{old:=*node;cursor:=node.VariantCursor;count:=m.draft.Payload.extent()-node.extent();node.Present=true;node.Variant=variant;if !node.rebuildWithin(&count){*node=old;node.VariantCursor=cursor;m.errorText="This type's minimum exceeds the request value limit. Choose another declared type.";return false};return true}
func(node *operatorPayloadNodeDraft)value()(json.RawMessage,bool){raw,ok,_:=node.valueCause();return raw,ok}
func(node *operatorPayloadNodeDraft)valueCause()(json.RawMessage,bool,error){
 if !node.Present{return nil,false,nil};if choices:=node.Schema.Constraints.Enum;choices!=nil{return append(json.RawMessage(nil),choices[node.EnumIndex%len(choices)]...),true,nil}
 var value any
 switch node.Variant{
 case "null":value=nil
 case "string":value=node.Text
 case "boolean":if node.Text!="true"&&node.Text!="false"{return nil,false,nil};value=node.Text=="true"
 case "integer","number":value=json.Number(node.Text)
 case "object":object:=map[string]json.RawMessage{};for _,property:=range node.Schema.Properties{child:=node.Properties[property.Name];if child==nil||!child.Present{required:=false;for _,name:=range node.Schema.Required{if name==property.Name{required=true}};if required{return nil,false,nil};continue};raw,ok,err:=child.valueCause();if err!=nil{return nil,false,err};if !ok{return nil,false,nil};object[property.Name]=raw};value=object
 case "array":array:=[]json.RawMessage{};for _,child:=range node.Items{raw,ok,err:=child.valueCause();if err!=nil{return nil,false,err};if !ok{return nil,false,nil};array=append(array,raw)};value=array
 default:return nil,false,nil
 };raw,err:=json.Marshal(value);if err!=nil{return nil,false,err};var admitted bool;if node.OverlaySchema!=nil{admitted,err=node.OverlaySchema.OverlayValidCause(raw)}else{admitted,err=node.Schema.ValueValidCause(raw)};if err!=nil{return nil,false,err};return raw,admitted,nil
}
func operatorPayloadPath(parent,name string)string{return parent+"/"+strings.ReplaceAll(strings.ReplaceAll(name,"~","~0"),"/","~1")}
func(node *operatorPayloadNodeDraft)focuses()[]operatorPayloadFocus{
 rows:=[]operatorPayloadFocus{};var walk func(*operatorPayloadNodeDraft,*operatorPayloadNodeDraft,string,int)
 walk=func(current,parent *operatorPayloadNodeDraft,path string,item int){rows=append(rows,operatorPayloadFocus{current,parent,path,item});if !current.Present||current.Schema.Constraints.Enum!=nil{return};switch current.Variant{case "object":for _,property:=range current.Schema.Properties{if child:=current.Properties[property.Name];child!=nil{walk(child,current,operatorPayloadPath(path,property.Name),-1)}};case "array":for i,child:=range current.Items{walk(child,current,path+"/"+strconv.Itoa(i),i)}}}
 walk(node,nil,"/input/payload",-1);return rows
}
func(m *operatorModel)payloadFocus()operatorPayloadFocus{rows:=m.draft.Payload.focuses();if m.draft.PayloadIndex>=len(rows){m.draft.PayloadIndex=0};return rows[m.draft.PayloadIndex]}
func(m *operatorModel)showPayloadFocus(){focus:=m.payloadFocus();m.draft.Selector=focus.Path;m.setEditorText(focus.Node.Text)}
func(m *operatorModel)savePayloadText()bool{
 if m.draft.Payload==nil||m.draft.ReferenceMode{return true};node:=m.payloadFocus().Node
 if node.Schema.Constraints.Enum!=nil||!node.Present{return true};if node.Variant=="string"||node.Variant=="integer"||node.Variant=="number"||node.Variant=="boolean"{node.Text=m.draft.Text};return true
}
func(node *operatorPayloadNodeDraft)textBytes()int{if !node.Present{return 0};total:=len(node.Text);for _,child:=range node.Properties{total+=child.textBytes()};for _,child:=range node.Items{total+=child.textBytes()};return total}
func(m *operatorModel)insertEditorText(text string){
 if m.draft.Action=="config_patch"&&(m.draft.Catalog!=nil&&m.draft.Catalog.Type=="template_entry"||len(m.draft.Fields)==1&&m.draft.Fields[0].Type=="template_entry"){return}
 if m.draft.ReferenceMode{return};if m.draft.Payload==nil{m.draft.insert(text,32768);return}
 node:=m.payloadFocus().Node;if !node.Present||node.Schema.Constraints.Enum!=nil||node.Variant=="object"||node.Variant=="array"||node.Variant=="null"{return};maximum:=4096;if node.Variant!="string"{maximum=128}
 if m.draft.Payload.textBytes()-len(node.Text)+len(m.draft.Text)+len(text)>api.OperatorRequestBytes{m.errorText="Draft text exceeds the request byte limit.";return};m.draft.insert(text,maximum)
}
func(m operatorModel)literalEditorInputAllowed()bool{
 if m.draft.ReferenceMode||m.draft.Preview!=nil||m.draft.Sent{return false}
 if m.draft.Payload!=nil{rows:=m.draft.Payload.focuses();if m.draft.PayloadIndex<0||m.draft.PayloadIndex>=len(rows){return false};node:=rows[m.draft.PayloadIndex].Node;return node.Present&&node.Variant=="string"&&node.Schema.Constraints.Enum==nil}
 if m.draft.Catalog!=nil{return m.draft.Catalog.Type=="file_entry"}
 return len(m.draft.Fields)==1&&m.draft.Fields[0].Type=="file_entry"
}
func(m *operatorModel)payloadKey(msg tea.KeyMsg)bool{
 if m.draft.Payload==nil||m.draft.ReferenceMode{return false};m.savePayloadText();focus:=m.payloadFocus();node:=focus.Node
 switch msg.String(){
 case "alt+pgdown","alt+pgup":if node.Present&&node.Schema.Constraints.Enum!=nil{rows,enumErr:=operatorPayloadEnumTreeCause(node.Schema.Constraints.Enum[node.EnumIndex]);if enumErr!=nil{m.editorFailure(m.draft,api.OperatorInvalidRequestCause(enumErr));return true};wrapped:=[]string{};for _,line:=range rows{wrapped=append(wrapped,operatorReviewWrap(line,m.width)...)};if len(wrapped)>0{if m.draft.ChoiceOffset<0{m.draft.ChoiceOffset=0};if m.draft.ChoiceOffset>=len(wrapped){m.draft.ChoiceOffset=len(wrapped)-1};step:=m.height-2;if step<1{step=1};if msg.String()=="alt+pgdown"{remaining:=len(wrapped)-1-m.draft.ChoiceOffset;if step>remaining{step=remaining};m.draft.ChoiceOffset+=step}else{if step>m.draft.ChoiceOffset{step=m.draft.ChoiceOffset};m.draft.ChoiceOffset-=step};m.draft.ChoiceReview=true}};return true
 case "alt+home":m.draft.ChoiceReview=false;return true
 case "alt+s":if node.Present&&node.Variant=="object"&&node.Schema.Constraints.Enum==nil{node.selectProperty(node.PropertyIndex+1)};m.draft.ChoiceReview=false;m.draft.ChoiceOffset=0;m.showPayloadFocus();return true
 case "tab","shift+tab":rows:=m.draft.Payload.focuses();if msg.String()=="tab"{m.draft.PayloadIndex=(m.draft.PayloadIndex+1)%len(rows)}else{m.draft.PayloadIndex=(m.draft.PayloadIndex+len(rows)-1)%len(rows)};m.draft.ChoiceReview=false;m.draft.ChoiceOffset=0;m.showPayloadFocus();return true
 case "alt+u":if !node.Required{if node.Present{node.Present=false;node.Text="";node.Properties=nil;node.Items=nil}else{m.rebuildPayloadNode(node,node.Variant)}};m.draft.ChoiceReview=false;m.draft.ChoiceOffset=0;m.showPayloadFocus();return true
 case "alt+v":if node.Schema.Constraints.Enum==nil{node.VariantCursor=(node.VariantCursor+1)%len(node.Schema.Types);m.rebuildPayloadNode(node,node.Schema.Types[node.VariantCursor])};m.draft.ChoiceReview=false;m.draft.ChoiceOffset=0;m.showPayloadFocus();return true
 case "alt+n":if choices:=node.Schema.Constraints.Enum;choices!=nil{nextCursor:=(node.EnumCursor+1)%len(choices);choiceExtent,enumErr:=operatorPayloadEnumExtentCause(choices[nextCursor]);if enumErr!=nil{candidate:=m.draft;candidate.Payload=operatorPayloadEvidence(m.draft.Payload);candidate.Payload.focuses()[m.draft.PayloadIndex].Node.EnumCursor=nextCursor;m.editorCandidateFailure(m.draft,candidate,api.OperatorInvalidRequestCause(enumErr));return true};node.EnumCursor=nextCursor;if m.draft.Payload.extent()-node.extent()+choiceExtent<=8192{node.EnumIndex=node.EnumCursor}else{m.errorText="This choice exceeds the request value limit. Choose another authorized choice."}}else if node.Variant=="boolean"{if node.Text=="true"{node.Text="false"}else{node.Text="true"}};m.draft.ChoiceReview=false;m.draft.ChoiceOffset=0;m.showPayloadFocus();return true
 case "alt+a":if node.Present&&node.Variant=="array"&&node.Schema.Constraints.Enum==nil&&len(node.Items)<*node.Schema.Constraints.MaxItems{count:=m.draft.Payload.extent();child,ok:=operatorPayloadNodeWithin(node.Schema.Items,true,&count);if ok{node.Items=append(node.Items,child)}else{m.errorText="Adding this item exceeds the request value limit."}};m.draft.ChoiceReview=false;m.draft.ChoiceOffset=0;m.showPayloadFocus();return true
 case "alt+d":if focus.Parent!=nil&&focus.Parent.Variant=="array"&&focus.Item>=0&&len(focus.Parent.Items)>*focus.Parent.Schema.Constraints.MinItems{parent:=focus.Parent;parent.Items=append(parent.Items[:focus.Item],parent.Items[focus.Item+1:]...);m.draft.PayloadIndex=0};m.draft.ChoiceReview=false;m.draft.ChoiceOffset=0;m.showPayloadFocus();return true
 };return false
}
func operatorPayloadEnumKind(raw json.RawMessage,schema *api.OperatorPayloadSchema)string{kind,err:=operatorPayloadEnumKindCause(raw,schema);if err!=nil{return "invalid"};return kind}
func operatorPayloadEnumKindCause(raw json.RawMessage,schema *api.OperatorPayloadSchema)(string,error){var value any;if err:=json.Unmarshal(raw,&value);err!=nil{return "",err};switch v:=value.(type){case nil:return "null",nil;case string:return "string",nil;case bool:return "boolean",nil;case float64:if schema.Has("integer")&&math.Trunc(v)==v{return "integer",nil};return "number",nil;case map[string]any:return "object",nil;case []any:return "array",nil};return "invalid",nil}
func operatorPayloadEnumTree(raw json.RawMessage)[]string{rows,err:=operatorPayloadEnumTreeCause(raw);if err!=nil{return []string{"Invalid choice"}};return rows}
func operatorPayloadEnumTreeCause(raw json.RawMessage)([]string,error){
 var value any;if err:=json.Unmarshal(raw,&value);err!=nil{return nil,err};lines:=[]string{};var walk func(any,string)
 walk=func(value any,path string){switch v:=value.(type){case map[string]any:lines=append(lines,path+" object");names:=[]string{};for name:=range v{names=append(names,name)};sort.Strings(names);for _,name:=range names{walk(v[name],operatorPayloadPath(path,name))};case []any:lines=append(lines,path+" array ("+strconv.Itoa(len(v))+")");for i,child:=range v{walk(child,path+"/"+strconv.Itoa(i))};case nil:lines=append(lines,path+" null");case string:lines=append(lines,path+" string: "+v);case bool:lines=append(lines,path+" boolean: "+strconv.FormatBool(v));case float64:lines=append(lines,path+" number: "+strconv.FormatFloat(v,'g',-1,64))}}
 walk(value,"choice");return lines,nil
}
func(m operatorModel)payloadRows()[]string{
 if m.draft.Payload==nil||m.draft.ReferenceMode||m.draft.Preview!=nil{return nil};rows:=[]string{}
 focuses:=m.draft.Payload.focuses();start:=m.draft.PayloadIndex-2;if start<0{start=0};end:=start+5;if end>len(focuses){end=len(focuses)}
 for i:=start;i<end;i++{focus:=focuses[i];node:=focus.Node;prefix:="  ";if i==m.draft.PayloadIndex&&!m.draft.ChoiceReview{prefix="> "};state:="absent";if node.Present{state=node.Variant};choices:=node.Schema.Constraints.Enum;if node.Present&&choices!=nil{state=operatorPayloadEnumKind(choices[node.EnumIndex],node.Schema)};if node.Required{state+=" required"};line:=prefix+focus.Path+" ["+state+"; "+strings.Join(node.Schema.Types,"|")+"]";if node.Present&&node.Variant=="object"&&choices==nil&&len(node.Schema.Properties)>0{line+=" property: "+node.Schema.Properties[node.PropertyIndex].Name};if node.Present&&choices!=nil{line+=" choice "+strconv.Itoa(node.EnumIndex+1)+"/"+strconv.Itoa(len(choices))};rows=append(rows,line);if i==m.draft.PayloadIndex&&node.Present&&choices!=nil{tree:=operatorPayloadEnumTree(choices[node.EnumIndex]);wrapped:=[]string{};for _,line:=range tree{wrapped=append(wrapped,operatorReviewWrap(line,m.width)...)};offset:=m.draft.ChoiceOffset;if offset<0{offset=0};if offset>=len(wrapped){offset=len(wrapped)-1};end:=offset+4;if end>len(wrapped){end=len(wrapped)};for j:=offset;j<end;j++{line:=wrapped[j];if m.draft.ChoiceReview&&j==offset{line="> "+line};rows=append(rows,line)};rows=append(rows,"Alt+PgUp/PgDn inspect choice; Alt+Home return to field")}}
 return append(rows,"Tab field  Alt+s property  Alt+u presence  Alt+v type  Alt+n choice","Alt+a add item  Alt+d remove item  Alt+i include payload  Alt+p reference  Alt+o omit reference","String fields: Ctrl+n newline  Ctrl+t tab")
}
