package tea

import (
	"io"
	"os"
	"os/exec"
)

// execMsg is used internally to run an ExecCommand sent with Exec.
type execMsg struct {
	cmd ExecCommand
	fn  ExecCallback
}

// Exec is used to perform arbitrary I/O in a blocking fashion, effectively
// pausing the Program while execution is running and resuming it when
// execution has completed.
//
// Most of the time you'll want to use ExecProcess, which runs an exec.Cmd.
//
// For non-interactive i/o you should use a Cmd (that is, a tea.Cmd).
func Exec(c ExecCommand, fn ExecCallback) Cmd {
	return func() Msg {
		return execMsg{cmd: c, fn: fn}
	}
}

// ExecProcess runs the given *exec.Cmd in a blocking fashion, effectively
// pausing the Program while the command is running. After the *exec.Cmd exists
// the Program resumes. It's useful for spawning other interactive applications
// such as editors and shells from within a Program.
//
// To produce the command, pass an *exec.Cmd and a function which returns
// a message containing the error which may have occurred when running the
// ExecCommand.
//
//	type VimFinishedMsg struct { err error }
//
//	c := exec.Command("vim", "file.txt")
//
//	cmd := ExecProcess(c, func(err error) Msg {
//	    return VimFinishedMsg{err: err}
//	})
//
// Or, if you don't care about errors, you could simply:
//
//	cmd := ExecProcess(exec.Command("vim", "file.txt"), nil)
//
// For non-interactive i/o you should use a Cmd (that is, a tea.Cmd).
func ExecProcess(c *exec.Cmd, fn ExecCallback) Cmd {
	return Exec(wrapExecCommand(c), fn)
}

// ExecCallback is used when executing an *exec.Command to return a message
// with an error, which may or may not be nil.
type ExecCallback func(error) Msg

// ExecCommand can be implemented to execute things in a blocking fashion in
// the current terminal.
type ExecCommand interface {
	Run() error
	SetStdin(io.Reader)
	SetStdout(io.Writer)
	SetStderr(io.Writer)
}

// wrapExecCommand wraps an exec.Cmd so that it satisfies the ExecCommand
// interface so it can be used with Exec.
func wrapExecCommand(c *exec.Cmd) ExecCommand {
	return &osExecCommand{Cmd: c}
}

// osExecCommand is a layer over an exec.Cmd that satisfies the ExecCommand
// interface.
type osExecCommand struct{ *exec.Cmd }

// SetStdin sets stdin on underlying exec.Cmd to the given io.Reader.
func (c *osExecCommand) SetStdin(r io.Reader) {
	// If unset, have the command use the same input as the terminal.
	if c.Stdin == nil {
		c.Stdin = r
	}
}

// SetStdout sets stdout on underlying exec.Cmd to the given io.Writer.
func (c *osExecCommand) SetStdout(w io.Writer) {
	// If unset, have the command use the same output as the terminal.
	if c.Stdout == nil {
		c.Stdout = w
	}
}

// SetStderr sets stderr on the underlying exec.Cmd to the given io.Writer.
func (c *osExecCommand) SetStderr(w io.Writer) {
	// If unset, use stderr for the command's stderr
	if c.Stderr == nil {
		c.Stderr = w
	}
}

// execOriginal retains the real command, callback, streams and distinct failure
// occurrences. This is not native child-creation authority or disposal proof.
type execOriginal struct{Command ExecCommand;Callback ExecCallback;Input io.Reader;Output,Stderr io.Writer;ReleaseError,RunError,RestoreError error;Message Msg;CallbackReturned bool}
func(p *Program)exec(c ExecCommand,fn ExecCallback){
 original:=&execOriginal{Command:c,Callback:fn,Input:p.input,Output:p.output,Stderr:os.Stderr};occurrence:=p.sourceStart("exec.operation",c,original);defer retainSourceInterruption(occurrence)
 deliver:=func(err error){if fn==nil{return};callback:=p.sourceStart("exec.callback",fn,original);original.Message=fn(err);original.CallbackReturned=true;callback.Output=original.Message;callback.Returned=true;p.sourceWorker("exec.callback-delivery",original,original.Message,func(*SourceOccurrence){p.SendOwned(original.Message,original,nil,nil)})}
 original.ReleaseError=p.ReleaseTerminal();if original.ReleaseError!=nil{deliver(original.ReleaseError);occurrence.Returned=true;return}
 configuring:=p.sourceStart("exec.streams",c,original);c.SetStdin(original.Input);c.SetStdout(original.Output);c.SetStderr(original.Stderr);configuring.Returned=true
 running:=p.sourceStart("exec.run",c,original);original.RunError=c.Run();running.Failure=original.RunError;running.Returned=true
 p.renderer.resetLinesRendered()
 restoring:=p.sourceStart("exec.restore",p,original);original.RestoreError=p.RestoreTerminal();restoring.Failure=original.RestoreError;restoring.ReturnedError=original.RestoreError;restoring.Returned=true
 if original.RunError!=nil{deliver(original.RunError);occurrence.Returned=true;return};deliver(original.RestoreError);occurrence.Returned=true
}