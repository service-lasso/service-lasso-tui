package app

import (
 "testing"
 "unicode/utf8"
 tea "github.com/charmbracelet/bubbletea"
)

// Actual compatibility reducer source cases; no native terminal acceptance.
func TestCompatibilitySearchBackspacePreservesGraphemesAndCustody(t *testing.T){
 for _,cluster:=range []string{"é","e\u0301","界","👩‍💻"}{
  retained:=&operationSubmission{id:17,uncertainWithoutID:true}
  original:=model{searching:true,search:"keep"+cluster,operation:retained,selectedServiceID:"original",connectionEpoch:31}
  next,command:=original.Update(tea.KeyMsg{Type:tea.KeyBackspace});current:=next.(model)
  if command!=nil||current.search!="keep"||!utf8.ValidString(current.search)||current.operation!=retained||current.selectedServiceID!=original.selectedServiceID||current.connectionEpoch!=31{t.Fatal("actual Backspace split a grapheme or changed retained custody")}
 }
 original:=model{searching:true,search:""};next,command:=original.Update(tea.KeyMsg{Type:tea.KeyBackspace});if command!=nil||next.(model).search!=""{t.Fatal("empty Backspace changed state")}
}