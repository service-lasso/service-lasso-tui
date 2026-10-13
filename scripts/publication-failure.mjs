// Keep actual operation/cleanup failures in order. This is private in-memory
// continuity, not a native owner, durable capture or quiescence qualification.
export class PublicationError extends Error {}
export async function withPublicationCleanup(operation, cleanup, message) {
  const causes=[];let result;let operationSucceeded=false;
  try{result=await operation();operationSucceeded=true;}catch(cause){causes.push(cause);}
  try{await cleanup();}catch(cause){causes.push(cause);}
  if(causes.length===1){if(!operationSucceeded&&causes[0] instanceof PublicationError)throw causes[0];const error=new PublicationError(message,{cause:causes[0]});if(operationSucceeded)Object.defineProperty(error,"operationResult",{value:result});throw error;}
  if(causes.length>1)throw new PublicationError(message,{cause:new AggregateError(causes,"private publication operation and cleanup failures")});
  return result;
}
