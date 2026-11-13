//go:build linux

package lifetime

// NewNativePayloadCommand uses the actual private generated construction once.
// Raw arguments are retained aliases, never resource funding or authority.
// Every partial startup remains on the SAME generated owner/Receiver; callers
// must complete original custody/disposition before any process exit. This
// constructor supplies neither a native gate nor a disposal decision.
func NewNativePayloadCommand(rawArguments []string)(*NativePayloadStartup,*Receiver,error){
 startup,err:=newNativeCandidateStartupRoot(rawArguments)
 if err!=nil{if startup!=nil{return startup,startup.receiverOriginal,err};return nil,nil,err}
 if startup==nil||startup.receiverOriginal==nil{return startup,nil,errOriginalEnrollment}
 receiver:=startup.receiverOriginal
 err=receiver.PrepareNativePayloadStartup(startup)
 return startup,receiver,err
}
