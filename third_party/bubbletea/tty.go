package tea

import (
	"errors"
	"fmt"
	"io"
 "sync"
	"time"

	"github.com/charmbracelet/x/term"
	"github.com/muesli/cancelreader"
)

func (p *Program) suspend() {
	if err := p.ReleaseTerminal(); err != nil {
		// If we can't release input, abort.
		return
	}

	suspendProcess()

	p.sourceError("terminal.resume-restore",p,nil,p.RestoreTerminal)
	p.sourceWorker("resume-delivery",p,nil,func(source *SourceOccurrence){p.SendOwned(ResumeMsg{},source,nil,nil)})
}

func (p *Program) initTerminal() error {
	if _, ok := p.renderer.(*nilRenderer); ok {
		// No need to initialize the terminal if we're not rendering
		return nil
	}

	if err := p.initInput(); err != nil {
		return err
	}

	p.renderer.hideCursor()
	return nil
}

// restoreTerminalState restores the terminal to the state prior to running the
// Bubble Tea program.
func (p *Program) restoreTerminalState() error {
	if p.renderer != nil {
		p.renderer.disableBracketedPaste()
		p.renderer.showCursor()
		p.disableMouse()

		if p.renderer.reportFocus() {
			p.renderer.disableReportFocus()
		}

		if p.renderer.altScreen() {
			p.renderer.exitAltScreen()

			// give the terminal a moment to catch up
			time.Sleep(time.Millisecond * 10) //nolint:mnd
		}
	}

	return p.restoreInput()
}

// restoreInput restores the tty input to its original state.
func(p *Program)restoreInput()error{
 var inputFailure,outputFailure,publicFailure error
 if p.ttyInput!=nil&&p.previousTtyInputState!=nil{inputFailure=p.sourceError("terminal-input.restore",p.ttyInput,p.previousTtyInputState,func()error{return term.Restore(p.ttyInput.Fd(),p.previousTtyInputState)})}
 if p.ttyOutput!=nil&&p.previousOutputState!=nil{outputFailure=p.sourceError("terminal-output.restore",p.ttyOutput,p.previousOutputState,func()error{return term.Restore(p.ttyOutput.Fd(),p.previousOutputState)})}
 // Independent private restorations precede public transport, whose panic
 // must not skip either already eligible physical restore.
 if p.sourceTerminal!=nil&&p.publicRawAttempted{
  restoration:=p.publicRestoreOriginal
  if restoration==nil{restoration=p.sourceStart("public-terminal.restore",p.sourceTerminal,p.ttyInput);p.publicRestoreOriginal=restoration;invoke:=p.sourceTerminal.RestorePublicInput;restoration.Invocation=invoke;restoration.Failure=invoke();restoration.ReturnedError=restoration.Failure;restoration.Returned=true}
  if restoration.Returned{publicFailure=restoration.Failure;if publicFailure==nil{p.publicRawAttempted=false}}else{publicFailure=ErrProgramKilled}
 }
 return errors.Join(inputFailure,outputFailure,publicFailure)
}
// initCancelReader (re)commences reading inputs.
func (p *Program) initCancelReader(cancel bool) error {
	if p.cancelReader!=nil{if err:=p.retireCancelReader();err!=nil{return err}}

	var err error
	occurrence:=p.sourceStart("input-reader.construct",p.input,p.mouseMode)
 p.cancelReader, err = newInputReaderOwned(p,p.input,p.mouseMode);occurrence.Output=p.cancelReader;occurrence.Failure=err;occurrence.ReturnedError=err;occurrence.Returned=true
	if err != nil {
		return fmt.Errorf("error creating cancelreader: %w", err)
	}

	episode:=&inputReaderEpisode{Reader:p.cancelReader};p.cancelReaderEpisode=episode;p.sourceStart("input-reader.episode",p.cancelReader,episode)
 p.readLoopDone = make(chan struct{});episode.ReadLoopDone=p.readLoopDone
	if !p.sourceWorkerBound("input-read-loop",p,p.cancelReader,func(*SourceOccurrence){p.readLoop()},func(original *SourceOccurrence){p.readLoopOriginal=original;episode.ReadLoop=original}){return p.readLoopOriginal.Failure}

	return nil
}

func (p *Program) readLoop() {
	defer close(p.readLoopDone)

	occurrence:=p.sourceStart("input.read",p.cancelReader,p.msgs)
 err := readInputsOwned(p,p.ctx, p.msgs, p.cancelReader);occurrence.Failure=err;occurrence.Returned=true
	if !errors.Is(err,io.EOF)&&!errors.Is(err,cancelreader.ErrCanceled){
  occurrence.ReturnedError=err
  notification:=p.sourceStart("input.failure-notification",p,occurrence)
  select{case <-p.ctx.Done():notification.Output="context-already-done"
  case p.errs<-err:notification.Output="error-delivered"
  default:notification.Output="joining-loop-cancel-request";p.cancel()}
  notification.Returned=true
 }
}
// waitForReadLoop waits for the cancelReader to finish its read loop.
func (p *Program) waitForReadLoop(){
 if p.readLoopDone!=nil&&p.readLoopOriginal!=nil&&p.readLoopOriginal.WorkerLaunched{<-p.readLoopDone}
}
// checkResize detects the current size of the output and informs the program
// via a WindowSizeMsg.
func (p *Program) checkResize() {
	if p.ttyOutput == nil {
		// can't query window size
		return
	}

	size:=p.sourceStart("terminal.get-size",p.ttyOutput,p.ttyOutput.Fd())
 var w,h int;var err error
 if p.sourceTerminal!=nil{size.Input=p.sourceTerminal;invoke:=p.sourceTerminal.PublicSize;size.Invocation=invoke;w,h,err=invoke()}else{w,h,err=term.GetSize(p.ttyOutput.Fd())};size.Output=struct{Width,Height int}{w,h};size.Failure=err;size.Returned=true
	if err != nil {
		select {
		case <-p.ctx.Done():
		case p.errs <- err:
		}

		return
	}

	p.SendOwned(WindowSizeMsg{Width:w,Height:h},size,nil,nil)
}

// Each acquired reader has one actual retirement occurrence, shared by restart,
// release and shutdown. Failed native close is retained, never retried.
type inputReaderEpisode struct{Reader cancelreader.CancelReader;RetireOnce sync.Once;Failure error;CancelResult bool;ReadLoop *SourceOccurrence;ReadLoopDone chan struct{};ReadLoopWaitReturned bool}
func(p *Program)retireCancelReader()error{
 episode:=p.cancelReaderEpisode;if episode==nil{return nil}
 episode.RetireOnce.Do(func(){
  request:=p.sourceStart("input-reader.retire-cancel",episode.Reader,episode);episode.CancelResult=episode.Reader.Cancel();request.Output=episode.CancelResult;request.Returned=true
  if episode.ReadLoop!=nil&&episode.ReadLoop.WorkerLaunched&&episode.ReadLoopDone!=nil{<-episode.ReadLoopDone;episode.ReadLoopWaitReturned=true}
  episode.Failure=p.sourceError("input-reader.retire-close",episode.Reader,episode,episode.Reader.Close)
 });return episode.Failure
}