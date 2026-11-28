import { createHash } from 'node:crypto';
import { readFile, writeFile, stat } from 'node:fs/promises';
import { assembleServiceManifest } from './service-manifest.mjs';
const { CANDIDATE_VERSION: version, CANDIDATE_TAG: tag, CANDIDATE_SHA: commit,
  CANDIDATE_PLATFORM_SCOPE: scope, GITHUB_REF: ref, GITHUB_REPOSITORY: repository } = process.env;
if (!['windows-linux', 'full'].includes(scope)) throw new Error('candidate platform scope invalid');
const targets = [['win32-amd64', 'zip', 'service-lasso-tui.exe'], ['linux-amd64', 'tar.gz', 'service-lasso-tui']];
if (scope === 'full') targets.push(['darwin-amd64', 'tar.gz', 'service-lasso-tui'], ['darwin-arm64', 'tar.gz', 'service-lasso-tui']);
const digest = bytes => createHash('sha256').update(bytes).digest('hex');
const assets = await Promise.all(targets.map(async ([platform, extension, executable]) => {
  const name = `service-lasso-tui-${version}-${platform}.${extension}`;
  return { platform, name, sha256: digest(await readFile(name)), executable };
}));
const candidate = { schemaVersion: scope === 'full' ? 3 : 4, kind: 'develop-prerelease-candidate',
  source: { repository, ref, commit }, release: { tag, prerelease: true, draft: false, immutable: true },
  version, checksumManifest: { name: 'SHA256SUMS.txt', sha256: digest(await readFile('SHA256SUMS.txt')) },
  corePackagingIssue: 'service-lasso/service-lasso#1461', assets };
await writeFile('candidate-manifest.json', JSON.stringify(candidate) + '\n', { flag: 'wx' });
const source = JSON.parse(await readFile(new URL('../service.json', import.meta.url), 'utf8'));
await writeFile('service.json', JSON.stringify(assembleServiceManifest(source, candidate), null, 2) + '\n', { flag: 'wx' });
const inventory = {};
for (const name of [...assets.map(asset => asset.name), 'SHA256SUMS.txt', 'candidate-manifest.json', 'service.json']) {
  inventory[name] = { sha256: digest(await readFile(name)), size: (await stat(name)).size };
}
await writeFile('candidate-local-assets.json', JSON.stringify(inventory) + '\n', { flag: 'wx' });
