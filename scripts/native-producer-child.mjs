// Retain the original ChildProcess and actual pipe/event observations until
// close. A timeout/error/exit observation never substitutes for that close.
const issued=new WeakMap();
const originalChildren=new WeakSet();
export function observeNativeProducerChild(child) {
  if(originalChildren.has(child))throw new Error("original producer already observed");originalChildren.add(child);
  const state={child,errors:[],exitObserved:false,exitCode:null,exitSignal:null,closeObserved:false,closeCode:null,closeSignal:null,pipes:[]};
  const owner={child};
  for(const key of ["exitObserved","exitCode","exitSignal","closeObserved","closeCode","closeSignal"]){Object.defineProperty(owner,key,{enumerable:true,get:()=>state[key]});}
  Object.defineProperty(owner,"errors",{enumerable:true,get:()=>Object.freeze([...state.errors])});
  Object.defineProperty(owner,"pipes",{enumerable:true,get:()=>Object.freeze(state.pipes.map(pipe=>Object.freeze({...pipe,errors:Object.freeze([...pipe.errors])})))});
  for(const name of ["stdin","stdout","stderr"]){
    const stream=child[name];if(!stream)continue;
    const pipe={name,stream,closed:false,ended:false,finished:false,errors:[]};state.pipes.push(pipe);
    stream.on("error",error=>{pipe.errors.push(error);state.errors.push(error);});
    stream.once("close",()=>{pipe.closed=true;});
    stream.once("end",()=>{pipe.ended=true;});
    stream.once("finish",()=>{pipe.finished=true;});
  }
  child.on("error",error=>{state.errors.push(error);});
  child.once("exit",(code,signal)=>{state.exitObserved=true;state.exitCode=code;state.exitSignal=signal;});
  owner.completion=new Promise(resolve=>child.once("close",(code,signal)=>{
    state.closeObserved=true;state.closeCode=code;state.closeSignal=signal;resolve(owner);
  }));
  issued.set(owner,state);return Object.freeze(owner);
}
export function nativeProducerClosedSuccessfully(owner) {
  const state=issued.get(owner);if(!state)return false;
  // Local destroy/close is not readable EOF or completed writable input.
  // These observations apply only to the actual pipes acquired here; an
  // ignored stream supplies no observation and no whole-family qualification.
  return state.closeObserved&&state.exitObserved&&state.exitCode===0&&state.exitSignal===null&&state.closeCode===state.exitCode&&state.closeSignal===state.exitSignal&&state.errors.length===0&&state.pipes.every(pipe=>pipe.closed&&pipe.errors.length===0&&(pipe.name==="stdin"?pipe.finished:pipe.ended));
}
