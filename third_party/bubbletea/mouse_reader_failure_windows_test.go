//go:build windows

package tea

import (
 "errors"
 "testing"
)

type mouseFailureReader struct{failure error;closes int}
func(r *mouseFailureReader)Read([]byte)(int,error){return 0,r.failure}
func(r *mouseFailureReader)Close()error{r.closes++;return r.failure}
func(r *mouseFailureReader)Cancel()bool{return true}

// UNRUN. Exercise both real message branches and their completed consumer
// occurrence, with an original reader-retirement failure rather than native IO.
func TestMouseReaderFailureCompletesConsumerAndStops(t *testing.T){
 for _,disable:=range []bool{false,true}{
  failure:=errors.New("retained reader failure")
  reader:=&mouseFailureReader{failure:failure}
  p:=&Program{renderer:&nilRenderer{},mouseMode:disable,cancelReader:reader,cancelReaderEpisode:&inputReaderEpisode{Reader:reader}}
  var message Msg=enableMouseCellMotionMsg{}
  if disable{message=disableMouseMsg{}}
  input:=&sourceMessageOriginal{Message:message}
  occurrence:=p.sourceStart("test.mouse-consumer",p,input)
  completed:=0
  model:=&testModel{}
  result,err,stop:=p.consumeMessage(model,nil,sourceMessage{occurrence:occurrence,message:message,completed:func(actual *SourceOccurrence){if actual!=occurrence{t.Fatal("consumer original changed")};completed++}})
  if result!=model||!errors.Is(err,failure)||!stop||!input.ConsumerReturned||!occurrence.Returned||completed!=1||reader.closes!=1{t.Fatal("mouse reader failure did not stop with its completed original")}
 }
}
