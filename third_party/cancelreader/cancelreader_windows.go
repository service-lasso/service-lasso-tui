//go:build windows
// +build windows

package cancelreader

import (
	"fmt"
 "errors"
 "sync"
	"io"
	"os"
	"syscall"
	"time"
	"unicode/utf16"

	"golang.org/x/sys/windows"
)

var fileShareValidFlags uint32 = 0x00000007

// NewReader returns a reader and a cancel function. If the input reader is a
// File with the same file descriptor as os.Stdin, the cancel function can
// be used to interrupt a blocking read call. In this case, the cancel function
// returns true if the call was canceled successfully. If the input reader is
// not a File with the same file descriptor as os.Stdin, the cancel
// function does nothing and always returns false. The Windows implementation
// is based on WaitForMultipleObject with overlapping reads from CONIN$.
func NewReader(reader io.Reader)(CancelReader,error){return NewReaderOwned(reader,nil)}
func newReaderWithOwnership(reader io.Reader,ownership *Ownership)(CancelReader,error){
	if f, ok := reader.(File); !ok || f.Fd() != os.Stdin.Fd() {
		return newFallbackCancelReaderOwned(reader,ownership)
	}

	r:=&winCancelReader{ownership:ownership,blockingReadSignal:make(chan struct{},1)};ownership.Partial=r
 opening:=ownership.begin("console.open",r,"CONIN$")
 // it is necessary to open CONIN$ (NOT windows.STD_INPUT_HANDLE) in
	// overlapped mode to be able to use it with WaitForMultipleObjects.
	conin, err := windows.CreateFile(
		&(utf16.Encode([]rune("CONIN$\x00"))[0]), windows.GENERIC_READ|windows.GENERIC_WRITE,
		fileShareValidFlags, nil, windows.OPEN_EXISTING, windows.FILE_FLAG_OVERLAPPED, 0)
	opening.Output=conin;opening.Failure=err;opening.Returned=true
	if err != nil {
		return nil, fmt.Errorf("open CONIN$ in overlapping mode: %w", err)
	}

	r.conin=conin;r.coninAcquired=true
 preparation:=ownership.begin("console.prepare",r,conin)
 resetConsole, err := prepareConsoleOwned(ownership,conin);r.resetConsole=resetConsole;preparation.Output=resetConsole;preparation.Failure=err;preparation.Returned=true
	if err != nil {
		return nil, errors.Join(fmt.Errorf("prepare console: %w",err),r.Close())
	}

	// flush input, otherwise it can contain events which trigger
	// WaitForMultipleObjects but which ReadFile cannot read, resulting in an
	// un-cancelable read
	flushing:=ownership.begin("console.flush",r,conin)
 err = flushConsoleInputBuffer(conin);flushing.Failure=err;flushing.Returned=true
	if err != nil {
		return nil, errors.Join(fmt.Errorf("flush console input buffer: %w",err),r.Close())
	}

	creating:=ownership.begin("cancel-event.create",r,nil)
 cancelEvent, err := windows.CreateEvent(nil,0,0,nil);creating.Output=cancelEvent;creating.Failure=err;creating.Returned=true
	if err != nil {
		return nil, errors.Join(fmt.Errorf("create stop event: %w",err),r.Close())
	}

	r.cancelEvent=cancelEvent;r.eventAcquired=true;return r,nil
}

type winCancelReader struct {
 ownership *Ownership
 closeOnce,cancelOnce sync.Once
 state sync.Mutex
 readers sync.WaitGroup
 closing,closeAttempted bool
 cancelResult bool
 cancelLock sync.Mutex
 closeError error
 coninAcquired,eventAcquired bool
	conin       windows.Handle
	cancelEvent windows.Handle
	cancelMixin

	resetConsole       func() error
	blockingReadSignal chan struct{}
}

func (r *winCancelReader) Read(data []byte) (int, error) {
 admission:=r.ownership.begin("console.read-admission",r,data);r.state.Lock();if r.closing{r.state.Unlock();admission.Failure=os.ErrClosed;admission.Returned=true;return 0,os.ErrClosed};r.readers.Add(1);r.state.Unlock();defer r.readers.Done();admission.Returned=true
	if r.isCanceled() {
		return 0, ErrCanceled
	}

	err := r.wait()
	if err != nil {
		return 0, err
	}

	if r.isCanceled() {
		return 0, ErrCanceled
	}

	// windows.Read does not work on overlapping windows.Handles
	return r.readAsync(data)
}

