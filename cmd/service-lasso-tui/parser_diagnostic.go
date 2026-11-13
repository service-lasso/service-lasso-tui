package main

import "io"

// FlagSet discards output errors. The actual synchronous forwarding occurrence
// retains them independently of the parser error. Input is the actual fmt
// borrow; Bytes is its observation before fmt can reuse that backing.
type parserDiagnosticWrite struct {
 Input,Bytes []byte
 Writer io.Writer
 Written int
 Failure error
 Panic any
 Attempted,Returned,Interrupted bool
 Next *parserDiagnosticWrite
}
type parserDiagnosticWriter struct {
 Writer io.Writer
 First,Last *parserDiagnosticWrite
}
func (writer *parserDiagnosticWriter) Write(input []byte)(written int,failure error){
 occurrence:=&parserDiagnosticWrite{Input:input,Writer:writer.Writer}
 if writer.Last==nil{writer.First=occurrence}else{writer.Last.Next=occurrence};writer.Last=occurrence
 // Link the source cell before allocation or output so a partial occurrence
 // remains reachable from the already enrolled startup owner.
 func(){
  defer func(){if !occurrence.Returned{occurrence.Interrupted=true;occurrence.Panic=recover()}}()
  occurrence.Bytes=append([]byte(nil),input...)
  occurrence.Attempted=true
  written,failure=occurrence.Writer.Write(input)
  occurrence.Written=written;occurrence.Failure=failure;occurrence.Returned=true
 }()
 // A recovered nil panic returns from the inner call. Goexit does not: it
 // keeps unwinding this actual goroutine, so it never becomes a new panic.
 if occurrence.Interrupted{panic(occurrence.Panic)}
 return written,failure
}
