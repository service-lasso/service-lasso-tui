import test from 'node:test';
import assert from 'node:assert/strict';
import {generateWindowsQueryRegions} from './generate-windows-query-regions.mjs';

// Synthetic source-carrier tests only. Digests below are representation inputs,
// never genuine provider/profile identity, containing finance or native fit.
function fixture(){return {schemaVersion:3,sourceCommit:'0'.repeat(40),providerProfileSHA256:'0'.repeat(64),
  regions:Object.fromEntries(['endpointSecurity','processSecurity','tokenAccess'].map(name=>[name,{capacity:1,originalSHA256:'0'.repeat(64)}])),
  arguments:{moduleUnitsIncludingNUL:1,originalSHA256:'0'.repeat(64)},
  argumentWriter:{stackReservationBytes:'1',turnCapacity:'1',originalSHA256:'0'.repeat(64)},
  creatorImages:{moduleCapacity:'1',mappedNameUnitsIncludingNUL:1,fileBytes:'1',memoryBytes:'1',memoryRegionCapacity:'1',originalSHA256:'0'.repeat(64)}};}
function generate(profile){return generateWindowsQueryRegions(Buffer.from(JSON.stringify(profile),'utf8'));}

test('private native dimensions retain full unsigned64 representation independently of child static geometry',()=>{
 const low=generate(fixture()),profile=fixture();
 profile.argumentWriter.stackReservationBytes='18446744073709551615';
 profile.argumentWriter.turnCapacity='9007199254740993';
 const high=generate(profile);
 assert.match(high.header,/#define SL_ARGUMENT_WRITER_STACK_RESERVATION_BYTES UINT64_C\(18446744073709551615\)/u);
 assert.match(high.header,/#define SL_ARGUMENT_WRITER_TURN_CAPACITY UINT64_C\(9007199254740993\)/u);
 assert.deepEqual(high.argumentWriterDimensions,{stackReservationBytes:'18446744073709551615',turnCapacity:'9007199254740993'});
 assert.equal(high.argumentStaticBytes,low.argumentStaticBytes);
 assert.equal(high.headerReaderStaticBytes,'20448');
 assert.equal(high.headerInterpretStaticBytes,'64');
 // Synthetic M1: BodyBytes=2*(32767+1)=65536, exact128B retained records.
 assert.equal(high.bodyReaderStaticBytes,(65536n*128n+48n).toString());
 assert.match(high.go,/unsafe.Sizeof\(serviceLassoHeaderReadOriginal\)/u);
 assert.equal(high.classification,'build-association-only');
});
test('creator image source envelopes keep separate snapshot and mapped-name domains',()=>{
 const profile=fixture();profile.creatorImages.moduleCapacity='2';
 profile.creatorImages.mappedNameUnitsIncludingNUL=3;
 const result=generate(profile);
 assert.equal(result.creatorImageBackingBytes,'194'); // 2*16 + 2*2*(3+17)*2 + 1+1.
 assert.equal(result.creatorImageMappingBytes,'1984');
 assert.equal(result.creatorImageNativePEOriginalBytes,'704');
 assert.match(result.go,/serviceLassoCreatorNamesBefore \[2\]\[3\]uint16/u);
 assert.match(result.go,/serviceLassoCreatorMappingsAfter \[2\]serviceLassoCreatorImageMappingStorage/u);
 for(const value of ['0','01','536870912','18446744073709551616',1]){
  const bad=fixture();bad.creatorImages.moduleCapacity=value;assert.throws(()=>generate(bad));
 }
 const overflow=fixture();overflow.creatorImages.fileBytes='18446744073709551615';
 assert.throws(()=>generate(overflow));
 const old=fixture();old.schemaVersion=2;assert.throws(()=>generate(old));
});
test('closed carrier rejects noncanonical and out-of-domain dimensions without coercion',()=>{
 for(const name of ['stackReservationBytes','turnCapacity']){
  for(const value of ['0','01','-1','1e3','1.0',' 1','1 ','18446744073709551616',1,null]){
   const profile=fixture();profile.argumentWriter[name]=value;
   assert.throws(()=>generate(profile));
  }
 }
 const old=fixture();old.schemaVersion=1;assert.throws(()=>generate(old));
 const extra=fixture();extra.argumentWriter.authorized=true;assert.throws(()=>generate(extra));
 const missing=fixture();delete missing.argumentWriter.originalSHA256;assert.throws(()=>generate(missing));
});
