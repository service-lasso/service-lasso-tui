package app

import (
 "strings"
 "testing"

 tea "github.com/charmbracelet/bubbletea"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// Source regression only: no terminal rendering or native acceptance credit.
func operatorJourneyService(id string)api.OperatorService{return api.OperatorService{ServiceID:id,DisplayName:id,Description:"",DefinitionRevision:"definition",Generation:"generation",ModelEpoch:"epoch",Installed:"yes",Configured:"yes",Running:"no",HealthStatus:"unknown",UpdateStatus:"not_checked",CapabilityIDs:[]string{}}}
func TestOperatorJourneyPositionSurvivesKeyboardNavigation(t *testing.T){
 model:=operatorModel{epoch:1,journey:"overview",focus:1,row:2,scroll:1,width:20,height:4,services:operatorPageState[api.OperatorService]{Rows:[]api.OperatorService{operatorJourneyService("first"),operatorJourneyService("second"),operatorJourneyService("third"),operatorJourneyService("fourth")}}}
 model.acceptPageSelection(model.ticket())
 updated,_:=model.Update(tea.KeyMsg{Type:tea.KeyRight});model=updated.(operatorModel)
 if model.journey!="graph"||model.row!=0{t.Fatal("service-free navigation did not choose graph")}
 updated,_=model.Update(tea.KeyMsg{Type:tea.KeyLeft});model=updated.(operatorModel)
 if model.journey!="overview"||model.row!=2||model.scroll!=1{t.Fatal("return navigation discarded original journey position")}
 updated,_=model.Update(tea.KeyMsg{Type:tea.KeyTab});model=updated.(operatorModel)
 if model.focus!=0{t.Fatal("tab did not select journey rail")}
 updated,_=model.Update(tea.KeyMsg{Type:tea.KeyEnter});model=updated.(operatorModel)
 if model.focus!=1||model.row!=2||model.scroll!=1{t.Fatal("rail selection discarded content position")}
 updated,_=model.Update(tea.KeyMsg{Type:tea.KeyTab});model=updated.(operatorModel)
 updated,_=model.Update(tea.KeyMsg{Type:tea.KeyTab});model=updated.(operatorModel)
 if model.row!=2||model.scroll!=1{t.Fatal("focus round trip discarded content position")}
}

func TestOperatorResizePreservesActualZeroBounds(t *testing.T){
 original:=operatorModel{epoch:1,journey:"overview",focus:1,width:80,height:24,draft:operatorDraft{Dirty:true,Action:"config_patch",Selector:"field",Text:"retained",Cursor:8},custodyWarning:"retained custody"}
 for _,size:=range []tea.WindowSizeMsg{{Width:8,Height:0},{Width:0,Height:3},{Width:-1,Height:-1}}{
  updated,_:=original.Update(size);model:=updated.(operatorModel)
  if model.width<0||model.height<0||size.Width==0&&model.width!=0||size.Height==0&&model.height!=0{t.Fatal("resize widened actual zero bounds")}
  screen:=model.View();if model.height==0&&screen!=""{t.Fatal("zero-height resize emitted content")};if model.width==0&&strings.Trim(screen,"\n")!=""{t.Fatal("zero-width resize emitted cells")}
  if model.draft.Text!="retained"||model.custodyWarning!="retained custody"{t.Fatal("zero-bound resize dropped original draft or custody")}
 }
}

func TestOperatorJourneyPositionRejectsDifferentReadSelection(t *testing.T){
 for _,changed:=range []string{"connection","service","filter","context"}{t.Run(changed,func(t *testing.T){
  model:=operatorModel{epoch:1,journey:"logs",service:"original",binding:selectedOperatorTestContext("a"),focus:1,row:7,scroll:5,filters:map[string]string{"logs":"stdout"}}
  model.rememberJourneyPosition()
  switch changed{case "connection":model.epoch++;case "service":model.service="different";case "filter":model.filters["logs"]="stderr";case "context":model.binding=selectedOperatorTestContext("b")}
  model.restoreJourneyPosition()
  if model.row!=0||model.scroll!=0{t.Fatal("another read selection imported original journey position")}
 })}
}

func TestOperatorJourneyPositionClampsToAuthoritativeRows(t *testing.T){
 model:=operatorModel{epoch:1,journey:"overview",focus:1,row:8,scroll:6}
 page:=api.OperatorPage[api.OperatorService]{SnapshotID:"snapshot",SourceRevision:"revision",ExpiresAt:"2099-10-08T00:00:00.000Z",Rows:[]api.OperatorService{operatorJourneyService("first"),operatorJourneyService("second")}}
 if !page.ValidHeader(25)||!page.Rows[0].Valid()||!page.Rows[1].Valid(){t.Fatal("fixture must use admitted closed service/page values")}
 updated,_:=model.Update(operatorLoaded{Ticket:model.ticket(),Data:page});model=updated.(operatorModel)
 if model.row!=1||model.scroll!=1{t.Fatal("shrinking authoritative rows left unreachable position")}
 page.Rows=[]api.OperatorService{};updated,_=model.Update(operatorLoaded{Ticket:model.ticket(),Data:page});model=updated.(operatorModel)
 if model.row!=0||model.scroll!=0{t.Fatal("genuine empty rows retained unreachable position")}
}

// A current pending read has no authoritative replacement rows yet. Focus must
// preserve the original position until the successful reducer clamps it.
func TestOperatorJourneyPositionSurvivesFocusDuringPendingRead(t *testing.T){
 model:=operatorModel{epoch:1,journey:"overview",focus:1,row:8,scroll:6,loading:true}
 model.rememberJourneyPosition()
 updated,_:=model.Update(tea.KeyMsg{Type:tea.KeyTab});model=updated.(operatorModel)
 updated,_=model.Update(tea.KeyMsg{Type:tea.KeyTab});model=updated.(operatorModel)
 if model.focus!=1||model.row!=8||model.scroll!=6{t.Fatal("pending read focus discarded retained original position")}
 page:=api.OperatorPage[api.OperatorService]{SnapshotID:"snapshot",SourceRevision:"revision",ExpiresAt:"2099-10-08T00:00:00.000Z",Rows:[]api.OperatorService{operatorJourneyService("first"),operatorJourneyService("second")}}
 updated,_=model.Update(operatorLoaded{Ticket:model.ticket(),Data:page});model=updated.(operatorModel)
 if model.loading||model.row!=1||model.scroll!=1{t.Fatal("completed authoritative read did not clamp retained position")}
}