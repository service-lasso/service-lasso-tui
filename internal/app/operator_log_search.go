package app

import tea "github.com/charmbracelet/bubbletea"

// Search is a read selection, separate from retained mutation and operation
// custody. This editor issues no source handle, tail lease or mutation grant.
func(m operatorModel)readSearch()string{if m.journey=="logs"{return m.logSearch};return ""}
func(m *operatorModel)logSearchKey(msg tea.KeyMsg)tea.Cmd{
 editor:=m.logSearchDraft;if editor==nil{return nil}
 switch msg.String(){
 case "esc":m.logSearchDraft=nil
 case "enter":m.logSearch=editor.Text;m.invalidate();return m.load(nil)
 case "left":editor.moveCursor(false)
 case "right":editor.moveCursor(true)
 case "home":editor.lineBoundary(false)
 case "end":editor.lineBoundary(true)
 case "delete":editor.erase(false)
 case "backspace":editor.erase(true)
 case "ctrl+u":editor.Text="";editor.Cursor=0
 default:if msg.Type==tea.KeyRunes&&!msg.Alt{editor.insert(string(msg.Runes),256)}else if msg.Type==tea.KeySpace&&!msg.Alt{editor.insert(" ",256)}
 };return nil
}