// Retain the original ChildProcess and actual pipe/event observations until
// close. A timeout/error/exit observation never substitutes for that close.
const issued=new WeakMap();
const originalChildren=new WeakSet();
const observedChildren=new WeakMap();
export function observeNativeProducerChild(child) {
  if(originalChildren.has(child))throw new Error("original producer already observed");originalChildren.add(child);
  const state={child,stdoutOriginal:child.stdout,errors:[],exitObserved:false,exitCode:null,exitSignal:null,closeObserved:false,closeCode:null,closeSignal:null,pipes:[]};
  const owner={child};
  for(const key of ["exitObserved","exitCode","exitSignal","closeObserved","closeCode","closeSignal"]){Object.defineProperty(owner,key,{enumerable:true,get:()=>state[key]});}
  Object.defineProperty(owner,"errors",{enumerable:true,get:()=>Object.freeze([...state.errors])});
  Object.defineProperty(owner,"pipes",{enumerable:true,get:()=>Object.freeze(state.pipes.map(pipe=>Object.freeze({...pipe,indices:Object.freeze([...pipe.indices]),errors:Object.freeze([...pipe.errors])})))});
  const originals=[child.stdin,child.stdout,child.stderr,...child.stdio.slice(3)];
  for(let index=0;index<originals.length;index++){
    const stream=originals[index];if(!stream)continue;
    const alias=state.pipes.find(pipe=>pipe.stream===stream);
    if(alias){alias.indices.push(index);continue;}
    const name=["stdin","stdout","stderr"][index]??`stdio[${index}]`;
    const pipe={name,indices:[index],stream,readable:stream.readable,writable:stream.writable,closed:false,closeCount:0,ended:false,finished:false,errors:[]};state.pipes.push(pipe);
    stream.on("error",error=>{pipe.errors.push(error);state.errors.push(error);});
    stream.on("close",()=>{pipe.closed=true;pipe.closeCount++;});
    stream.once("end",()=>{pipe.ended=true;});
    stream.once("finish",()=>{pipe.finished=true;});
  }
  child.on("error",error=>{state.errors.push(error);});
  child.once("exit",(code,signal)=>{state.exitObserved=true;state.exitCode=code;state.exitSignal=signal;});
  owner.completion=new Promise(resolve=>child.once("close",(code,signal)=>{
    state.closeObserved=true;state.closeCode=code;state.closeSignal=signal;resolve(owner);
  }));
  issued.set(owner,state);observedChildren.set(child,state);return Object.freeze(owner);
}
// Actual already-observed JS originals only; no OS identity/issuer/readiness
// authority. Mutable descriptor arrays and public projections cannot add rows.
export function resolveNativeProducerHandoffPipe(child,...selection) {
  const state=observedChildren.get(child);if(!state||selection.length>1)return null;
  const stream=selection.length?selection[0]:state.stdoutOriginal;
  const pipe=state.pipes.find(row=>row.stream===stream);
  return pipe&&pipe.readable&&!pipe.indices.includes(0)?pipe.stream:null;
}
export function nativeProducerClosedSuccessfully(owner) {
  const state=issued.get(owner);if(!state)return false;
  // Local destroy/close is not readable EOF or completed writable input.
  // These observations apply only to the actual pipes acquired here; an
  // ignored stream supplies no observation and no whole-family qualification.
  return state.closeObserved&&state.exitObserved&&state.exitCode===0&&state.exitSignal===null&&state.closeCode===state.exitCode&&state.closeSignal===state.exitSignal&&state.errors.length===0&&state.pipes.every(pipe=>pipe.closed&&pipe.closeCount===1&&pipe.errors.length===0&&(pipe.name==="stdin"?pipe.finished:pipe.name==="stdout"||pipe.name==="stderr"?pipe.ended:(!pipe.readable||pipe.ended)&&(!pipe.writable||pipe.finished)));
}
