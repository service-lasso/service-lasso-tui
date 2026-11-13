package main

import (
 "errors"
 "testing"
)
type diagnosticRefusal struct{cause error;body []byte;calls int}
func(w *diagnosticRefusal)Write(body []byte)(int,error){w.calls++;w.body=body;return 0,w.cause}
func TestStartupDiagnosticRetainsActualWriteFailureAndOriginal(t *testing.T){
 t.Setenv(startupProbeNonceEnvironment,"")
 original:=errors.New("private startup input");writeFailure:=errors.New("private refused diagnostic writer")
 writer:=&diagnosticRefusal{cause:writeFailure};observation:=&startupDiagnosticObservation{}
 reportStartupFailureObserved(observation,writer,startupBoundaryAPIClientError,original)
 if observation.Original!=original||observation.Writer!=writer||observation.WriteError!=writeFailure||!observation.WriteAttempted||observation.Written!=0||writer.calls!=1{t.Fatal("actual original/write result lost")}
 if observation.Output==original.Error(){t.Fatal("raw original projected to ordinary output")}
}
