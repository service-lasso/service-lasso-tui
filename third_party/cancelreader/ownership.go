package cancelreader

import("io";"sync")

// Ownership is the actual partial constructor/resource graph. Attach runs before
// native effects; returned API projections do not replace this SAME local graph.
type Ownership struct{Reader io.Reader;Partial,Result CancelReader;Failure error;mutex sync.Mutex;head *OriginalOperation}
type OriginalOperation struct{Source string;Subject,Input,Output any;Failure error;Attempted,Returned bool;next *OriginalOperation}
func(o *Ownership)begin(source string,subject,input any)*OriginalOperation{
 operation:=&OriginalOperation{Source:source,Subject:subject,Input:input,Attempted:true};if o==nil{return operation};o.mutex.Lock();operation.next=o.head;o.head=operation;o.mutex.Unlock();return operation
}
// NewReaderOwned is a source-bound constructor entry, never JSON callback input
// or native authority. Existing NewReader behavior and public API remain useful.
func NewReaderOwned(reader io.Reader,attach func(*Ownership))(CancelReader,error){
 ownership:=&Ownership{Reader:reader};if attach!=nil{attach(ownership)}
 result,err:=newReaderWithOwnership(reader,ownership);ownership.Result=result;ownership.Failure=err;return result,err
}
// VisitOriginals requires the caller's actual producer/borrow joins; it is only
// SAME-object inspection, not an eligibility, native closure or custody receipt.
func(o *Ownership)VisitOriginals(visit func(*OriginalOperation)){
 if o==nil||visit==nil{return};o.mutex.Lock();head:=o.head;o.mutex.Unlock();for operation:=head;operation!=nil;operation=operation.next{visit(operation)}
}
