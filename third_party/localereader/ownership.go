package localereader

import("io";"sync";"runtime/debug";"golang.org/x/text/transform")

// Ownership holds SAME actual locale reader/decoder/native/read originals. It
// conveys no finite fit, native control, collector ACK or disposal eligibility.
type Ownership struct{Source io.Reader;Reader io.Reader;Decoder transform.Transformer;Attachment any;mutex sync.Mutex;head *OriginalOperation}
type OriginalOperation struct{Source string;Subject,Input,Output any;Failure error;Panic any;Stack []byte;Returned bool;next *OriginalOperation}
func(o *Ownership)begin(source string,subject,input any)*OriginalOperation{operation:=&OriginalOperation{Source:source,Subject:subject,Input:input};if o!=nil{o.mutex.Lock();operation.next=o.head;o.head=operation;o.mutex.Unlock()};return operation}
func retainInterruption(operation *OriginalOperation){if original:=recover();original!=nil{operation.Panic=original;operation.Stack=debug.Stack();panic(original)};if !operation.Returned{operation.Stack=debug.Stack()}}
func NewReaderOwned(source io.Reader,attach func(*Ownership))io.Reader{ownership:=&Ownership{Source:source,Attachment:attach};if attach!=nil{attach(ownership)};reader:=newReaderOwned(source,ownership);ownership.Reader=reader;return reader}
// Actual producer/borrow joins are a caller precondition. This SAME-object
// inspection is never proof of eligibility or transport of a Go object graph.
func(o *Ownership)VisitOriginals(visit func(*OriginalOperation)){if o==nil||visit==nil{return};o.mutex.Lock();head:=o.head;o.mutex.Unlock();for operation:=head;operation!=nil;operation=operation.next{visit(operation)}}
type readOriginal struct{Buffer,Observed []byte;Count int;Failure error}
type transformOriginal struct{Destination,Source,SourceObserved,DestinationBefore,DestinationAfter []byte;AtEOF bool;DestinationCount,SourceCount int;Failure error}
// A truthful completed-byte observation is separate from the SAME live buffer.
func observeRead(original *readOriginal){if original.Count>=0&&original.Count<=len(original.Buffer){original.Observed=append([]byte(nil),original.Buffer[:original.Count]...)}}