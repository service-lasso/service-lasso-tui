//go:build windows
// +build windows

package tea

import (
	"fmt"
	"os"

	"github.com/charmbracelet/x/term"
	"golang.org/x/sys/windows"
)

func (p *Program) initInput() (err error) {
	// Save stdin state and enable VT input
	// We also need to enable VT
	// input here.
	if f, ok := p.input.(term.File); ok && term.IsTerminal(f.Fd()) {
		p.ttyInput = f
		raw:=p.sourceStart("terminal.make-raw",p.ttyInput,p.ttyInput.Fd())
 p.previousTtyInputState, err = term.MakeRaw(p.ttyInput.Fd());raw.Output=p.previousTtyInputState;raw.Failure=err;raw.Returned=true
		if err != nil {
			return fmt.Errorf("error making raw: %w", err)
		}

		// Enable VT input
		var mode uint32
		getMode:=p.sourceStart("terminal.get-input-mode",p.ttyInput,&mode)
 modeError:=windows.GetConsoleMode(windows.Handle(p.ttyInput.Fd()),&mode);getMode.Output=mode;getMode.Failure=modeError;getMode.Returned=true
 if err:=modeError;err!=nil{
			return fmt.Errorf("error getting console mode: %w", err)
		}

		if err := p.sourceError("terminal.set-input-mode",p.ttyInput,mode|windows.ENABLE_VIRTUAL_TERMINAL_INPUT,func()error{return windows.SetConsoleMode(windows.Handle(p.ttyInput.Fd()),mode|windows.ENABLE_VIRTUAL_TERMINAL_INPUT)}); err != nil {
			return fmt.Errorf("error setting console mode: %w", err)
		}
	}

	// Save output screen buffer state and enable VT processing.
	if f, ok := p.output.(term.File); ok && term.IsTerminal(f.Fd()) {
		p.ttyOutput = f
		state:=p.sourceStart("terminal.get-output-state",f,f.Fd())
 p.previousOutputState, err = term.GetState(f.Fd());state.Output=p.previousOutputState;state.Failure=err;state.Returned=true
		if err != nil {
			return fmt.Errorf("error getting state: %w", err)
		}

		var mode uint32
		getMode:=p.sourceStart("terminal.get-output-mode",p.ttyOutput,&mode)
 modeError:=windows.GetConsoleMode(windows.Handle(p.ttyOutput.Fd()),&mode);getMode.Output=mode;getMode.Failure=modeError;getMode.Returned=true
 if err:=modeError;err!=nil{
			return fmt.Errorf("error getting console mode: %w", err)
		}

		if err := p.sourceError("terminal.set-output-mode",p.ttyOutput,mode|windows.ENABLE_VIRTUAL_TERMINAL_PROCESSING,func()error{return windows.SetConsoleMode(windows.Handle(p.ttyOutput.Fd()),mode|windows.ENABLE_VIRTUAL_TERMINAL_PROCESSING)}); err != nil {
			return fmt.Errorf("error setting console mode: %w", err)
		}
	}

	return nil
}

// Open the Windows equivalent of a TTY.
func openInputTTY()(*os.File,error){return openInputTTYOwned(nil)}
func openInputTTYOwned(owner *Program) (*os.File, error) {
	opening:=owner.sourceStart("terminal.os-open-file",owner,struct{Path string;Flags int;Mode os.FileMode}{"CONIN$",os.O_RDWR,0o644})
 f, err := os.OpenFile("CONIN$", os.O_RDWR, 0o644);opening.Output=f;opening.Failure=err;opening.Returned=true //nolint:gosec
	if err != nil {
		return nil, fmt.Errorf("error opening file: %w", err)
	}
	return f, nil
}

const suspendSupported = false

func suspendProcess() {}
