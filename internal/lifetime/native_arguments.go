package lifetime

import "errors"

const NativeArgumentVersion="service-lasso-tui-native-v2"
var errNativeArguments=errors.New("native payload argument framing invalid")

// NativeArguments owns the actual private argv and derived public string aliases.
// Prefix validity is only framing, never native authority or endpoint admission.
type NativeArguments struct {
 Original Original
 Raw,Public []string
 PublicZero string
 PublicTail []string
 Descriptor,PublicCount int
 SecurityLabelBytes,TerminalCalls,RepresentationBytes int
 DescriptorText,CountText string
 SecurityText,TerminalText,RepresentationText string
 Attempted,Returned bool
 Failure error
}
func canonicalNativeNumber(text string)(int,bool){
 if len(text)==0||len(text)>1&&text[0]=='0'{return 0,false}
 value:=0;maximum:=int(^uint(0)>>1)
 for index:=0;index<len(text);index++{digit:=text[index]-'0';if digit>9||value>(maximum-int(digit))/10{return 0,false};value=value*10+int(digit)}
 return value,true
}
func(r *Receiver)ObserveNativeArguments(o *NativeArguments)error {
 return r.observeNativeArguments(o,true)
}
// The native caller uses original argv spans; only the compatibility observer
// requests a contiguous projected Public string-header allocation.
func(r *Receiver)observeNativeArguments(o *NativeArguments,projectPublic bool)error {
 if r==nil||o==nil{return errOriginalEnrollment}
 if err:=r.Enroll(&o.Original,"native-payload.private-argv",o);err!=nil{return err}
 if o.Attempted{return errOriginalEnrollment};o.Attempted=true
 fail:=func()error{o.Failure=errNativeArguments;o.Returned=true;return o.Failure}
 if len(o.Raw)<8||o.Raw[1]!=NativeArgumentVersion||o.Raw[7]!="--"{return fail()}
 o.DescriptorText=o.Raw[2];o.CountText=o.Raw[3]
 descriptor,ok:=canonicalNativeNumber(o.DescriptorText);if !ok||descriptor<3{return fail()}
 count,ok:=canonicalNativeNumber(o.CountText);if !ok||count==0&&(len(o.Raw)!=8||o.Raw[0]!="")||count>0&&count!=len(o.Raw)-7{return fail()}
 o.SecurityText=o.Raw[4];o.TerminalText=o.Raw[5];o.RepresentationText=o.Raw[6]
 security,ok:=canonicalNativeNumber(o.SecurityText);if !ok||security<1||uint64(security)>uint64(^uint32(0)){return fail()}
 terminal,ok:=canonicalNativeNumber(o.TerminalText);if !ok||terminal<1{return fail()}
 representation,ok:=canonicalNativeNumber(o.RepresentationText);if !ok||representation<1{return fail()}
 o.SecurityLabelBytes=security;o.TerminalCalls=terminal;o.RepresentationBytes=representation
 o.Descriptor=descriptor;o.PublicCount=count
 if count>0{o.PublicZero=o.Raw[0];o.PublicTail=o.Raw[8:len(o.Raw):len(o.Raw)]}
 if projectPublic&&count>0{o.Public=make([]string,count);o.Public[0]=o.PublicZero;copy(o.Public[1:],o.PublicTail)}
 o.Returned=true;return nil
}