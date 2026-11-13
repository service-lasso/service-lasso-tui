package app

import (
 "fmt"
 "strconv"
 "strings"
 "unicode"

 "github.com/charmbracelet/x/ansi"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// Only application-owned layout inserts control bytes. File/log/payload text
// retains literal data in its draft and is escaped at this rendering boundary.
func operatorText(value string)string{var out strings.Builder;for _,r:=range value{if unicode.IsControl(r)||r=='\u2028'||r=='\u2029'{quoted:=strconv.QuoteRune(r);out.WriteString(quoted[1:len(quoted)-1])}else{out.WriteRune(r)}};return out.String()}
func operatorReason(value api.OperatorAvailability)string{
 if value.Available{return "available"};if value.ReasonCode==nil{return "unavailable"}
 switch *value.ReasonCode{
 case "unsupported_capability":return "unsupported"
 case "authentication_required":return "sign in required"
 case "permission_denied":return "permission denied"
 case "confirmation_required":return "confirmation required"
 case "binding_stale":return "context changed"
 case "revision_conflict":return "changed since loaded"
 case "source_unavailable":return "source unavailable"
 case "source_corrupt":return "stored data needs recovery"
 case "dependency_blocked":return "dependencies blocked"
 case "ownership_unknown":return "ownership needs reconciliation"
 case "storage_unavailable":return "storage unavailable"
 case "no_mutation_required":return "already in requested state"
 default:return "unavailable"
 }
}
func operatorLifecycleLabel(id string)string{switch id{
 case "service_install":return "Install"
 case "service_configure":return "Configure"
 case "service_start":return "Start"
 case "service_stop":return "Stop"
 case "service_restart":return "Restart"
 case "service_reload":return "Reload"
 case "update_check":return "Check updates"
 case "update_download":return "Download update"
 default:return "Action"
}}
func(m operatorModel)rowLines()[]string{
 if !m.pageSelectionCurrent(){return nil}
 if m.formReadFailed(){if m.journey=="metadata"{return []string{"favorite=unknown","layout: unknown","revision: unknown"}};return nil}
 rows:=[]string{}
 switch m.journey{
 case "overview":for _,r:=range m.overviewServices(){rows=append(rows,fmt.Sprintf("%s  %s  installed=%s configured=%s running=%s health=%s update=%s favorite=%s",r.ServiceID,r.DisplayName,r.Installed,r.Configured,r.Running,r.HealthStatus,r.UpdateStatus,m.favoriteLabel(r.ServiceID,r.Generation)))}
 case "detail":if m.detail!=nil{r:=m.detail;rows=append(rows,r.Summary.DisplayName,r.Summary.Description,"provider: "+r.Provider.Role+" "+operatorReason(r.Provider.Availability));for _,edge:=range r.Dependencies{rows=append(rows,fmt.Sprintf("%s -> %s [%s %s]",edge.FromServiceID,edge.ToServiceID,edge.Kind,edge.State))}}
 case "graph":for _,node:=range m.graph.Nodes{rows=append(rows,fmt.Sprintf("%s [%s %s] %s layout=%s",node.ServiceID,node.Role,node.Presence,operatorReason(node.Availability),m.layoutLabel(node.ServiceID)));for _,edge:=range m.graph.Edges{if edge.ToServiceID==node.ServiceID{rows=append(rows,fmt.Sprintf("  %s -> %s [%s %s]",edge.FromServiceID,edge.ToServiceID,edge.Kind,edge.State))}}}
 case "variables":for _,r:=range m.variables.Rows{rows=append(rows,fmt.Sprintf("%s [%s] %s",r.Name,r.Scope,r.Resolution))}
 case "endpoints":for _,r:=range m.endpoints.Rows{port:="unknown";if r.Port!=nil{port=strconv.Itoa(*r.Port)};rows=append(rows,fmt.Sprintf("%s %s port=%s %s",r.Label,r.Protocol,port,operatorReason(r.Availability)))}
 case "lifecycle":if m.lifecycle!=nil{for _,r:=range m.lifecycle.Capabilities{rows=append(rows,operatorLifecycleLabel(r.ID)+" "+operatorReason(r.Availability))}}
 case "metadata":if m.meta!=nil{favorite:="unknown";if m.meta.Favorite!=nil{favorite=strconv.FormatBool(*m.meta.Favorite)};rows=append(rows,"favorite="+favorite);if !m.meta.Availability.Available{rows=append(rows,"layout: unknown")}else if m.meta.Position==nil{rows=append(rows,"layout: automatic")}else{rows=append(rows,fmt.Sprintf("layout: %d,%d",m.meta.Position.X,m.meta.Position.Y))};revision:="unknown";if m.meta.SourceRevision!=nil{revision=*m.meta.SourceRevision};rows=append(rows,"revision: "+revision,operatorReason(m.meta.Availability))}
 case "config":if m.config!=nil{for _,f:=range m.config.Fields{rows=append(rows,f.Selector+" ["+f.Type+"] "+operatorReason(f.Availability)+fmt.Sprintf(" readonly=%t",f.Readonly))};for _,c:=range m.config.Catalogs{rows=append(rows,fmt.Sprintf("%s [%s] entries=%d slots=%d %s",c.Selector,c.Type,len(c.Entries),len(c.Slots),operatorReason(c.Availability)))}}
 case "setup":if m.setup!=nil{for _,r:=range m.setup.Steps{status:="no last run";if r.Status!=nil{status=*r.Status};rows=append(rows,r.StepID+" "+r.Description+" ["+status+"] "+operatorReason(r.Availability))}}
 case "declared_actions":if m.actions!=nil{for _,r:=range m.actions.Actions{rows=append(rows,r.ActionID+" "+r.Description+" "+operatorReason(r.Availability))}}
 case "updates":if m.updates!=nil{r:=m.updates;install:="unavailable";if r.Availability.Available&&r.CandidateID!=nil&&r.ExpectedRevision!=nil{install="available"};rows=append(rows,"Check updates","Download update","Install update — "+install,"mode: "+r.Mode,"running service: "+r.RunningService,"check status: "+r.CheckStatus);if r.CandidateID!=nil{rows=append(rows,"candidate: "+*r.CandidateID)};rows=append(rows,operatorReason(r.Availability))}
 case "health_history":for _,r:=range m.health.Rows{rows=append(rows,r.Status+" "+r.Summary)}
 case "history":for _,r:=range m.history.Rows{outcome:="pending";if r.Outcome!=nil{outcome=*r.Outcome};rows=append(rows,r.Kind+" "+r.Status+" "+r.Phase+" "+outcome+" "+r.Summary)}
 case "inbox":for _,r:=range m.inbox.Rows{rows=append(rows,r.ServiceID+" "+r.Kind+" "+r.Status+" "+r.Visibility+" "+r.Summary)}
 case "queue":for _,r:=range m.queue.Rows{rows=append(rows,r.ServiceID+" "+r.Kind+" "+r.Status+" "+r.Summary)}
 case "audit":for _,r:=range m.audit.Rows{rows=append(rows,r.Time+" "+r.Actor+" "+r.Action+" "+r.Status+" ["+r.Integrity+"] "+r.Summary)}
 case "logs":for _,r:=range m.logs.Rows{flag:="";if r.Truncated{flag=" [truncated]"};rows=append(rows,r.Time+" "+r.Stream+" "+r.Text+flag)}
 case "removal":if m.removal!=nil{for _,r:=range m.removal.Categories{rows=append(rows,r.Code+": "+r.Disposition)}}
 };return rows
}
func(m operatorModel)formStatus()(string,bool){
 if !m.pageSelectionCurrent(){return "",false}
 if m.formReadFailed(){return "Form: current read unavailable; retained values need refresh",true}
 var availability *api.OperatorAvailability
 switch m.journey{case "lifecycle":if m.lifecycle!=nil{availability=&m.lifecycle.Availability};case "config":if m.config!=nil{availability=&m.config.Availability};case "setup":if m.setup!=nil{availability=&m.setup.Availability};case "declared_actions":if m.actions!=nil{availability=&m.actions.Availability};case "removal":if m.removal!=nil{availability=&m.removal.Availability}}
 if availability==nil{return "",false};status:="Form: "+operatorReason(*availability);if availability.Available&&len(m.rowLines())==0{status+="; no declared rows"};return status,!availability.Available
}
func(m operatorModel)rowCount()int{return len(m.displayRows())}
func(m operatorModel)View()string{
 focusLine:=-1;editorLine:=-1;custodyLine:=-1;errorLine:=-1;warningLine:=-1;operationLine:=-1;retainedLine:=-1
 lines:=[]string{"Service Lasso  operator/v2",m.journey+"  "+m.service+"  "+m.filter()}
 focus:="Rows";if m.focus==0{focus="Views"};lines=append(lines,"Focus: "+focus+" (Tab changes focus)")
 if m.journey=="overview"{mode:="all";if m.favoritesOnly{mode="favorites and unknown"};lines=append(lines,"Favorites: "+mode+" (F changes local filter)")}
  if m.discovery==nil{lines=append(lines,"Connecting to authenticated operator API...")}
 if m.loading{lines=append(lines,"Loading...")}
 if !m.pageSelectionCurrent(){label:="No read loaded for this selection.";if operatorPagedJourney(m.journey){label="No snapshot loaded for this selection."};lines=append(lines,label)}
 if status:=m.snapshotStatus();status!=""{lines=append(lines,status)}
 if status,unavailable:=m.formStatus();status!=""{if unavailable{errorLine=len(lines);if m.focus==1&&len(m.rowLines())==0{focusLine=errorLine}};lines=append(lines,status)}
 if m.errorText!=""{errorLine=len(lines);lines=append(lines,"Error: "+m.errorText)};if m.warning!=""{warningLine=len(lines);lines=append(lines,"Retained warning: "+m.warning)}
 if m.terminalRefreshWarning!=""{if warningLine<0{warningLine=len(lines)};lines=append(lines,"Original service refresh failed: "+m.terminalRefreshWarning+"; use read refresh to retry.")}
 if m.custodyWarning!=""{custodyLine=len(lines);lines=append(lines,"Storage custody: "+m.custodyWarning)}
 for _,field:=range m.fieldErrors{lines=append(lines,field.Selector+": "+api.OperatorMessage(field.Code))}
 if m.operation!=nil{op:=m.operation.Operation;operationLine=len(lines);lines=append(lines,"Operation "+op.OperationID+" "+op.ActionID+" "+op.Status+"/"+op.Phase,"Targets: "+strings.Join(op.Targets,", "));if m.operation.Result!=nil{lines=append(lines,m.operation.Result.Summary)}}
 if m.terminalDetail!=nil&&m.operation!=nil&&m.terminalDetail.Summary.ServiceID==m.operation.Operation.ServiceID{state:=m.terminalDetail.Summary;lines=append(lines,"Observed original service "+state.ServiceID+": installed="+state.Installed+" configured="+state.Configured+" running="+state.Running)}
 if m.draft.Sent{retainedLine=len(lines);lines=append(lines,"Original request retained: "+m.draft.Action+". Read-only navigation; do not replay.")}else if m.draft.Retired{retainedLine=len(lines);lines=append(lines,"Previous request was denied before dispatch; its original draft is retained read-only.")}else if m.draft.Dirty||m.draft.Preview!=nil{lines=append(lines,"Draft: "+m.draft.Action,m.editorLine());editorLine=len(lines)-1;focusLine=editorLine;payloadRows:=m.payloadRows();for i,line:=range payloadRows{if strings.HasPrefix(line,"> "){focusLine=len(lines)+i;if m.draft.ChoiceReview{payloadRows[i]=strings.TrimPrefix(line,"> ")};break}};lines=append(lines,payloadRows...);if m.draft.ChoiceReview&&m.draft.Preview==nil{editorLine=-1};lines=append(lines,m.payloadReferenceRows()...);if m.draft.Preview!=nil{review:=m.previewRows();if m.draft.PreviewReview&&len(review)>0{offset:=m.draft.PreviewOffset;if offset<0{offset=0};if offset>=len(review){offset=len(review)-1};focusLine=len(lines)+offset;editorLine=-1};lines=append(lines,review...)}}
 if m.draft.Action=="setup_step_run"&&m.draft.Dirty&&!m.draft.Sent{
  if m.draft.SetupReview&&m.draft.Preview==nil{review:=m.setupOptionRows();offset:=m.draft.SetupOffset;if offset<0{offset=0};if offset>=len(review){offset=len(review)-1};focusLine=len(lines)+offset;editorLine=-1;lines=append(lines,review...)}else{lines=append(lines,fmt.Sprintf("Setup options: force=%t include manual=%t",m.draft.Force,m.draft.IncludeManual),"Ctrl+f toggle force  Alt+m toggle manual steps","Alt+PgUp/PgDn review setup options; Alt+Home return to field")}
 }
 if m.draft.Payload==nil&&m.literalEditorInputAllowed(){lines=append(lines,"File content: Ctrl+n newline  Ctrl+t tab")}
 if m.quotedPayloadInputAllowed(){hint:="String content: Ctrl+v quote next character  Ctrl+n newline  Ctrl+t tab";if m.draft.LiteralNext{hint="Quoted input armed: next character or control key becomes string data."};lines=append(lines,hint)}
 rows:=m.displayRows();for i:=0;i<len(rows);i++{prefix:="  ";if i==m.row{prefix="> ";if editorLine<0&&!m.draft.PreviewReview&&!m.draft.SetupReview&&!(m.draft.ChoiceReview&&m.draft.Preview==nil){focusLine=len(lines)}};lines=append(lines,prefix+rows[i])}
 if m.nextCursor()!=nil{lines=append(lines,"More rows: n. Current view is incomplete.")}
 if m.draft.Action=="config_patch"&&m.draft.Dirty&&m.draft.Preview==nil&&!m.draft.Sent{lines=append(lines,fmt.Sprintf("Staged configuration changes: %d",len(m.draft.ConfigOperations)),"Alt+Enter stage field  Alt+f next field  Alt+k add collection entry  Enter review all changes");for _,operation:=range m.draft.ConfigOperations{lines=append(lines,operation.Op+" "+operation.Selector)};if catalog:=m.draft.Catalog;catalog!=nil&&len(catalog.Slots)>0{slot:=catalog.Slots[m.draft.SlotIndex%len(catalog.Slots)];lines=append(lines,fmt.Sprintf("Collection slot %d of %d (Alt+s)",m.draft.SlotIndex%len(catalog.Slots)+1,len(catalog.Slots)));if len(slot.Targets)>0{target:=slot.Targets[m.draft.TargetIndex%len(slot.Targets)];lines=append(lines,"Target: "+target.TargetHandle+" (Alt+t)");if len(target.SourceHandles)>0{lines=append(lines,"Template source: "+target.SourceHandles[m.draft.SourceIndex%len(target.SourceHandles)]+" (Alt+r)")}}}}
 if entry:=m.draft.EditEntry;entry!=nil&&len(entry.EditTargets)>0&&!m.draft.Sent&&m.draft.Preview==nil{target:=entry.EditTargets[m.draft.TargetIndex%len(entry.EditTargets)];lines=append(lines,"Replacement target: "+target.TargetHandle+" (Alt+t)",operatorReason(target.Availability));if len(target.SourceHandles)>0{lines=append(lines,"Replacement template source: "+target.SourceHandles[m.draft.SourceIndex%len(target.SourceHandles)]+" (Alt+r)")}}
 if m.journey=="logs"{lines=append(lines,"Log search: "+m.logSearch+" (/ edit, Enter apply, Escape retain, empty clears)");if m.logSearchDraft!=nil{copy:=m;copy.draft=*m.logSearchDraft;editorLine=len(lines);focusLine=editorLine;lines=append(lines,copy.editorLine())}}
 if m.declaredInspectionCurrent(){review:=m.declaredInspectionRows();if len(review)>0{offset:=m.declaredReadOffset;if offset<0{offset=0};if offset>=len(review){offset=len(review)-1};focusLine=len(lines)+offset;lines=append(lines,review...)}}else if m.declaredInspectionAction()!=nil{lines=append(lines,"Alt+PgUp/PgDn inspect declared schema and reference labels (read-only)")}
 lines=append(lines,"←/→ view  ↑/↓ select  Enter/e edit  r refresh  n next  c cancel  x clear settled  q quit")
 lines=operatorViewport(lines,m.height,focusLine,[]int{custodyLine,editorLine,warningLine,errorLine,retainedLine,operationLine,0,1,2,len(lines)-1});for i,line:=range lines{lines[i]=ansi.Truncate(operatorText(line),m.width,"")};return strings.Join(lines,"\n")
}
