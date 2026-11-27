import test from 'node:test';
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { mkdtempSync, readFileSync, writeFileSync, rmSync, existsSync } from 'node:fs';
import os from 'node:os';
import path from 'node:path';
const root = path.resolve(import.meta.dirname, '..');
const git = (...args) => execFileSync('git', args, { cwd: root, encoding: 'utf8' }).trim();
// Execute the production YAML shell bodies, not a mirrored packaging algorithm.
test('actual ordinary workflow packages four clean stamped platforms outside source and denies dirty input', () => {
  const tmp = mkdtempSync(path.join(os.tmpdir(), 'tui137-package-'));
  const source = path.join(tmp, 'source');
  const sha = git('rev-parse', 'HEAD');
  const workflow = readFileSync(path.join(root, '.github/workflows/release.yml'), 'utf8');
  const packageJob = workflow.split('  package-candidate:\n')[1].split('  publish-candidate:\n')[0];
  assert.match(packageJob, /CANDIDATE_ASSET_DIR: \$\{\{ runner.temp \}\}\/release-assets/);
  assert.match(packageJob, /path: \$\{\{ env.CANDIDATE_ASSET_DIR \}\}/);
  const bodies = [...packageJob.matchAll(/^        run: (?:\|\n((?:          .*\n|\n)+)|([^\n]+)\n)/gm)].map(match => match[1] ? match[1].replace(/^          /gm, '') : match[2]);
  assert.equal(bodies.length, 4);
  try {
    execFileSync('git', ['-c','init.defaultBranch=develop','init','-q',source]);
    execFileSync('git', ['-C',source,'fetch','-q',root,sha]);
    execFileSync('git', ['-C',source,'checkout','-q','--detach','FETCH_HEAD']);
    const env = {...process.env, GOFLAGS:'',GOWORK:'off',GITHUB_REF:'refs/heads/develop',GITHUB_REPOSITORY:'service-lasso/service-lasso-tui',GITHUB_WORKSPACE:source,CANDIDATE_SHA:sha,CANDIDATE_VERSION:`2026.11.28-${sha.slice(0,7)}`,CANDIDATE_TAG:`candidate-2026.11.28-${sha.slice(0,7)}`,CANDIDATE_ASSET_DIR:path.join(tmp,'release-assets')};
    for (const body of bodies) execFileSync('bash',['-c',body],{cwd:source,env,stdio:'pipe'});
    assert.equal(execFileSync('git',['status','--porcelain=v1','--untracked-files=all'],{cwd:source,encoding:'utf8'}).trim(),'');
    assert.equal(existsSync(path.join(source,'release-assets')),false);
    const inventory = JSON.parse(readFileSync(path.join(env.CANDIDATE_ASSET_DIR,'candidate-local-assets.json')));
    assert.equal(Object.keys(inventory).length,7);
    for (const mode of ['tracked','untracked']) {
      const file = path.join(source,mode === 'tracked'?'README.md':'unexpected-output');
      const original = mode === 'tracked' ? readFileSync(file) : null;
      writeFileSync(file,'dirty source');
      assert.throws(()=>execFileSync('bash',['-c',bodies[0]],{cwd:source,env,stdio:'pipe'}),error=>String(error.stderr).includes('source checkout must be clean'));
      if(original) writeFileSync(file,original); else rmSync(file);
    }
  } finally { rmSync(tmp,{recursive:true,force:true}); }
});
