package app

import (
 "fmt"
 "strconv"

 "github.com/rivo/uniseg"
)

func (m operatorModel) previewRows() []string {
 p:=m.draft.Preview;if p==nil{return nil}
 original:=[]string{"Original preview targets:"};for _,target:=range p.Targets{original=append(original,"  "+target)};if len(p.Targets)==0{original=append(original,"  none")}
 original=append(original,"Original effects:");for _,effect:=range p.Effects{original=append(original,"  "+effect)};if len(p.Effects)==0{original=append(original,"  none")}
 original=append(original,"Confirmation required: "+strconv.FormatBool(p.Confirmation.Required));if p.Confirmation.PhraseKey!=nil{original=append(original,"Confirmation phrase: "+*p.Confirmation.PhraseKey)};if p.Confirmation.ExpiresAt!=nil{original=append(original,"Confirmation expires: "+*p.Confirmation.ExpiresAt)};original=append(original,"Preview expires: "+p.ExpiresAt,"Alt+PgUp/PgDn review; Alt+Home return to confirmation")
 rows:=[]string{};for _,line:=range original{rows=append(rows,operatorReviewWrap(line,m.width)...)};return rows
}
func operatorReviewWrap(original string,width int) []string {
 if width<1{width=1};text:=operatorText(original);rows:=[]string{};line:="";cells:=0;graphemes:=uniseg.NewGraphemes(text)
 for graphemes.Next(){part:=graphemes.Str();size:=uniseg.StringWidth(part)
  if size>width{escaped:="";for _,r:=range part{escaped+=fmt.Sprintf("\\u{%x}",r)};for _,r:=range escaped{if cells==width{rows=append(rows,line);line="";cells=0};line+=string(r);cells++};continue}
  if cells+size>width{rows=append(rows,line);line="";cells=0};line+=part;cells+=size
 };if line!=""||len(rows)==0{rows=append(rows,line)};return rows
}
func (m *operatorModel) movePreviewReview(down bool) {
 rows:=m.previewRows();if len(rows)==0{return};if m.draft.PreviewOffset<0{m.draft.PreviewOffset=0};if m.draft.PreviewOffset>=len(rows){m.draft.PreviewOffset=len(rows)-1};step:=m.height-2;if step<1{step=1};m.draft.PreviewReview=true
 if down{remaining:=len(rows)-1-m.draft.PreviewOffset;if remaining<step{step=remaining};m.draft.PreviewOffset+=step}else{if m.draft.PreviewOffset<step{step=m.draft.PreviewOffset};m.draft.PreviewOffset-=step}
}

func(m operatorModel)setupOptionRows()[]string{
 original:=[]string{"force="+strconv.FormatBool(m.draft.Force),"include manual="+strconv.FormatBool(m.draft.IncludeManual),"Ctrl+f toggle force  Alt+m toggle manual steps","Alt+PgUp/PgDn review setup options; Alt+Home return to field"}
 rows:=[]string{};for _,line:=range original{rows=append(rows,operatorReviewWrap(line,m.width)...)};return rows
}
func(m *operatorModel)moveSetupReview(down bool){
 rows:=m.setupOptionRows();if len(rows)==0{return};if m.draft.SetupOffset<0{m.draft.SetupOffset=0};if m.draft.SetupOffset>=len(rows){m.draft.SetupOffset=len(rows)-1};step:=m.height-2;if step<1{step=1};m.draft.SetupReview=true
 if down{remaining:=len(rows)-1-m.draft.SetupOffset;if remaining<step{step=remaining};m.draft.SetupOffset+=step}else{if m.draft.SetupOffset<step{step=m.draft.SetupOffset};m.draft.SetupOffset-=step}
}