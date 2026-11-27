import { readFile, writeFile } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import { isDeepStrictEqual } from 'node:util';

export const platforms = Object.freeze({
  'win32-x64': ['win32-amd64', 'zip', 'service-lasso-tui.exe'],
  'linux-x64': ['linux-amd64', 'tar.gz', 'service-lasso-tui'],
  'darwin-arm64': ['darwin-arm64', 'tar.gz', 'service-lasso-tui'],
  'darwin-x64': ['darwin-amd64', 'tar.gz', 'service-lasso-tui'],
});
export function assembleServiceManifest(source, candidate) {
  if (source?.id !== '@tui' || source.role !== 'provider' || source.enabled !== true || source.autostart !== false || source.artifact?.kind !== 'archive' || source.artifact.source?.repo !== 'service-lasso/service-lasso-tui') throw new Error('canonical TUI provider manifest invalid');
  if (!/^\d{4}\.\d{1,2}\.\d{1,2}-[a-f0-9]{7}$/.test(candidate.version) || candidate.release?.tag !== `candidate-${candidate.version}` || candidate.source?.commit?.slice(0, 7) !== candidate.version.slice(-7)) throw new Error('service candidate identity invalid');
  if (candidate.assets?.length !== 4 || new Set(candidate.assets.map(row => row.platform)).size !== 4) throw new Error('service platform inventory invalid');
  const manifest = structuredClone(source);
  manifest.version = candidate.version;
  manifest.artifact.source = { type: 'github-release', repo: 'service-lasso/service-lasso-tui', tag: candidate.release.tag };
  manifest.artifact.platforms = {};
  for (const [platform, [native, archiveType, executable]] of Object.entries(platforms)) {
    const asset = candidate.assets.find(row => row.platform === native);
    const name = `service-lasso-tui-${candidate.version}-${native}.${archiveType}`;
    if (asset?.name !== name || asset.executable !== executable || !/^[a-f0-9]{64}$/.test(asset.sha256)) throw new Error('service platform archive identity invalid');
    manifest.artifact.platforms[platform] = { assetName: name, archiveType, command: `./${executable}`, args: [], sha256: asset.sha256 };
  }
  return manifest;
}
export async function assertServiceManifest(bytes, candidate) {
  const canonical = JSON.parse(await readFile(new URL('../service.json', import.meta.url), 'utf8'));
  if (!isDeepStrictEqual(JSON.parse(bytes.toString('utf8')), assembleServiceManifest(canonical, candidate))) throw new Error('released service manifest differs from exact candidate');
}
if (process.argv[1] === fileURLToPath(import.meta.url)) {
  const [source, candidate, output] = process.argv.slice(2);
  if (!source || !candidate || !output || process.argv.length !== 5) throw new Error('usage: service-manifest.mjs source candidate output');
  await writeFile(output, JSON.stringify(assembleServiceManifest(JSON.parse(await readFile(source, 'utf8')), JSON.parse(await readFile(candidate, 'utf8'))), null, 2) + '\n', { flag: 'wx' });
}
