import { parseStrictJSON } from "./scoped-json.mjs";
import { performance } from "node:perf_hooks";
import { resolveNativeProducerHandoffPipe } from "./native-producer-child.mjs";

// This bounds only the existing readiness handoff. It neither terminates the
// original child nor certifies its closure or descendant custody.
export function readNativeProducerHandoff(child,...selection) {
  const pipe=resolveNativeProducerHandoffPipe(child,...selection);
  if(!pipe)return Promise.reject(new Error("input handoff pipe original"));
  return new Promise((resolve,reject)=>{
    let bytes=Buffer.alloc(0),settled=false;
    const started=performance.now(),deadline=started+30000;
    const timeoutError=()=>new Error("input handoff timeout",{cause:{elapsedMilliseconds:performance.now()-started,budgetMilliseconds:30000}});
    const finish=(error,value)=>{
      if(settled)return;settled=true;clearTimeout(timer);
      child.removeListener("exit",ended);child.removeListener("error",failed);
      pipe.removeListener("data",received);
      pipe.removeListener("error",failed);pipe.removeListener("end",streamEnded);
      if(error)reject(error);else resolve(value);
    };
    const ended=()=>finish(new Error("input writer ended"));
    const failed=error=>finish(new Error("input writer failed",{cause:error}));
    const streamEnded=()=>finish(new Error("input writer stream ended"));
    const received=chunk=>{
      if(settled)return;
      if(performance.now()>=deadline){finish(timeoutError());return;}
      if(chunk.length>1048576-bytes.length){finish(new Error("input handoff quota",{cause:{retainedBytes:bytes.length,receivedChunkBytes:chunk.length,limitBytes:1048576}}));return;}
      bytes=Buffer.concat([bytes,chunk]);
      if(bytes.includes(10)){
        try{const ready=parseStrictJSON(bytes);if(performance.now()>=deadline)finish(timeoutError());else finish(null,ready);}
        catch(error){finish(new Error("input handoff shape",{cause:error}));}
      }
    };
    const timer=setTimeout(()=>finish(timeoutError()),30000);
    child.once("exit",ended);child.once("error",failed);pipe.on("data",received);
    pipe.once("error",failed);pipe.once("end",streamEnded);
  });
}
