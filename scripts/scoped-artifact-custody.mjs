import { inflateRawSync } from "node:zlib";
import { crc32 } from "./scoped-native-archive.mjs";
import { hash } from "./scoped-native-contract.mjs";
const LIMIT=512*1024*1024;
const deny=()=>{throw new Error("native artifact custody denied");};
export function extractArtifactZIP(bytes,expectedNames){
  if(bytes.length<22||bytes.length>LIMIT)deny();let end=-1;
  for(let at=bytes.length-22;at>=Math.max(0,bytes.length-65557);at--)if(bytes.readUInt32LE(at)===0x06054b50&&at+22+bytes.readUInt16LE(at+20)===bytes.length){end=at;break;}
  if(end<0||bytes.readUInt16LE(end+4)||bytes.readUInt16LE(end+6)||bytes.readUInt16LE(end+20))deny();
  const count=bytes.readUInt16LE(end+10),start=bytes.readUInt32LE(end+16),length=bytes.readUInt32LE(end+12);
  if(count!==expectedNames.length||count!==bytes.readUInt16LE(end+8)||start+length!==end||count>16)deny();
  const names=new Set(expectedNames),entries=[],files={};let central=start,total=0;
  for(let index=0;index<count;index++){
    if(central+46>end||bytes.readUInt32LE(central)!==0x02014b50)deny();
    const flags=bytes.readUInt16LE(central+8),method=bytes.readUInt16LE(central+10),crc=bytes.readUInt32LE(central+16),compressed=bytes.readUInt32LE(central+20),size=bytes.readUInt32LE(central+24),nameLength=bytes.readUInt16LE(central+28),extraLength=bytes.readUInt16LE(central+30),commentLength=bytes.readUInt16LE(central+32),local=bytes.readUInt32LE(central+42);
    if(flags&~0x808||![0,8].includes(method)||!size||size>LIMIT||compressed>LIMIT||nameLength<1||nameLength>200||extraLength>4096||commentLength||bytes.readUInt16LE(central+34)||central+46+nameLength+extraLength>end)deny();
    const nameBytes=bytes.subarray(central+46,central+46+nameLength);const name=nameBytes.toString("ascii");if(!nameBytes.equals(Buffer.from(name,"ascii"))||!names.has(name)||Object.hasOwn(files,name)||!/^[A-Za-z0-9._-]+$/u.test(name))deny();
    const attributes=bytes.readUInt32LE(central+38),mode=attributes>>>16;if((attributes&16)||((mode&0xf000)!==0&&(mode&0xf000)!==0x8000))deny();
    const extras=(offset,limit)=>{while(offset<limit){if(offset+4>limit)deny();const id=bytes.readUInt16LE(offset),size=bytes.readUInt16LE(offset+2);if(id===1||offset+4+size>limit)deny();offset+=4+size;}};
    extras(central+46+nameLength,central+46+nameLength+extraLength);
    if(local+30>start||bytes.readUInt32LE(local)!==0x04034b50||bytes.readUInt16LE(local+6)!==flags||bytes.readUInt16LE(local+8)!==method||bytes.readUInt16LE(local+26)!==nameLength)deny();
    const localExtra=bytes.readUInt16LE(local+28),data=local+30+nameLength+localExtra;if(localExtra>4096||data+compressed>start||!bytes.subarray(local+30,local+30+nameLength).equals(nameBytes))deny();extras(local+30+nameLength,data);
    if(!(flags&8)&&(bytes.readUInt32LE(local+14)!==crc||bytes.readUInt32LE(local+18)!==compressed||bytes.readUInt32LE(local+22)!==size))deny();
    const packed=bytes.subarray(data,data+compressed);let body;try{body=method===0?packed:inflateRawSync(packed,{maxOutputLength:Math.min(size,LIMIT)});}catch{deny();}
    if(body.length!==size||crc32(body)!==crc||(total+=size)>LIMIT)deny();files[name]=body;
    let finish=data+compressed;if(flags&8){const signature=bytes.readUInt32LE(finish)===0x08074b50;if(signature)finish+=4;if(finish+12>start||bytes.readUInt32LE(finish)!==crc||bytes.readUInt32LE(finish+4)!==compressed||bytes.readUInt32LE(finish+8)!==size)deny();finish+=12;}
    entries.push({local,finish});central+=46+nameLength+extraLength;
  }
  if(central!==end)deny();entries.sort((a,b)=>a.local-b.local);let cursor=0;for(const entry of entries){if(entry.local!==cursor)deny();cursor=entry.finish;}if(cursor!==start)deny();return files;
}
export async function verifyProviderArtifactBytes({artifact,expected,token,fetchImpl=fetch}){
  if(!token||!Number.isSafeInteger(artifact.id)||artifact.id<=0||!/^sha256:[a-f0-9]{64}$/u.test(artifact.digest)||artifact.expired!==false)deny();
  const controller=new AbortController();let timer;
  try{return await Promise.race([new Promise((_,reject)=>{timer=setTimeout(()=>{controller.abort();reject(new Error("artifact deadline"));},30000);}), (async()=>{
    const response=await fetchImpl(`https://api.github.com/repos/service-lasso/service-lasso-tui/actions/artifacts/${artifact.id}/zip`,{redirect:"manual",signal:controller.signal,headers:{Authorization:`Bearer ${token}`,Accept:"application/vnd.github+json"}});
    if(response.status!==302)deny();const location=response.headers.get("location");let url;try{url=new URL(location);}catch{deny();}
    if(url.protocol!=="https:"||url.username||url.password||url.port||url.hash||!/^productionresultssa[a-z0-9]+\.blob\.core\.windows\.net$/u.test(url.hostname)||!url.pathname.startsWith("/actions-results/")||url.pathname.includes("..")||!["sig","se","sp","sv"].every(key=>url.searchParams.getAll(key).length===1&&url.searchParams.get(key))||location.length>16384)deny();
    const download=await fetchImpl(url.href,{redirect:"error",signal:controller.signal});if(download.status!==200||!download.body?.[Symbol.asyncIterator])deny();
    const chunks=[];let size=0;for await(const chunk of download.body){const buffer=Buffer.from(chunk);if((size+=buffer.length)>LIMIT)deny();chunks.push(buffer);}const bytes=Buffer.concat(chunks);
    if(hash(bytes)!==artifact.digest.slice(7))deny();const files=extractArtifactZIP(bytes,Object.keys(expected));for(const [name,original] of Object.entries(expected))if(!files[name].equals(original))deny();return {id:artifact.id,sha256:hash(bytes),size:bytes.length};
  })()]);}finally{clearTimeout(timer);controller.abort();}
}
