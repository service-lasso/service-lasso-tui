package tea

import("errors";"testing")
// Fixture checks source call/once/original retention only; no native behavior.
type sourceTerminalFixture struct{restores int;failure error;original any}
func(*sourceTerminalFixture)BeforeFallback()error{return nil}
func(*sourceTerminalFixture)MakePublicRaw()error{return nil}
func(*sourceTerminalFixture)PublicSize()(int,int,error){return 80,24,nil}
func(f *sourceTerminalFixture)RestorePublicInput()error{f.restores++;if f.original!=nil{panic(f.original)};return f.failure}
func TestPublicRestoreFailureIsSameOnceAttempt(t *testing.T){
 original:=errors.New("same restore original")
 owner:=&sourceTerminalFixture{failure:original}
 p:=&Program{sourceTerminal:owner,publicRawAttempted:true}
 first:=p.restoreInput();second:=p.restoreInput()
 if !errors.Is(first,original)||!errors.Is(second,original)||owner.restores!=1||p.publicRestoreOriginal.Failure!=original||!p.publicRestoreOriginal.Returned||!p.publicRawAttempted{t.Fatal("failed public restore was retried/discarded")}
}
func TestInterruptedPublicRestoreNeverRepeatsNativeInvocation(t *testing.T){
 original:=new(int);owner:=&sourceTerminalFixture{original:original}
 p:=&Program{sourceTerminal:owner,publicRawAttempted:true}
 func(){defer func(){if recover()!=original{t.Fatal("original panic lost")}}();p.restoreInput()}()
 if !errors.Is(p.restoreInput(),ErrProgramKilled)||owner.restores!=1||p.publicRestoreOriginal.Returned||p.publicRestoreOriginal.Invocation==nil||!p.publicRawAttempted{t.Fatal("interrupted public restore repeated/claimed returned")}
}