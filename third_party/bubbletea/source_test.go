package tea

import (
 "context"
 "errors"
 "testing"
 "runtime"
 "io"
 "strings"
 "github.com/muesli/cancelreader"
)

// UNRUN actual source cases; no terminal/native acceptance or eligibility proof.
func TestOwnedSourceWorkerRefusalDoesNotManufactureWorkerCompletion(t *testing.T){
 program:=NewProgram(nil);program.stopSourceWorkers();var original *SourceOccurrence
 entered:=false
 admitted:=program.sourceWorkerBound("test.refused",program,nil,func(*SourceOccurrence){entered=true},func(value *SourceOccurrence){original=value})
 if admitted||entered||original==nil||original.WorkerLaunched||original.WorkerEntered||original.WorkerReturned||original.Failure!=ErrProgramKilled{t.Fatal("refusal confused with real worker lifecycle")}
 program.joinSourceWorkers()
}
func TestOwnedSourceWorkerPanicRetainsActualOriginalAndJoins(t *testing.T){
 program:=NewProgram(nil);cause:=errors.New("actual private library panic");entered:=make(chan struct{});var original *SourceOccurrence
 if !program.sourceWorkerBound("test.panic",program,nil,func(*SourceOccurrence){close(entered);panic(cause)},func(value *SourceOccurrence){original=value}){t.Fatal("worker not admitted")}
 <-entered;program.stopSourceWorkers();program.joinSourceWorkers()
 if original.Panic!=cause||!original.WorkerLaunched||!original.WorkerEntered||original.WorkerReturned||original.Returned||len(original.Stack)==0||program.ctx.Err()!=context.Canceled{t.Fatal("actual interrupted worker graph/notification lost")}
}
type sourceRefusingWriter struct{cause error;input []byte}
func(w *sourceRefusingWriter)Write(input []byte)(int,error){w.input=input;return 0,w.cause}
func TestOwnedOutputRetainsSameWriterBufferErrorAndTruthfulSnapshot(t *testing.T){
 program:=NewProgram(nil);cause:=errors.New("actual writer failure");target:=&sourceRefusingWriter{cause:cause};input:=[]byte("frame")
 writer:=sourceWriter{program:program,writer:target};if count,err:=writer.Write(input);count!=0||err!=cause{t.Fatal("actual writer result changed")}
 occurrence:=program.sourceOwnership.head;operation:=occurrence.Input.(*sourceWriteOriginal)
 if operation.Writer!=target||operation.Failure!=cause||&operation.Input[0]!=&input[0]||string(operation.ObservedBytes)!="frame"{t.Fatal("original alias or observation replaced")}
 input[0]='F';if operation.Input[0]!='F'||string(operation.ObservedBytes)!="frame"{t.Fatal("live alias conflated with truthful preeffect snapshot")}
 if !errors.Is(program.sourceFailures(),cause){t.Fatal("actual output failure absent from return relation")}
}
func TestOwnedShutdownContinuesAfterIndependentCleanupFailure(t *testing.T){
 program:=NewProgram(nil);cause:=errors.New("first cleanup failure");second:=false
 if err:=program.sourceError("test.first",program,nil,func()error{return cause});err!=cause{t.Fatal("original cleanup failure replaced")}
 program.sourceError("test.second",program,nil,func()error{second=true;return nil})
 if !second||!errors.Is(program.sourceFailures(),cause){t.Fatal("independent cleanup dropped")}
}
func TestOwnedQuitUsesOriginalControlSwitchAndExactConsumer(t *testing.T){
 program:=NewProgram(nil);producer:=&struct{ID string}{"actual producer"};original:=&sourceMessageOriginal{Owner:producer,Message:QuitMsg{}}
 occurrence:=program.sourceStart("test.delivery",program,original);var received *SourceOccurrence
 _,err,stop:=program.consumeMessage(nil,nil,sourceMessage{occurrence:occurrence,message:original.Message,completed:func(value *SourceOccurrence){received=value}})
 if err!=nil||!stop||received!=occurrence||!occurrence.Returned||original.Owner!=producer{t.Fatal("original Quit switch/producer/consumer relation lost")}
}
func TestOwnedCanceledDeliveryRetainsActualNoConsumerObservation(t *testing.T){
 program:=NewProgram(nil);program.cancel();producer:=&struct{}{};completed:=false
 if err:=program.SendOwned(QuitMsg{},producer,nil,func(*SourceOccurrence){completed=true});err!=nil{t.Fatal(err)}
 occurrence:=program.sourceOwnership.head;original:=occurrence.Input.(*sourceMessageOriginal)
 if original.Owner!=producer||!original.SendAttempted||!original.SendReturned||original.Accepted||original.ConsumerStarted||original.ConsumerReturned||original.ContextError!=context.Canceled||completed||occurrence.Returned{t.Fatal("canceled delivery fabricated consumption or lost producer")}
}

