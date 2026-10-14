//go:build windows
// +build windows

package tea

import (
	"fmt"
	"io"
	"os"
	"sync"

	"github.com/charmbracelet/x/term"
	"github.com/erikgeiser/coninput"
	"github.com/muesli/cancelreader"
	"golang.org/x/sys/windows"
)

type conInputReader struct {
	cancelMixin

	owner *Program
 closeOnce sync.Once
 closeError error
 conin windows.Handle

	originalMode uint32
 modePrepared bool
}

var _ cancelreader.CancelReader = &conInputReader{}

func newInputReader(r io.Reader,enableMouse bool)(cancelreader.CancelReader,error){return newInputReaderOwned(nil,r,enableMouse)}
func newInputReaderOwned(owner *Program,r io.Reader, enableMouse bool) (result cancelreader.CancelReader, resultError error) {
 constructor:=&consoleInputConstructor{Reader:r,Mouse:enableMouse};occurrence:=owner.sourceStart("console-input.construct",r,constructor);defer retainSourceInterruption(occurrence)
 result,resultError=newInputReaderSource(owner,r,enableMouse,constructor);occurrence.Output=result;occurrence.Failure=resultError;occurrence.Returned=true;return result,resultError
}
func newInputReaderSource(owner *Program,r io.Reader,enableMouse bool,constructor *consoleInputConstructor)(cancelreader.CancelReader,error){
	fallback := func(io.Reader) (cancelreader.CancelReader, error) {
		var attachment *SourceOccurrence
 result,err:=cancelreader.NewReaderOwned(r,func(original *cancelreader.Ownership){attachment=owner.sourceStart("cancelreader.construct-owner",r,original)})
 attachment.Output=result;attachment.Failure=err;attachment.Returned=true;return result,err
	}
	if f, ok := r.(term.File); !ok || f.Fd() != os.Stdin.Fd() {
		return fallback(r)
	}

	handleOperation:=owner.sourceStart("console-input.standard-handle",r,constructor)
 conin, err := coninput.NewStdinHandle();constructor.Handle=conin;constructor.HandleError=err;handleOperation.Output=conin;handleOperation.Failure=err;handleOperation.Returned=true
	if err != nil {
		return fallback(r)
	}

	modes := []uint32{
		windows.ENABLE_WINDOW_INPUT,
		windows.ENABLE_EXTENDED_FLAGS,
	}

	// Since we have options to enable mouse events, [WithMouseCellMotion],
	// [WithMouseAllMotion], and [EnableMouseCellMotion],
	// [EnableMouseAllMotion], and [DisableMouse], we need to check if the user
	// has enabled mouse events and add the appropriate mode accordingly.
	// Otherwise, mouse events will be enabled all the time.
	if enableMouse {
		modes = append(modes, windows.ENABLE_MOUSE_INPUT)
	}

	originalMode, err := prepareConsoleOwned(owner,conin, modes...)
	if err != nil {
		return nil, fmt.Errorf("failed to prepare console input: %w", err)
	}

	return &conInputReader{
		owner:owner,
 conin:        conin,
		originalMode: originalMode,
 modePrepared:true,
	}, nil
}

// Cancel implements cancelreader.CancelReader.
func (r *conInputReader) Cancel() bool {
	r.setCanceled()

	// Warning: These cancel methods do not reliably work on console input
	// 			and should not be counted on.
	first:=r.owner.sourceStart("console-input.cancel-io-ex",r,r.conin);first.Failure=windows.CancelIoEx(r.conin,nil);first.Returned=true;if first.Failure==nil{first.Output=true;return true}
 second:=r.owner.sourceStart("console-input.cancel-io",r,r.conin);second.Failure=windows.CancelIo(r.conin);second.Returned=true;second.Output=second.Failure==nil;return second.Failure==nil
}

// Close implements cancelreader.CancelReader.
func(r *conInputReader)Close()error{
 r.closeOnce.Do(func(){if r.modePrepared{r.closeError=r.owner.sourceError("console-input.restore-mode",r,r.originalMode,func()error{return windows.SetConsoleMode(r.conin,r.originalMode)})}});return r.closeError
}
// Read implements cancelreader.CancelReader.
func (r *conInputReader) Read(_ []byte) (n int, err error) {
	if r.isCanceled() {
		err = cancelreader.ErrCanceled
	}
	return
}

type consoleInputConstructor struct{Reader io.Reader;Mouse bool;Handle windows.Handle;HandleError error}
func prepareConsole(input windows.Handle,modes ...uint32)(uint32,error){return prepareConsoleOwned(nil,input,modes...)}
func prepareConsoleOwned(owner *Program,input windows.Handle, modes ...uint32) (originalMode uint32, err error) {
	getMode:=owner.sourceStart("console-input.get-mode",input,&originalMode)
 err = windows.GetConsoleMode(input, &originalMode);getMode.Output=originalMode;getMode.Failure=err;getMode.Returned=true
	if err != nil {
		return 0, fmt.Errorf("get console mode: %w", err)
	}

	newMode := coninput.AddInputModes(0, modes...)

	err=owner.sourceError("console-input.set-mode",input,newMode,func()error{return windows.SetConsoleMode(input,newMode)})
	if err != nil {
		return 0, fmt.Errorf("set console mode: %w", err)
	}

	return originalMode, nil
}

// cancelMixin represents a goroutine-safe cancellation status.
type cancelMixin struct {
	unsafeCanceled bool
	lock           sync.Mutex
}

func (c *cancelMixin) setCanceled() {
	c.lock.Lock()
	defer c.lock.Unlock()

	c.unsafeCanceled = true
}

func (c *cancelMixin) isCanceled() bool {
	c.lock.Lock()
	defer c.lock.Unlock()

	return c.unsafeCanceled
}
