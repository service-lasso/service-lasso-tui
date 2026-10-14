//go:build linux

package tea

import("os";"strconv";"reflect";"testing";"github.com/charmbracelet/x/term";"golang.org/x/sys/unix")

// UNRUN native regression: the real private PTY restore must happen despite a
// later public-channel panic. This fixture is no product native admission.
func TestPublicRestorePanicStillRestoresActualPrivateTerminal(t *testing.T){
 master,err:=os.OpenFile("/dev/ptmx",os.O_RDWR|unix.O_NOCTTY,0);if err!=nil{t.Fatal(err)};defer master.Close()
 if err=unix.IoctlSetPointerInt(int(master.Fd()),unix.TIOCSPTLCK,0);err!=nil{t.Fatal(err)}
 number,err:=unix.IoctlGetInt(int(master.Fd()),unix.TIOCGPTN);if err!=nil{t.Fatal(err)}
 slave,err:=os.OpenFile("/dev/pts/"+strconv.Itoa(number),os.O_RDWR|unix.O_NOCTTY,0);if err!=nil{t.Fatal(err)};defer slave.Close()
 originalState,err:=term.MakeRaw(slave.Fd());if err!=nil{t.Fatal(err)}
 originalPanic:=new(int);owner:=&sourceTerminalFixture{original:originalPanic}
 p:=&Program{sourceTerminal:owner,publicRawAttempted:true,ttyInput:slave,previousTtyInputState:originalState}
 func(){defer func(){if recover()!=originalPanic{t.Fatal("SAME public panic lost")}}();p.restoreInput()}()
 restored,err:=term.GetState(slave.Fd());if err!=nil{t.Fatal(err)}
 if !reflect.DeepEqual(restored,originalState){t.Fatal("public panic skipped actual private restoration")}
 var private *SourceOccurrence
 for occurrence:=p.sourceOwnership.head;occurrence!=nil;occurrence=occurrence.next{if occurrence.Source=="terminal-input.restore"{private=occurrence}}
 if private==nil||!private.Returned||private.Failure!=nil||private.Invocation==nil||p.publicRestoreOriginal.Returned||owner.restores!=1{t.Fatal("independent private/public originals lost")}
}