// Cancel cancels ongoing and future Read() calls and returns true if the
// cancelation of the ongoing Read() was successful. On Windows Terminal,
// WaitForMultipleObjects sometimes immediately returns without input being
// available. In this case, graceful cancelation is not possible and Cancel()
// returns false.
type windowsCancelOriginal struct{Timer *time.Timer;TimerValue time.Time;EventError error;SignalAcquired,SignalReturned bool;Result bool}
func(r *winCancelReader)Cancel()bool{
 r.cancelLock.Lock();defer r.cancelLock.Unlock();if r.closeAttempted{return false};r.setCanceled()
 r.cancelOnce.Do(func(){
  original:=&windowsCancelOriginal{};operation:=r.ownership.begin("console.cancel-request",r,original)
  if !r.eventAcquired{operation.Returned=true;return}
  // Preserve this existing API's cancellation observation window. It is not
  // the independent caller/native acceptance deadline or a granted TUI budget.
  original.Timer=time.NewTimer(100*time.Millisecond);defer original.Timer.Stop()
  select{case r.blockingReadSignal<-struct{}{}:
   original.SignalAcquired=true;setting:=r.ownership.begin("cancel-event.set",r,original)
   original.EventError=windows.SetEvent(r.cancelEvent);setting.Failure=original.EventError;setting.Returned=true
   <-r.blockingReadSignal;original.SignalReturned=true;if original.EventError==nil{original.Result=true}
  case original.TimerValue=<-original.Timer.C:}
  r.cancelResult=original.Result;operation.Output=original.Result;operation.Failure=original.EventError;operation.Returned=true
 });return r.cancelResult
}
func(r *winCancelReader)Close()error{
 r.closeOnce.Do(func(){r.state.Lock();r.closing=true;r.state.Unlock();if r.eventAcquired{r.Cancel()};r.readers.Wait();r.cancelLock.Lock();defer r.cancelLock.Unlock();r.closeAttempted=true
  if r.eventAcquired{operation:=r.ownership.begin("cancel-event.close",r,r.cancelEvent);operation.Failure=windows.CloseHandle(r.cancelEvent);operation.Returned=true;r.closeError=errors.Join(r.closeError,operation.Failure)}
  if r.resetConsole!=nil{operation:=r.ownership.begin("console.restore",r,r.resetConsole);operation.Failure=r.resetConsole();operation.Returned=true;r.closeError=errors.Join(r.closeError,operation.Failure)}
  if r.coninAcquired{operation:=r.ownership.begin("console.close",r,r.conin);operation.Failure=windows.Close(r.conin);operation.Returned=true;r.closeError=errors.Join(r.closeError,operation.Failure)}
 });return r.closeError
}
func (r *winCancelReader) wait() error {
	handles:=[]windows.Handle{r.conin,r.cancelEvent};operation:=r.ownership.begin("console.wait",r,handles)
 event, err := windows.WaitForMultipleObjects(handles,false,windows.INFINITE);operation.Output=event;operation.Failure=err;operation.Returned=true
	switch {
	case windows.WAIT_OBJECT_0 <= event && event < windows.WAIT_OBJECT_0+2:
		if event == windows.WAIT_OBJECT_0+1 {
			return ErrCanceled
		}

		if event == windows.WAIT_OBJECT_0 {
			return nil
		}

		return fmt.Errorf("unexpected wait object is ready: %d", event-windows.WAIT_OBJECT_0)
	case windows.WAIT_ABANDONED <= event && event < windows.WAIT_ABANDONED+2:
		return fmt.Errorf("abandoned")
	case event == uint32(windows.WAIT_TIMEOUT):
		return fmt.Errorf("timeout")
	case event == windows.WAIT_FAILED:
		return fmt.Errorf("failed console wait: %w",err)
	default:
		return fmt.Errorf("unexpected error: %w", error(err))
	}
}

