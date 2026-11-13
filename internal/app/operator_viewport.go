package app

// Layout works with original application lines. Escaping and cell truncation
// remain the final View boundary; this never writes displayed text into a draft.
func operatorViewport(lines []string, height, focus int, pinned []int) []string {
 if height <= 0 || len(lines) == 0 { return nil }
 if len(lines) <= height { return lines }
 if focus < 0 || focus >= len(lines) { focus = 0 }
 if height == 1 {
  // A retained custody warning or the literal editor cursor is useful even
  // when there is no room for a title. Otherwise keep the selected row.
  for i:=0;i<len(pinned)&&i<2;i++ { if pinned[i]>=0&&pinned[i]<len(lines){return []string{lines[pinned[i]]}} }
  return []string{lines[focus]}
 }
 out:=make([]string,0,height);used:=make(map[int]bool)
 for _,index:=range pinned {
  if len(out)>=height-1 { break }
  if index<0||index>=len(lines)||index==focus||used[index] { continue }
  used[index]=true;out=append(out,lines[index])
 }
 body:=make([]string,0,len(lines));bodyFocus:=0
 for index,line:=range lines {
  if used[index] { continue }
  if index==focus { bodyFocus=len(body) }
  body=append(body,line)
 }
 room:=height-len(out);start:=bodyFocus-room/2
 if start<0 { start=0 };if start+room>len(body){start=len(body)-room};if start<0{start=0}
 end:=start+room;if end>len(body){end=len(body)}
 return append(out,body[start:end]...)
}
