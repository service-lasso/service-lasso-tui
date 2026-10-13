package app

import (
 "bytes"
 "testing"

 tea "github.com/charmbracelet/bubbletea"
)

// These source cases exercise command creation, not mocked storage or native
// qualification. A retained record cannot be overwritten by a metadata command
// merely because restoration could not attach authoritative operation state.
func TestOperatorRetainedRecordPreventsDirectDispatchKeepsReadFocus(t *testing.T){
 for _,legacy:=range []bool{false,true}{
  original:=[]byte("retained-original-record")
  m:=operatorModel{saved:append([]byte(nil),original...),legacyRecord:legacy,width:80,height:24,
   draft:operatorDraft{Review:true,Action:"service_meta_patch",Expected:"original-revision",Selector:"favorite",Text:"true"}}
  if cmd:=m.submitDirect();cmd!=nil{t.Fatal("unreconciled retained record created a mutation command")}
  if m.draft.Sent||m.draft.MutationTicket!=nil||!bytes.Equal(m.saved,original){t.Fatal("mutation denial changed retained source or dispatched identity")}
  m.draft=operatorDraft{}
  previousFocus:=m.focus
  updated,cmd:=m.Update(tea.KeyMsg{Type:tea.KeyTab})
  m=updated.(operatorModel)
  if cmd!=nil{t.Fatal("read focus navigation created a command")}
  if m.focus==previousFocus||!bytes.Equal(m.saved,original){t.Fatal("retained record prevented read focus or changed original bytes")}
 }
}

func TestOperatorUnavailableRecordReadDoesNotBecomeEmptyRecordAuthority(t *testing.T){
 m:=operatorModel{restorationBlocked:true,draft:operatorDraft{Review:true,Action:"service_meta_patch",Expected:"original-revision",Selector:"favorite",Text:"true"}}
 if m.saved!=nil{t.Fatal("fixture must represent a failed read with no known raw bytes")}
 if cmd:=m.submitDirect();cmd!=nil||m.draft.Sent{t.Fatal("failed record read was treated as authoritative absence")}
}
