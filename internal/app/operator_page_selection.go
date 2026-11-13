package app

import (
 "time"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// Retained snapshot and form rows never become rows of a different read selection.
// The old page remains intact until a genuine successful replacement read.
type operatorPageSelection struct{Epoch uint64;Service,Filter,Search string;Binding api.OperatorContext `json:"-"`}
func operatorPagedJourney(journey string)bool{switch journey{case "overview","graph","variables","endpoints","health_history","history","inbox","queue","audit","logs":return true};return false}
func operatorBoundReadJourney(journey string)bool{for _,known:=range operatorViews{if known==journey{return true}};return false}
func(m operatorModel)pageSelection()operatorPageSelection{return operatorPageSelection{m.epoch,m.service,m.filter(),m.readSearch(),m.binding}}
func(m operatorModel)pageSelectionCurrent()bool{
 if !operatorBoundReadJourney(m.journey){return true}
 selection,ok:=m.pageSelections[m.journey];return ok&&selection==m.pageSelection()
}
func(m *operatorModel)acceptPageSelection(ticket operatorTicket){
 if !operatorBoundReadJourney(ticket.Journey){return};if m.pageSelections==nil{m.pageSelections=map[string]operatorPageSelection{}}
 m.pageSelections[ticket.Journey]=operatorPageSelection{ticket.Connection,ticket.Service,ticket.Filter,ticket.Search,ticket.Binding}
 delete(m.failedFormSelections,ticket.Journey)
 if ticket.Journey=="declared_actions"{m.declaredReadReview=false;m.declaredReadOffset=0}
}

// A failed current form read retains its original values privately, but those
// values cannot supply a fresh ready label or a new draft. This safe status is
// not a durable original diagnostic capture or measured native capacity.
func(m operatorModel)formReadFailed()bool{selection,ok:=m.failedFormSelections[m.journey];return ok&&selection==m.pageSelection()}
func(m *operatorModel)markFormReadFailed(){if !operatorBoundReadJourney(m.journey)||operatorPagedJourney(m.journey){return};if m.failedFormSelections==nil{m.failedFormSelections=map[string]operatorPageSelection{}};m.failedFormSelections[m.journey]=m.pageSelection()}

// Expiry is checked at the gesture as well as by the periodic tick. Retained
// rows remain available for display, but cannot supply a continuation handle.
func(m operatorModel)pageSelectionExpired(now time.Time)bool{
 expiry:=""
 switch m.journey{case "overview":expiry=m.services.Expiry;case "graph":expiry=m.graph.Expiry;case "variables":expiry=m.variables.Expiry;case "endpoints":expiry=m.endpoints.Expiry;case "health_history":expiry=m.health.Expiry;case "history":expiry=m.history.Expiry;case "inbox":expiry=m.inbox.Expiry;case "queue":expiry=m.queue.Expiry;case "audit":expiry=m.audit.Expiry;case "logs":expiry=m.logs.Expiry}
 return operatorSnapshotExpired(expiry,now)
}
