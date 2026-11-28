import test from 'node:test';
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { mkdtempSync, readFileSync, writeFileSync, rmSync, existsSync } from 'node:fs';
import os from 'node:os';
import path from 'node:path';
const root = path.resolve(import.meta.dirname, '..');
const git = (...args) => execFileSync('git', args, { cwd: root, encoding: 'utf8' }).trim();
// Execute the production YAML shell bodies, not a mirrored packaging algorithm.
for (const scope of ['full', 'windows-linux']) test(`actual ordinary workflow packages ${scope} clean stamped platforms outside source and denies dirty input`, () => {
  const tmp = mkdtempSync(path.join(os.tmpdir(), 'tui137-package-'));
  const source = path.join(tmp, 'source');
  const sha = git('rev-parse', 'HEAD');
  const workflow = readFileSync(path.join(root, '.github/workflows/release.yml'), 'utf8');
  const packageJob = workflow.split('  package-candidate:\n')[1].split('  publish-candidate:\n')[0];
  assert.match(packageJob, /CANDIDATE_ASSET_DIR: \$\{\{ runner.temp \}\}\/release-assets/);
  assert.match(packageJob, /path: \$\{\{ runner.temp \}\}\/release-assets/);
  const runMatches = [...packageJob.matchAll(/^        run: (?:\|\n((?:          .*\n|\n)+)|([^\n]+)\n)/gm)];
  const bodies = runMatches.map(match => match[1] ? match[1].replace(/^          /gm, '') : match[2]);
  assert.equal(bodies.length, 4);
  try {
    execFileSync('git', ['-c','init.defaultBranch=develop','init','-q',source]);
    execFileSync('git', ['-C',source,'fetch','-q',root,sha]);
    execFileSync('git', ['-C',source,'checkout','-q','--detach','FETCH_HEAD']);
    // Resolve literal env declarations at their actual YAML indentation. Unknown
    // expressions fail rather than being supplied by test-only defaults.
    const version = `2026.11.28-${sha.slice(0,7)}`;
    const contexts = {'needs.validate-source.outputs.platform_scope':scope,'needs.validate-source.outputs.sha':sha,'needs.validate-source.outputs.version':version,'needs.validate-source.outputs.tag':`candidate-${version}`,'runner.temp':tmp};
    const declared = (block, depth) => Object.fromEntries([...block.matchAll(new RegExp(`^ {${depth}}([A-Z_]+): (.*)$`,'gm'))].map(([,key,raw]) => [key, raw.startsWith('"') ? JSON.parse(raw) : raw.replace(/\$\{\{ ([^}]+) \}\}/g,(_,context)=>{assert.ok(Object.hasOwn(contexts,context),`unknown env expression ${context}`);return contexts[context];})]));
    const base = {...process.env,GITHUB_REF:'refs/heads/develop',GITHUB_REPOSITORY:'service-lasso/service-lasso-tui',GITHUB_WORKSPACE:source};
    for(const key of ['GOFLAGS','GOWORK','CANDIDATE_SHA','CANDIDATE_VERSION','CANDIDATE_TAG','CANDIDATE_ASSET_DIR']) delete base[key];
    const jobEnv = declared(packageJob.split('    steps:')[0],6);
    const environments = runMatches.map(match => ({...base,...jobEnv,...declared(packageJob.slice(packageJob.lastIndexOf('      - ',match.index),match.index),10)}));
    for (const [index, body] of bodies.entries()) execFileSync('bash',['-c',body],{cwd:source,env:environments[index],stdio:'pipe'});
    // The old final guard had no step-level flags. Removing job GOWORK must
    // reproduce its real failure, even when the parent test process sets it.
    const missingGuardEnv = {...environments.at(-1)}; delete missingGuardEnv.GOWORK;
    assert.throws(()=>execFileSync('bash',['-c',bodies.at(-1)],{cwd:source,env:missingGuardEnv,stdio:'pipe'}),error=>String(error.stderr).includes('GOWORK must be explicitly off'));
    assert.equal(execFileSync('git',['status','--porcelain=v1','--untracked-files=all'],{cwd:source,encoding:'utf8'}).trim(),'');
    assert.equal(existsSync(path.join(source,'release-assets')),false);
    const inventory = JSON.parse(readFileSync(path.join(tmp,'release-assets','candidate-local-assets.json')));
    assert.equal(Object.keys(inventory).length,scope === 'full' ? 7 : 5);
    for (const mode of ['tracked','untracked']) {
      const file = path.join(source,mode === 'tracked'?'README.md':'unexpected-output');
      const original = mode === 'tracked' ? readFileSync(file) : null;
      writeFileSync(file,'dirty source');
      assert.throws(()=>execFileSync('bash',['-c',bodies[0]],{cwd:source,env:environments[0],stdio:'pipe'}),error=>String(error.stderr).includes('source checkout must be clean'));
      if(original) writeFileSync(file,original); else rmSync(file);
    }
  } finally { rmSync(tmp,{recursive:true,force:true}); }
});
