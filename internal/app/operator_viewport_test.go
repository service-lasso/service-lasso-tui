package app

import (
 "strings"
 "testing"
 tea "github.com/charmbracelet/bubbletea"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

func TestOperatorViewportKeepsSelectedRowInShortTerminal(t *testing.T) {
 lines:=[]string{"title","view","focus","loading","row A","row B","row C","help"}
 for _,height:=range []int{1,2,3,4,6} {
  got:=operatorViewport(lines,height,6,[]int{-1,-1,0,1,2,7})
  if len(got)>height||!strings.Contains(strings.Join(got,"\n"),"row C") {t.Fatalf("height %d lost selected row: %q",height,got)}
 }
 if got:=operatorViewport(lines,0,6,nil);len(got)!=0{t.Fatal("zero height must not render or panic")}
}

func TestOperatorViewportRetainsCustodyAndEditorBeforeTitle(t *testing.T) {
 lines:=[]string{"title","view","Storage custody: unresolved","field: value▏","payload sibling","payload selected","help"}
 got:=operatorViewport(lines,3,5,[]int{2,3,0,1,6})
 if strings.Join(got,"\n")!="Storage custody: unresolved\nfield: value▏\npayload selected"{t.Fatalf("lost original custody/editor/focus: %q",got)}
 if got:=operatorViewport(lines,1,5,[]int{2,3,0});len(got)!=1||got[0]!=lines[2]{t.Fatal("one-line blocked view must retain custody warning")}
}

func TestOperatorViewNarrowEditorPreservesLiteralDraft(t *testing.T) {
 literal:="A\x00\r\n\t\u2029👩‍💻"
 m:=operatorModel{width:80,height:1,draft:operatorDraft{Dirty:true,Action:"config_patch",Selector:"name",Text:literal,Cursor:len(literal)}}
 rendered:=m.View()
 if !strings.Contains(rendered,"▏")||strings.ContainsAny(rendered,"\x00\r\t\u2029"){t.Fatalf("narrow editor lost cursor or emitted literal controls: %q",rendered)}
 if m.draft.Text!=literal||m.draft.Cursor!=len(literal){t.Fatal("layout must not copy escaped display into original draft")}
}

func TestOperatorSetupOptionsRemainDiscoverableInNarrowReview(t *testing.T){
 literal:="original setup draft"
 for _,width:=range []int{1,2,4,12}{
  model:=operatorModel{width:width,height:1,draft:operatorDraft{Dirty:true,Action:"setup_step_run",Text:literal,Cursor:len(literal),Force:true,IncludeManual:true}}
  up:=tea.KeyMsg{Type:tea.KeyPgUp,Alt:true};down:=tea.KeyMsg{Type:tea.KeyPgDown,Alt:true}
  updated,command:=model.Update(up);model=updated.(operatorModel);if command!=nil||!model.draft.SetupReview{t.Fatal("existing review key did not enter setup options")}
  seen:=""
  for n:=0;n<64;n++{
   rendered:=model.View();if strings.Contains(rendered,"\n"){t.Fatal("one-line setup review exceeded actual height")};seen+=rendered
   if strings.Contains(seen,"force=true")&&strings.Contains(seen,"include manual=true"){break}
   updated,command=model.Update(down);model=updated.(operatorModel);if command!=nil{t.Fatal("setup option inspection created a request")}
  }
  if !strings.Contains(seen,"force=true")||!strings.Contains(seen,"include manual=true"){t.Fatalf("width %d hid a setup flag through full keyboard review",width)}
  updated,_=model.Update(tea.KeyMsg{Type:tea.KeyRunes,Runes:[]rune("replace")});model=updated.(operatorModel)
  if model.draft.Text!=literal||model.draft.Cursor!=len(literal){t.Fatal("option review changed hidden original draft")}
  updated,_=model.Update(tea.KeyMsg{Type:tea.KeyHome,Alt:true});model=updated.(operatorModel)
  if model.draft.SetupReview||model.draft.Text!=literal{t.Fatal("return-to-field lost original setup draft")}
 }
 model:=operatorModel{width:80,height:2,custodyWarning:"retained original custody",draft:operatorDraft{Dirty:true,Action:"setup_step_run",SetupReview:true,Force:true}}
 rendered:=model.View();if !strings.Contains(rendered,"retained original custody")||!strings.Contains(rendered,"force=true"){t.Fatal("setup review replaced retained custody priority or focused flag")}
}
func TestOperatorSetupOptionReviewKeepsFocusWithActualSetupRowsAndResize(t *testing.T){
 ready:=api.OperatorAvailability{Supported:true,Available:true};revision:="revision"
 form:=api.OperatorSetupForm{ServiceID:"root",DefinitionRevision:revision,Availability:ready,Steps:[]api.OperatorSetupStep{{StepID:"step",Description:"Available setup step",Rerun:"always",DefinitionRevision:revision,InputFields:[]api.OperatorField{},Availability:ready,ExpectedRevision:&revision}}}
 model:=operatorModel{journey:"setup",service:"root",focus:1,row:0,width:20,height:1,setup:&form,draft:operatorDraft{Dirty:true,Action:"setup_step_run",StepID:"step",Force:true,IncludeManual:true,Text:"original",Cursor:8}}
 model.acceptPageSelection(model.ticket())
 if len(model.displayRows())!=1{t.Fatal("fixture is not the actual nonempty setup row pane")}
 updated,_:=model.Update(tea.KeyMsg{Type:tea.KeyPgUp,Alt:true});model=updated.(operatorModel)
 if model.View()!="force=true"{t.Fatal("selected setup row overwrote explicit option review focus")}
 model.draft.SetupOffset=9999
 updated,_=model.Update(tea.WindowSizeMsg{Width:4,Height:1});model=updated.(operatorModel)
 if !model.draft.SetupReview||model.draft.SetupOffset!=0||model.View()!="forc"||model.draft.Text!="original"||model.draft.Cursor!=8{t.Fatal("resize lost current review mode, original draft or start of rewrapped options")}
 model.custodyWarning="retained custody";updated,_=model.Update(tea.WindowSizeMsg{Width:80,Height:2});model=updated.(operatorModel)
 rendered:=model.View();if !strings.Contains(rendered,"retained custody")||!strings.Contains(rendered,"force=true")||strings.Contains(rendered,"Available setup step"){t.Fatal("actual row replaced focused flag or retained custody at narrow height")}
 updated,_=model.Update(tea.KeyMsg{Type:tea.KeyHome,Alt:true});model=updated.(operatorModel)
 if model.draft.SetupReview||model.draft.Text!="original"||model.draft.Cursor!=8{t.Fatal("return-to-field lost setup field custody")}
}