package app

import tea "github.com/charmbracelet/bubbletea"

func(m operatorModel)quotedPayloadInputAllowed()bool{
 return !m.loading&&!m.custodyBlocked&&!m.draft.Retired&&m.draft.Dirty&&m.draft.Action=="declared_action_run"&&m.draft.Payload!=nil&&m.literalEditorInputAllowed()
}

// An explicit quote consumes one key before global quit/cancel/navigation.
// Bubble Tea's C0 key types are their original scalar values; special key
// sequences and Alt modifiers have no single literal scalar interpretation.
func(m *operatorModel)consumeQuotedPayloadKey(msg tea.KeyMsg)bool{
 if !m.draft.LiteralNext{return false};m.draft.LiteralNext=false
 // Async preview/loading/custody changes can retire the editable gesture.
 // A hidden, stale arm must not consume the normal global key behavior.
 if !m.quotedPayloadInputAllowed(){return false}
 text:=""
 if !msg.Alt{switch{case msg.Type>=0&&msg.Type<=31||msg.Type==127:text=string(rune(msg.Type));case msg.Type==tea.KeyRunes:text=string(msg.Runes);case msg.Type==tea.KeySpace:text=" "}}
 if text==""{m.errorText="Quote a character or control key; special navigation keys are not text.";return true}
 m.insertEditorText(text);m.savePayloadText();return true
}