// readAsync is necessary to read from a windows.Handle in overlapping mode.
// windowsReadOriginal holds the actual native buffer/frame after uncertain IO;
// caller reuse of its input slice cannot overwrite a still-borrowed native frame.
type windowsReadOriginal struct{Caller,Native []byte;Event windows.Handle;Overlapped windows.Overlapped;Count uint32;CreateError,ReadError,CompletionError,CloseError error}
func(r *winCancelReader)readAsync(data []byte)(int,error){
 original:=&windowsReadOriginal{Caller:data,Native:make([]byte,len(data))};creating:=r.ownership.begin("read-event.create",r,original)
 original.Event,original.CreateError=windows.CreateEvent(nil,0,0,nil);creating.Output=original.Event;creating.Failure=original.CreateError;creating.Returned=true
 if original.CreateError!=nil{return 0,fmt.Errorf("create read event: %w",original.CreateError)}
 original.Overlapped.HEvent=original.Event;reading:=r.ownership.begin("console.read-file",r,original)
 original.ReadError=windows.ReadFile(r.conin,original.Native,&original.Count,&original.Overlapped);reading.Output=original.Count;reading.Failure=original.ReadError;reading.Returned=true
 if original.ReadError!=nil&&original.ReadError!=windows.ERROR_IO_PENDING{return 0,original.ReadError}
 r.blockingReadSignal<-struct{}{};defer func(){<-r.blockingReadSignal}()
 completion:=r.ownership.begin("console.overlapped-result",r,original)
 original.CompletionError=windows.GetOverlappedResult(r.conin,&original.Overlapped,&original.Count,true);completion.Output=original.Count;completion.Failure=original.CompletionError;completion.Returned=true
 if original.CompletionError!=nil{return 0,original.CompletionError}
 copy(data,original.Native[:int(original.Count)])
 closing:=r.ownership.begin("read-event.close",r,original);original.CloseError=windows.CloseHandle(original.Event);closing.Failure=original.CloseError;closing.Returned=true
 return int(original.Count),original.CloseError
}
func prepareConsole(input windows.Handle)(func()error,error){return prepareConsoleOwned(nil,input)}
func prepareConsoleOwned(ownership *Ownership,input windows.Handle) (reset func() error, err error) {
	var originalMode uint32

	observation:=ownership.begin("console.get-mode",input,&originalMode)
 err = windows.GetConsoleMode(input,&originalMode);observation.Output=originalMode;observation.Failure=err;observation.Returned=true
	if err != nil {
		return nil, fmt.Errorf("get console mode: %w", err)
	}

	var newMode uint32
	newMode &^= windows.ENABLE_ECHO_INPUT
	newMode &^= windows.ENABLE_LINE_INPUT
	newMode &^= windows.ENABLE_MOUSE_INPUT
	newMode &^= windows.ENABLE_WINDOW_INPUT
	newMode &^= windows.ENABLE_PROCESSED_INPUT

	newMode |= windows.ENABLE_EXTENDED_FLAGS
	newMode |= windows.ENABLE_INSERT_MODE
	newMode |= windows.ENABLE_QUICK_EDIT_MODE

	// Enabling virtual terminal input is necessary for processing certain
	// types of input like X10 mouse events and arrows keys with the current
	// bytes-based input reader. It does, however, prevent cancelReader from
	// being able to cancel input. The planned solution for this is to read
	// Windows events in a more native fashion, rather than the current simple
	// bytes-based input reader which works well on unix systems.
	newMode |= windows.ENABLE_VIRTUAL_TERMINAL_INPUT

	setting:=ownership.begin("console.set-mode",input,newMode)
 err = windows.SetConsoleMode(input,newMode);setting.Failure=err;setting.Returned=true
	if err != nil {
		return nil, fmt.Errorf("set console mode: %w", err)
	}

	return func() error {
		restoring:=ownership.begin("console.reset-mode",input,originalMode)
 err := windows.SetConsoleMode(input,originalMode);restoring.Failure=err;restoring.Returned=true
		if err != nil {
			return fmt.Errorf("reset console mode: %w", err)
		}

		return nil
	}, nil
}

var (
	modkernel32                 = windows.NewLazySystemDLL("kernel32.dll")
	procFlushConsoleInputBuffer = modkernel32.NewProc("FlushConsoleInputBuffer")
)

func flushConsoleInputBuffer(consoleInput windows.Handle) error {
	r, _, e := syscall.Syscall(procFlushConsoleInputBuffer.Addr(), 1,
		uintptr(consoleInput), 0, 0)
	if r == 0 {
		return error(e)
	}

	return nil
}
