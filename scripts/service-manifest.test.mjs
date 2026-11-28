import test from 'node:test';
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import { assembleServiceManifest, assertServiceManifest, platforms } from './service-manifest.mjs';
const source = JSON.parse(await readFile(new URL('../service.json', import.meta.url), 'utf8'));
const version = '2026.11.27-0123456';
const candidate = { version, source: { commit: '0123456' + 'a'.repeat(33) }, release: { tag: `candidate-${version}` }, assets: Object.values(platforms).map(([platform, extension, executable]) => ({ platform, name: `service-lasso-tui-${version}-${platform}.${extension}`, executable, sha256: 'b'.repeat(64) })) };
test('release manifest pins prebuilt archives for each actual architecture and on-demand command', async () => {
 const actual = assembleServiceManifest(source, candidate);
 assert.deepEqual(actual, assembleServiceManifest(source, candidate));
 assert.deepEqual(actual.artifact.source, {type:'github-release',repo:'service-lasso/service-lasso-tui',tag:candidate.release.tag});
 assert.equal(actual.id, '@tui'); assert.equal(actual.role, 'provider'); assert.equal(actual.autostart, false);
 assert.deepEqual(Object.keys(actual.artifact.platforms), Object.keys(platforms));
 await assertServiceManifest(Buffer.from(JSON.stringify(actual)), candidate);
 for (const entry of Object.values(actual.artifact.platforms)) { assert.equal(entry.sha256,'b'.repeat(64)); assert.deepEqual(entry.args, []); }
});
for (const [name, mutate] of Object.entries({
 'unknown architecture': c => { c.assets[0].platform = 'win32-arm64'; },
 'missing architecture': c => { c.assets.pop(); },
 'duplicate architecture': c => { c.assets[0] = c.assets[1]; },
 'path escape': c => { c.assets[0].name = '../escape.zip'; },
 'mismatched executable': c => { c.assets[0].executable = 'other.exe'; },
 'missing digest': c => { delete c.assets[0].sha256; },
 'mutable tag': c => { c.release.tag = 'latest'; },
 'mismatched source': c => { c.source.commit = 'a'.repeat(40); },
})) test(`rejects ${name}`, () => { const bad = structuredClone(candidate); mutate(bad); assert.throws(() => assembleServiceManifest(source,bad)); });
test('rejects altered released manifest even with a valid candidate', async () => {
 const bad = assembleServiceManifest(source, candidate); bad.artifact.platforms['linux-x64'].sha256 = 'c'.repeat(64);
 await assert.rejects(assertServiceManifest(Buffer.from(JSON.stringify(bad)), candidate));
});

test('schema4 releases exactly Windows and Linux while historical projection remains complete', async () => {
 const selected = {...structuredClone(candidate), schemaVersion:4};
 selected.assets = selected.assets.filter(asset => ['win32-amd64','linux-amd64'].includes(asset.platform));
 const actual = assembleServiceManifest(source, selected);
 assert.deepEqual(Object.keys(actual.artifact.platforms), ['win32-x64','linux-x64']);
 await assertServiceManifest(Buffer.from(JSON.stringify(actual)),selected);
 const extra = structuredClone(selected); extra.assets.push(candidate.assets[2]);
 assert.throws(() => assembleServiceManifest(source,extra));
});
