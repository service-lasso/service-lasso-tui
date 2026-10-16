// Invocation-private error ordering only. No registry, issuer, reservation,
// subprocess grant or native-family closure authority is created here.
export class CommandFailureLedger {
 constructor(){this.occurrences=[];this.originals=[];}
 record(phase,original){this.occurrences.push({phase,original});this.originals.push(original);}
 async attempt(phase,body){
  try{return {completed:true,value:await body()};}
  catch(original){this.record(phase,original);return {completed:false,original};}
 }
 throwFirst(){if(this.originals.length)throw this.originals[0];}
}
