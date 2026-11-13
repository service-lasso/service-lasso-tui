package app

import "testing"

func TestOperatorVerticalCursorUsesEscapedDisplayCells(t *testing.T){
 original:="\x1b界\r\n12345678界";draft:=operatorDraft{Text:original,Cursor:len("\x1b界")};draft.moveLine(true)
 // ESC renders as the four literal cells \x1b and the wide character as two.
 // The second logical row cursor must land after six ASCII cells.
 expected:=len("\x1b界\r\n123456");if draft.Cursor!=expected{t.Fatal("vertical cursor used raw control width instead of escaped display width")};draft.moveLine(false)
 if draft.Cursor!=len("\x1b界")||draft.Text!=original{t.Fatal("vertical cursor changed original bytes or escaped text copied back")}
}
