package app

import (
 "reflect"
 "strings"
 "testing"

 tea "github.com/charmbracelet/bubbletea"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

func TestOperatorPreviewReviewNarrowOriginalRetention(t *testing.T){
 phrase:="confirm service-start 0123456789abcdef0123456789abcdef";confirmationID:="confirmation";expiry:="2026-10-08T00:00:00.000Z"
 preview:=api.OperatorPreview{PlanID:"plan",ServiceID:"first-service",ActionID:"service_start",DefinitionRevision:"definition",ExpectedRevision:"family revision",Generation:"generation",ModelEpoch:"epoch",Targets:[]string{"first-service","second-service"},Effects:[]string{"start_runtime","stop_runtime"},Availability:api.OperatorAvailability{Supported:true,Available:true},Confirmation:api.OperatorConfirmation{Required:true,ID:&confirmationID,PhraseKey:&phrase,ExpiresAt:&expiry},ExpiresAt:expiry}
 if !api.OperatorConfirmationResponseValid(preview,phrase){t.Fatal("genuine closed preview fixture rejected")}
 original:=preview;original.Targets=append([]string(nil),preview.Targets...);original.Effects=append([]string(nil),preview.Effects...)
 m:=operatorModel{width:4,height:1,draft:operatorDraft{Dirty:true,Preview:&preview,Text:"exact response",Input:[]byte("original input")}}
 rows:=m.previewRows();joined:=strings.Join(rows,"");if !strings.Contains(joined,"first-service")||!strings.Contains(joined,"second-service")||!strings.Contains(joined,phrase){t.Fatal("narrow review lost original targets or phrase")}
 for _,row:=range rows{if len(row)>4{t.Fatal("ASCII review row exceeds narrow width")}}
 for i:=0;i<len(rows)+1;i++{m.editKey(tea.KeyMsg{Type:tea.KeyPgDown,Alt:true})};if m.draft.PreviewOffset!=len(rows)-1||!m.draft.PreviewReview{t.Fatal("narrow paging failed to reach final original row")}
 if !reflect.DeepEqual(preview,original)||m.draft.Text!="exact response"||string(m.draft.Input)!="original input"{t.Fatal("review rendering changed original authority or confirmation")}
 m.width=80;m.movePreviewReview(false);if m.draft.PreviewOffset<0||m.draft.PreviewOffset>=len(m.previewRows()){t.Fatal("resize left review focus outside current rows")}
}
func TestOperatorReviewWrapSafeControlsAndWideGraphemes(t *testing.T){
 original:="界👩‍💻\x1b\n";rows:=operatorReviewWrap(original,1);for _,row:=range rows{if strings.ContainsAny(row,"\x1b\n"){t.Fatal("literal control reached terminal review")}}
 if !strings.Contains(strings.Join(rows,""),"\\u{754c}"){t.Fatal("one-cell view omitted wide grapheme instead of explicit display escape")}
 if original!="界👩‍💻\x1b\n"{t.Fatal("rendering changed original data")}
}
