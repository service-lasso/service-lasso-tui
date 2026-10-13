package app

import (
 "github.com/charmbracelet/x/ansi"
 "github.com/rivo/uniseg"
)

func operatorBoundaries(text string)[]int{values:=[]int{0};g:=uniseg.NewGraphemes(text);for g.Next(){_,end:=g.Positions();values=append(values,end)};return values}
func(d *operatorDraft)boundCursor(){if d.Cursor<0{d.Cursor=0};if d.Cursor>len(d.Text){d.Cursor=len(d.Text)};last:=0;for _,boundary:=range operatorBoundaries(d.Text){if boundary>d.Cursor{break};last=boundary};d.Cursor=last}
func(m *operatorModel)setEditorText(text string){m.draft.Text=text;m.draft.Cursor=len(text)}
func(d *operatorDraft)insert(text string,maximum int){d.boundCursor();if len(d.Text)+len(text)>maximum{return};d.Text=d.Text[:d.Cursor]+text+d.Text[d.Cursor:];d.Cursor+=len(text);for _,boundary:=range operatorBoundaries(d.Text){if boundary>=d.Cursor{d.Cursor=boundary;break}}}
func(d *operatorDraft)moveCursor(right bool){d.boundCursor();before:=0;for _,boundary:=range operatorBoundaries(d.Text){if right&&boundary>d.Cursor{d.Cursor=boundary;return};if !right&&boundary>=d.Cursor{d.Cursor=before;return};before=boundary}}
func(d *operatorDraft)erase(backward bool){d.boundCursor();old:=d.Cursor;if backward{d.moveCursor(false);d.Text=d.Text[:d.Cursor]+d.Text[old:]}else{d.moveCursor(true);end:=d.Cursor;d.Cursor=old;d.Text=d.Text[:old]+d.Text[end:]}}
type operatorTextLine struct{start,end int}
func operatorTextLines(text string)[]operatorTextLine{lines:=[]operatorTextLine{};start:=0;g:=uniseg.NewGraphemes(text);for g.Next(){from,to:=g.Positions();cluster:=text[from:to];if cluster=="\n"||cluster=="\r\n"{lines=append(lines,operatorTextLine{start,from});start=to}};return append(lines,operatorTextLine{start,len(text)})}
func(d *operatorDraft)lineIndex(lines []operatorTextLine)int{for i,line:=range lines{if d.Cursor<=line.end{return i}};return len(lines)-1}
func(d *operatorDraft)lineBoundary(end bool){d.boundCursor();lines:=operatorTextLines(d.Text);line:=lines[d.lineIndex(lines)];if end{d.Cursor=line.end}else{d.Cursor=line.start}}
func(d *operatorDraft)moveLine(down bool){
 d.boundCursor();lines:=operatorTextLines(d.Text);index:=d.lineIndex(lines);column:=ansi.StringWidth(operatorText(d.Text[lines[index].start:d.Cursor]));if down{index++}else{index--};if index<0||index>=len(lines){return};targetStart,targetEnd:=lines[index].start,lines[index].end
 d.Cursor=targetStart;g:=uniseg.NewGraphemes(d.Text[targetStart:targetEnd]);width:=0;for g.Next(){from,end:=g.Positions();next:=width+ansi.StringWidth(operatorText(d.Text[targetStart+from:targetStart+end]));if next>column{break};d.Cursor=targetStart+end;width=next}
}
func(m operatorModel)editorLine()string{
 draft:=m.draft;draft.boundCursor();before:=operatorText(draft.Text[:draft.Cursor]);after:=operatorText(draft.Text[draft.Cursor:]);label:=operatorText(draft.Selector)+": ";budget:=m.width-ansi.StringWidth(label)-1;if budget<1{label="";budget=m.width-1};if budget<0{budget=0}
 if ansi.StringWidth(before)>budget{before=ansi.TruncateLeft(before,ansi.StringWidth(before)-budget,"")};remaining:=budget-ansi.StringWidth(before);if remaining<0{remaining=0};return label+before+"▏"+ansi.Truncate(after,remaining,"")
}
