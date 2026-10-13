package api

import (
 "context"
 "io"
 "net/http"
 "github.com/service-lasso/service-lasso-tui/internal/lifetime"
)

// requestOccurrence exists before marshal, request construction or transport.
// Destination and Result retain actual decoded objects alongside raw/body/error
// aliases; these are local originals, not archive acceptance or effect proof.
type requestOccurrence struct {
 original lifetime.Original
 Client *Client
 Context context.Context
 Method,Path string
 Input any
 Encoded []byte
 Source io.Reader
 Request *http.Request
 Response *http.Response
 Observation *operatorResponseObservation
 Destination any
 Result any
 Failure error
}
func(c *Client)beginRequest(ctx context.Context,method,path string,input any)(*requestOccurrence,error){
 occurrence:=&requestOccurrence{Client:c,Context:ctx,Method:method,Path:path,Input:input}
 receiver:=lifetime.FromContext(ctx);if receiver==nil{receiver=c.receiver}
 if err:=receiver.Enroll(&occurrence.original,"api.request",occurrence);err!=nil{return occurrence,err}
 return occurrence,nil
}
func readObservedResponse(observation *operatorResponseObservation,response *http.Response,limit int)([]byte,error,*operatorResponseObservation){
 observation.Response=response;observation.Body=response.Body;observation.Status=response.StatusCode;observation.Limit=limit
 return finishObservedResponse(observation)
}

type connectionOccurrence struct {
 Original lifetime.Original
 Options ConnectionOptions
 Opener func(string)(io.ReadCloser,error)
 Input *connectionInputObservation
 Destination any
 Manager *ConnectionManager
 Client *Client
 Failure error
}
