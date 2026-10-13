package app

import (
 tea "github.com/charmbracelet/bubbletea"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// One position per declared journey, bound to its original read selection.
// Changing service, filter or connection never imports another selection's row.
type operatorJourneyPosition struct{Epoch uint64;Service,Filter,Search string;Row,Scroll int;Binding api.OperatorContext `json:"-"`}
func(m *operatorModel)rememberJourneyPosition(){
 if m.focus!=1{return};known:=false;for _,name:=range operatorViews{if name==m.journey{known=true;break}};if !known{return}
 if m.journeyPositions==nil{m.journeyPositions=map[string]operatorJourneyPosition{}}
 m.journeyPositions[m.journey]=operatorJourneyPosition{m.epoch,m.service,m.filter(),m.readSearch(),m.row,m.scroll,m.binding}
}
func(m *operatorModel)restoreJourneyPosition(){
 m.row=0;m.scroll=0
 if position,ok:=m.journeyPositions[m.journey];ok&&position.Epoch==m.epoch&&position.Service==m.service&&position.Filter==m.filter()&&position.Search==m.readSearch()&&position.Binding==m.binding{m.row=position.Row;m.scroll=position.Scroll}
}
func(m *operatorModel)clampJourneyPosition(){
 rows:=m.displayRows();if len(rows)==0{m.row=0;m.scroll=0;return}
 if m.row<0{m.row=0};if m.row>=len(rows){m.row=len(rows)-1};if m.scroll<0{m.scroll=0};if m.scroll>=len(rows){m.scroll=len(rows)-1}
}

func operatorReadCapability(journey string)string{switch journey{case "overview":return "read_overview";case "detail":return "read_detail";case "graph":return "read_graph";case "variables":return "read_variables";case "endpoints":return "read_endpoints";case "health_history":return "read_health_history";case "inbox":return "read_inbox";case "queue":return "read_queue";case "audit":return "read_audit";case "logs":return "read_logs";case "metadata":return "service_meta_read"};return ""}
func operatorReadAdmission(discovery *api.OperatorDiscovery,journey string)(bool,string){
 capability:=operatorReadCapability(journey);if capability==""{return true,""}
 return operatorCapabilityAdmission(discovery,capability)
}
func operatorCapabilityAdmission(discovery *api.OperatorDiscovery,capability string)(bool,string){
 if discovery!=nil{for _,current:=range discovery.Capabilities{if current.ID==capability{if current.Availability.Available{return true,""};if current.Availability.ReasonCode!=nil{return false,*current.Availability.ReasonCode};return false,"unsupported_capability"}}}
 return false,"unsupported_capability"
}
func(m *operatorModel)changeView(right bool)tea.Cmd{
 m.rememberJourneyPosition()
 index:=0;for i,name:=range operatorViews{if name==m.journey{index=i;break}}
 for i:=0;i<len(operatorViews);i++{if right{index=(index+1)%len(operatorViews)}else{index=(index+len(operatorViews)-1)%len(operatorViews)};if m.service!=""||!operatorJourneyNeedsService(operatorViews[index]){break}}
 m.invalidate();m.journey=operatorViews[index];if m.focus==0{m.row=index}else{m.restoreJourneyPosition()};return m.load(nil)
}
func(m *operatorModel)toggleFocus(){m.rememberJourneyPosition();m.focus=(m.focus+1)%2;if m.focus==1{m.restoreJourneyPosition();if !m.loading{m.clampJourneyPosition()};return};m.row=0;m.scroll=0;for i,name:=range operatorViews{if name==m.journey{m.row=i;m.scroll=i;break}}}
func(m operatorModel)displayRows()[]string{if m.focus==0{rows:=[]string{};for _,name:=range operatorViews{label:=name;if m.service==""&&operatorJourneyNeedsService(name){label+=" (choose a service)"};rows=append(rows,label)};return rows};return m.rowLines()}
