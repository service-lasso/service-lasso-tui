package app

import(
 "encoding/json"
 "strings"
 "testing"
 tea "github.com/charmbracelet/bubbletea"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

func TestOperatorQuotedControlsPrecedeGlobalDispatch(t *testing.T){
 minimum,maximum:=0,4096
 schema:=&api.OperatorPayloadSchema{Types:[]string{"string"},Constraints:api.OperatorPayloadConstraints{MinLength:&minimum,MaxLength:&maximum},Properties:[]api.OperatorPayloadProperty{},Required:[]string{}}
 closed:=false;root:=&api.OperatorPayloadSchema{Types:[]string{"object"},Constraints:api.OperatorPayloadConstraints{AdditionalProperties:&closed},Properties:[]api.OperatorPayloadProperty{{Name:"text",Schema:*schema}},Required:[]string{"text"}};node:=newOperatorPayloadNode(root,true);if node==nil{t.Fatal("missing typed object") }
 model:=operatorModel{draft:operatorDraft{Action:"declared_action_run",Dirty:true,Payload:node,PayloadIndex:1,InlinePayload:true}}
 keys:=[]tea.KeyMsg{{Type:tea.KeyCtrlC},{Type:tea.KeyEsc},{Type:tea.KeyEnter},{Type:tea.KeyBackspace},{Type:tea.KeyRunes,Runes:[]rune("界👩‍💻")}}
 expected:="\x03\x1b\r\x7f界👩‍💻"
 for _,key:=range keys{model.editKey(tea.KeyMsg{Type:tea.KeyCtrlV});if !model.draft.LiteralNext{t.Fatal("quote not armed")};updated,cmd:=model.Update(key);model=updated.(operatorModel);if cmd!=nil||model.draft.LiteralNext||!model.draft.Dirty{t.Fatal("quoted key dispatched global action")}}
 if model.draft.Text!=expected||node.Properties["text"].Text!=expected{t.Fatal("quoted data changed")}
 shown:=operatorText(model.draft.Text);for _,control:=range []string{"\x03","\x1b","\r","\x7f"}{if strings.Contains(shown,control){t.Fatal("literal terminal control")}}
 model.width=72;model.height=24;shown=model.View();for _,control:=range []string{"\x03","\x1b","\r","\x7f"}{if strings.Contains(shown,control){t.Fatal("actual view retained quoted terminal control")}}
 model.width=1;model.height=1;if model.View()!="▏"{t.Fatal("narrow escaped string lost literal cursor")};if model.draft.Text!=expected{t.Fatal("view copied escaped text back")}
 payload,ok:=node.value();if !ok{t.Fatal("accepted scalar rejected")};input,_:=json.Marshal(api.OperatorDeclaredInput{DeclaredActionID:"action",Payload:payload});wire,_:=json.Marshal(api.OperatorPreviewRequest{ServiceID:"root",ActionID:"declared_action_run",DefinitionRevision:"definition",ExpectedRevision:"revision",Input:input});var body map[string]json.RawMessage;var declared api.OperatorDeclaredInput;var text map[string]string
 if json.Unmarshal(wire,&body)!=nil||len(body)!=4||json.Unmarshal(body["input"],&declared)!=nil||json.Unmarshal(declared.Payload,&text)!=nil||text["text"]!=expected{t.Fatal("preview body changed original scalar")}
 _,quit:=model.Update(tea.KeyMsg{Type:tea.KeyCtrlC});if quit==nil{t.Fatal("unarmed quit lost")}
}

func TestOperatorQuotedSpecialKeyDoesNotNavigate(t *testing.T){
 minimum,maximum:=0,4096;schema:=&api.OperatorPayloadSchema{Types:[]string{"string"},Constraints:api.OperatorPayloadConstraints{MinLength:&minimum,MaxLength:&maximum},Properties:[]api.OperatorPayloadProperty{},Required:[]string{}}
 model:=operatorModel{draft:operatorDraft{Action:"declared_action_run",Dirty:true,Payload:newOperatorPayloadNode(schema,true)}}
 model.editKey(tea.KeyMsg{Type:tea.KeyCtrlV});updated,cmd:=model.Update(tea.KeyMsg{Type:tea.KeyUp});model=updated.(operatorModel);if cmd!=nil||model.draft.Text!=""||model.draft.LiteralNext||model.errorText==""{t.Fatal("special key acquired literal meaning")}
}

func TestOperatorControlEnumRetainsExactChoice(t *testing.T){
 original:="\x00\x1b\r\n\t\u0085\u2029";choice,_:=json.Marshal(original);minimum,maximum:=0,4096
 schema:=&api.OperatorPayloadSchema{Types:[]string{"string"},Constraints:api.OperatorPayloadConstraints{MinLength:&minimum,MaxLength:&maximum,Enum:[]json.RawMessage{choice}},Properties:[]api.OperatorPayloadProperty{},Required:[]string{}}
 node:=newOperatorPayloadNode(schema,true);if node==nil{t.Fatal("control enum rejected")};model:=operatorModel{draft:operatorDraft{Action:"declared_action_run",Dirty:true,Payload:node}}
 model.editKey(tea.KeyMsg{Type:tea.KeyCtrlV});if model.draft.LiteralNext{t.Fatal("enum acquired free text editor")}
 rows:=operatorPayloadEnumTree(choice);shown:=operatorText(strings.Join(rows,"\n"));for _,control:=range []string{"\x00","\x1b","\r","\t","\u0085","\u2029"}{if strings.Contains(shown,control){t.Fatal("enum displayed raw control")}}
 model.width=72;model.height=24;shown=model.View();for _,control:=range []string{"\x00","\x1b","\r","\t","\u0085","\u2029"}{if strings.Contains(shown,control){t.Fatal("actual enum view retained raw control")}}
 value,ok:=node.value();var actual string;if !ok||json.Unmarshal(value,&actual)!=nil||actual!=original{t.Fatal("enum render changed choice")}
}

func TestOperatorRetiredQuoteDoesNotHideQuit(t *testing.T){
 minimum,maximum:=0,4096;schema:=&api.OperatorPayloadSchema{Types:[]string{"string"},Constraints:api.OperatorPayloadConstraints{MinLength:&minimum,MaxLength:&maximum},Properties:[]api.OperatorPayloadProperty{},Required:[]string{}}
 model:=operatorModel{draft:operatorDraft{Action:"declared_action_run",Dirty:true,Payload:newOperatorPayloadNode(schema,true)}}
 model.editKey(tea.KeyMsg{Type:tea.KeyCtrlV});model.loading=true
 updated,cmd:=model.Update(tea.KeyMsg{Type:tea.KeyCtrlC});model=updated.(operatorModel)
 if cmd==nil||model.draft.LiteralNext||model.draft.Text!=""{t.Fatal("retired invisible quote consumed normal quit")}
}