func TestInputConstructorAttachmentRecordsActualReturn(t *testing.T){
 program:=NewProgram(nil);input:=strings.NewReader("input");reader,err:=newInputReaderOwned(program,input,false);if err!=nil{t.Fatal(err)}
 occurrence:=program.sourceOwnership.head
 if occurrence.Source!="cancelreader.construct-owner"||!occurrence.Returned||occurrence.Output!=reader||occurrence.Failure!=err{t.Fatal("constructor attachment outcome detached")}
 ownership:=occurrence.Input.(*cancelreader.Ownership);if ownership.Reader!=input||ownership.Result!=reader||ownership.Partial!=reader{t.Fatal("actual constructor graph replaced")}
 if err:=reader.Close();err!=nil{t.Fatal(err)}
}
// Routine source case only: this reader is not native qualification evidence.
type retiringSourceReader struct{cause error;cancelled chan struct{};loopReturned chan struct{};closes int;closedBeforeJoin bool}
func(r *retiringSourceReader)Read([]byte)(int,error){return 0,r.cause}
func(r *retiringSourceReader)Cancel()bool{close(r.cancelled);return false}
func(r *retiringSourceReader)Close()error{r.closes++;select{case <-r.loopReturned:default:r.closedBeforeJoin=true};return r.cause}
func TestReaderRetirementJoinsActualLoopAndRetainsOnceCloseFailure(t *testing.T){
 program:=NewProgram(nil);cause:=errors.New("actual reader close failure");reader:=&retiringSourceReader{cause:cause,cancelled:make(chan struct{}),loopReturned:make(chan struct{})}
 program.cancelReader=reader;program.readLoopDone=reader.loopReturned;program.readLoopOriginal=&SourceOccurrence{WorkerLaunched:true}
 program.cancelReaderEpisode=&inputReaderEpisode{Reader:reader,ReadLoop:program.readLoopOriginal,ReadLoopDone:reader.loopReturned}
 workerDone:=make(chan struct{});go func(){<-reader.cancelled;close(reader.loopReturned);close(workerDone)}()
 if err:=program.retireCancelReader();err!=cause{t.Fatal("actual close failure replaced")};<-workerDone
 if reader.closedBeforeJoin||reader.closes!=1||program.retireCancelReader()!=cause{t.Fatal("close before join or native retry")}
 if err:=program.ReleaseTerminal();!errors.Is(err,cause)||reader.closes!=1{t.Fatal("release lost failure or repeated physical close")}
 if !errors.Is(program.sourceFailures(),cause){t.Fatal("Run failure relation lost actual close original")}
}
func TestInputFailureCannotBlockRetirementBehindFullErrorChannel(t *testing.T){
 program:=NewProgram(nil);cause:=errors.New("actual input failure behind queued error");queued:=errors.New("independent queued failure")
 program.errs=make(chan error,1);program.errs<-queued;program.msgs=make(chan Msg);program.readLoopDone=make(chan struct{})
 program.cancelReader=&retiringSourceReader{cause:cause};program.readLoop()
 select{case <-program.readLoopDone:default:t.Fatal("actual read loop did not finish")}
 if program.ctx.Err()!=context.Canceled||!errors.Is(program.sourceFailures(),cause){t.Fatal("actual undelivered input failure lost")}
 if <-program.errs!=queued{t.Fatal("independent error replaced")}
}
func TestOwnedAnsiInputLinksActualParserToOriginalFilterConsumer(t *testing.T){
 program:=NewProgram(nil);messages:=make(chan Msg,1);input:=strings.NewReader("a")
 if err:=readAnsiInputsOwned(program,program.ctx,messages,input);!errors.Is(err,io.EOF){t.Fatal(err)}
 carrier:=<-messages;owned,ok:=carrier.(sourceMessage);if !ok{t.Fatal("actual input source detached")};original:=owned.occurrence.Input.(*sourceMessageOriginal);producer,ok:=original.Owner.(*SourceOccurrence)
 if !ok||producer.Source!="input.ansi-parse"||!original.Accepted||!original.SendReturned||original.ConsumerStarted{t.Fatal("parse/send source relation replaced")}
 var observed Msg;program.filter=func(_ Model,message Msg)Msg{observed=message;return nil};program.consumeMessage(nil,nil,carrier)
 key,ok:=observed.(KeyMsg);if !ok||string(key.Runes)!="a"||!original.ConsumerReturned{t.Fatal("original input type/filter consumer lost")}
}
func TestSourceInvocationGoexitDoesNotClaimNormalReturn(t *testing.T){
 occurrence:=&SourceOccurrence{Started:true};done:=make(chan struct{})
 go func(){defer close(done);defer retainSourceInterruption(occurrence);runtime.Goexit();occurrence.Returned=true}();<-done
 if occurrence.Returned||len(occurrence.Stack)==0{t.Fatal("interrupted source marked normally returned")}
}
func TestSourceInvocationPanicRetainsAndRethrowsSameOriginal(t *testing.T){
 cause:=errors.New("actual invocation panic");occurrence:=&SourceOccurrence{Started:true};var recovered any
 func(){defer func(){recovered=recover()}();defer retainSourceInterruption(occurrence);panic(cause)}()
 if recovered!=cause||occurrence.Panic!=cause||occurrence.Returned||len(occurrence.Stack)==0{t.Fatal("original panic replaced or invocation marked returned")}
}
type steppedSourceInput struct{values [][]byte;buffers [][]byte;next int}
func(r *steppedSourceInput)Read(buffer []byte)(int,error){r.buffers=append(r.buffers,buffer);if r.next==len(r.values){return 0,io.EOF};value:=r.values[r.next];r.next++;return copy(buffer,value),nil}
func TestUnknownCSIActualMessageBorrowsNonreusedReadFrameAcrossNextRead(t *testing.T){
 program:=NewProgram(nil);input:=&steppedSourceInput{values:[][]byte{[]byte("\x1b[999~"),[]byte("z")}};messages:=make(chan Msg,2)
 if err:=readAnsiInputsOwned(program,program.ctx,messages,input);!errors.Is(err,io.EOF){t.Fatal(err)}
 delivered:=(<-messages).(sourceMessage);message,ok:=delivered.message.(unknownCSISequenceMsg);if !ok||string(message)!="\x1b[999~"{t.Fatal("actual prior unknown CSI bytes overwritten")}
 original:=delivered.occurrence.Input.(*sourceMessageOriginal);parser:=original.Owner.(*SourceOccurrence);reading:=parser.Subject.(*SourceOccurrence);frame:=reading.Input.(*ansiReadOriginal)
 if &message[0]!=&frame.Buffer[0]||&input.buffers[0][0]==&input.buffers[1][0]{t.Fatal("original parser message substituted or read frame reused")}
 if string(frame.Observed)!="\x1b[999~"||string(message)!="\x1b[999~"{t.Fatal("truthful snapshot or original message changed")}
}
type invalidExtentSourceInput struct{cause error}
func(r invalidExtentSourceInput)Read([]byte)(int,error){return 300,r.cause}
func TestAnsiInvalidReadExtentRetainsRawCountAndCauseBeforeSnapshot(t *testing.T){
 program:=NewProgram(nil);cause:=errors.New("actual invalid-count input failure");err:=readAnsiInputsOwned(program,program.ctx,make(chan Msg),invalidExtentSourceInput{cause})
 invalid,ok:=err.(*invalidAnsiReadCount);if !ok||invalid.Original.Count!=300||invalid.Original.Failure!=cause||!errors.Is(err,cause)||invalid.Original.Observed!=nil{t.Fatal("raw count/cause lost before extent validation")}
}