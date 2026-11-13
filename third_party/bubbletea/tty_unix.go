//go:build darwin || dragonfly || freebsd || linux || netbsd || openbsd || solaris || aix || zos
// +build darwin dragonfly freebsd linux netbsd openbsd solaris aix zos

package tea

import (
	"fmt"
	"os"
	"os/signal"
	"syscall"

	"github.com/charmbracelet/x/term"
)

func (p *Program) initInput() (err error) {
	// Check if input is a terminal
	if f, ok := p.input.(term.File); ok && term.IsTerminal(f.Fd()) {
		p.ttyInput = f
  if p.sourceTerminal!=nil{p.publicRawAttempted=true;p.publicRestoreOriginal=nil;if err=p.sourceError("public-terminal.make-raw",p.sourceTerminal,p.ttyInput,p.sourceTerminal.MakePublicRaw);err!=nil{return fmt.Errorf("error entering public raw mode: %w",err)}}
		raw:=p.sourceStart("terminal.make-raw",p.ttyInput,p.ttyInput.Fd())
 p.previousTtyInputState, err = term.MakeRaw(p.ttyInput.Fd());raw.Output=p.previousTtyInputState;raw.Failure=err;raw.Returned=true
		if err != nil {
			return fmt.Errorf("error entering raw mode: %w", err)
		}
	}

	if f, ok := p.output.(term.File); ok && term.IsTerminal(f.Fd()) {
		p.ttyOutput = f
	}

	return nil
}

func openInputTTY()(*os.File,error){return openInputTTYOwned(nil)}
func openInputTTYOwned(owner *Program) (*os.File, error) {
	if owner!=nil&&owner.sourceTerminal!=nil{if err:=owner.sourceError("public-terminal.default-input-fallback",owner.sourceTerminal,"/dev/tty",owner.sourceTerminal.BeforeFallback);err!=nil{return nil,fmt.Errorf("could not open public TTY: %w",err)}}
 opening:=owner.sourceStart("terminal.os-open",owner,"/dev/tty")
 f, err := os.Open("/dev/tty");opening.Output=f;opening.Failure=err;opening.Returned=true
	if err != nil {
		return nil, fmt.Errorf("could not open a new TTY: %w", err)
	}
	return f, nil
}

const suspendSupported = true

// Send SIGTSTP to the entire process group.
func suspendProcess() {
	c := make(chan os.Signal, 1)
	signal.Notify(c, syscall.SIGCONT)
	_ = syscall.Kill(0, syscall.SIGTSTP)
	// blocks until a CONT happens...
	<-c
}
