import {spawn, spawnSync} from 'node:child_process';
import {createHash} from 'node:crypto';
import {once} from 'node:events';
import {access, readFile, stat, writeFile} from 'node:fs/promises';
import path from 'node:path';
import {fileURLToPath} from 'node:url';

const scripts = path.dirname(fileURLToPath(import.meta.url));
const arg = name => {
  const index = process.argv.indexOf(name);
  return index < 0 ? undefined : process.argv[index + 1];
};
const ACTIONS = ['install', 'config', 'start', 'stop', 'restart'];
const CORE_ACTIONS = ['service_install', 'service_configure', 'service_start', 'service_stop', 'service_restart'];
const fail = message => {
  throw new Error(message);
};
const digest = bytes => createHash('sha256').update(bytes).digest('hex');
const commit = value => typeof value === 'string' && /^[a-f0-9]{40}$/u.test(value);

export function assertSummary(value) {
  if (!value || typeof value !== 'object' || Array.isArray(value)) fail('native lifecycle summary shape invalid');
  if (value.schemaVersion !== 1 || value.kind !== 'tui23-windows-native-lifecycle' || value.status !== 'passed') fail('native lifecycle summary identity invalid');
  if (!commit(value.sourceCommit) || !commit(value.coreCommit)) fail('native lifecycle source identity invalid');
  if (typeof value.binarySHA256 !== 'string' || !/^[a-f0-9]{64}$/u.test(value.binarySHA256)) fail('native lifecycle binary identity invalid');
  if (JSON.stringify(value.actions) !== JSON.stringify(ACTIONS)) fail('native lifecycle actions invalid');
  if (value.operationCount !== 5 || !Array.isArray(value.operations) || value.operations.length !== 5) fail('native lifecycle operation count invalid');
  const operations = value.operations;
  if (new Set(operations.map(row => row.operationId)).size !== 5) fail('native lifecycle operation identity invalid');
  if (operations.map(row => row.action).sort().join(',') !== [...CORE_ACTIONS].sort().join(',')) fail('native lifecycle operation actions invalid');
  if (operations.some(row => row.status !== 'succeeded' || row.outcome !== 'succeeded' || row.cancellationSupported !== false || JSON.stringify(row.targetIds) !== JSON.stringify(['tui20-fixture']))) fail('native lifecycle operation result invalid');
  if (!Array.isArray(value.coreAudit) || value.coreAudit.length !== 5 || value.coreAudit.some(row => row.coreAuditCount !== 1 || !operations.some(operation => operation.operationId === row.operationId))) fail('native lifecycle Core audit invalid');
  if (!Array.isArray(value.terminals) || value.terminals.length !== 2) fail('native lifecycle terminals invalid');
  if (value.terminals.map(row => row.terminal + '=' + row.exit).sort().join(',') !== 'allowed=terminal_exited_zero,reconnect=terminal_exited_zero') fail('native lifecycle terminal exits invalid');
  if (value.reconnect?.completedOperationNoReplay !== true) fail('native lifecycle reconnect invalid');
  if (value.unrelatedService?.id !== 'tui20-unrelated' || value.unrelatedService?.unchangedAfterFiveActions !== true) fail('native lifecycle unrelated service invalid');
  if (value.resize !== true || value.previewFrozen !== true) fail('native lifecycle terminal workflow invalid');
  if (value.ownedRuntimeClosed !== true) fail('native lifecycle runtime closure invalid');
  return value;
}

function absolute(value) {
  if (typeof value !== 'string' || !path.isAbsolute(value) || value.includes('\0')) fail('native lifecycle absolute path required');
  return value;
}

async function main() {
  if (process.platform !== 'win32' || process.arch !== 'x64') fail('Windows amd64 native lifecycle host required');
  const root = absolute(arg('--root'));
  const executable = absolute(arg('--executable'));
  const sourceCommit = arg('--source-commit');
  const coreCommit = arg('--core-commit');
  const python = arg('--python') ?? 'python';
  const node = arg('--node') ?? process.execPath;
  const go = arg('--go') ?? 'go';
  const output = absolute(arg('--output') ?? root);
  if (!commit(sourceCommit) || !commit(coreCommit)) fail('exact TUI and Core revisions required');
  for (const file of [path.join(root, 'core-source', 'dist', 'server', 'index.js'), path.join(root, 'core-source', 'node_modules', 'jose', 'dist', 'webapi', 'index.js'), path.join(root, 'core-source', 'tests', 'test-helpers.js')]) {
    try { await access(file); } catch { fail('built Core source required at ' + file); }
  }
  const executableStat = await stat(executable);
  if (!executableStat.isFile() || executableStat.size <= 0) fail('exact TUI executable required');
  const metadata = spawnSync(go, ['version', '-m', executable], {encoding: 'utf8'});
  if (metadata.status !== 0) fail('TUI build metadata unavailable');
  if (!new RegExp('^\\s*build\\s+vcs\\.revision=' + sourceCommit + '$', 'mu').test(metadata.stdout) || !/^\s*build\s+vcs\.modified=false$/mu.test(metadata.stdout)) fail('VCS-stamped exact TUI binary required');
  const binarySHA256 = digest(await readFile(executable));
  const observer = path.join(scripts, 'tui28-native-owner-observer.py');
  const owner = path.join(scripts, 'tui23-windows-lifecycle.py');
  const runtime = path.join(scripts, 'tui20-native-runtime.mjs');
  for (const file of [observer, owner, runtime]) await access(file);
  const child = spawn(python, ['-s', observer, '--owner', owner, '--root', root, '--executable', executable, '--source-commit', sourceCommit, '--core-commit', coreCommit, '--runtime-script', runtime, '--node', node], {env: process.env, stdio: ['ignore', 'pipe', 'pipe']});
  const stdout = [], stderr = [];
  let stdoutSize = 0, stderrSize = 0;
  child.stdout.on('data', chunk => { if (stdoutSize < 65536) { stdout.push(chunk); stdoutSize += chunk.length; } });
  child.stderr.on('data', chunk => { if (stderrSize < 65536) { stderr.push(chunk); stderrSize += chunk.length; } });
  const timer = setTimeout(() => { child.kill('SIGKILL'); }, 900000);
  const [code] = await once(child, 'exit');
  clearTimeout(timer);
  await writeFile(path.join(root, 'runner.stdout.private'), Buffer.concat(stdout), {mode: 0o600});
  await writeFile(path.join(root, 'runner.stderr.private'), Buffer.concat(stderr), {mode: 0o600});
  let receipt, projection;
  try { receipt = JSON.parse(await readFile(path.join(root, 'native-exit-receipt.json'), 'utf8')); } catch { fail('native exit receipt missing'); }
  try { projection = JSON.parse(await readFile(path.join(root, 'native-public-projection.json'), 'utf8')); } catch { fail('native public projection missing'); }
  if (code !== 0) fail('Windows native five-action lifecycle failed' + (receipt.failureStage ? ' at ' + receipt.failureStage : '') + (receipt.failureDetail ? ': ' + receipt.failureDetail : ''));
  let audit;
  try { audit = JSON.parse(await readFile(path.join(root, 'operation-audit.json'), 'utf8')); } catch { fail('native operation audit missing'); }
  if (receipt.outcome !== 'succeeded') fail('Windows native five-action lifecycle outcome invalid');
  if (receipt.candidateIdentity?.sourceCommit !== sourceCommit || receipt.candidateIdentity?.binarySHA256 !== binarySHA256 || receipt.coreCommit !== coreCommit) fail('native lifecycle receipt identity invalid');
  if (projection.result !== 'succeeded' || projection.actionsPassed !== true || projection.ownedRuntimeClosed !== true) fail('native lifecycle public projection invalid');
  if (projection.sourceCommit !== sourceCommit || projection.binarySHA256 !== binarySHA256) fail('native lifecycle projection identity invalid');
  const summary = {
    schemaVersion: 1,
    kind: 'tui23-windows-native-lifecycle',
    status: 'passed',
    scope: 'issue-23-windows-five-action-acceptance',
    sourceCommit,
    binarySHA256,
    coreCommit,
    transport: 'ConPTY',
    actions: receipt.actions,
    operationCount: audit.operations.length,
    operations: audit.operations,
    coreAudit: audit.coreAudit,
    terminals: receipt.terminals,
    reconnect: receipt.reconnect,
    unrelatedService: {id: receipt.unrelatedService?.id, unchangedAfterFiveActions: receipt.unrelatedService?.unchangedAfterFiveActions},
    resize: receipt.keyboard?.nativeResizeRendered === true,
    previewFrozen: receipt.previewFrozen === true,
    reloadDenied: receipt.reloadDenied === true,
    ownedRuntimeClosed: projection.ownedRuntimeClosed,
  };
  assertSummary(summary);
  await writeFile(path.join(output, 'windows-native-lifecycle-summary.json'), JSON.stringify(summary, null, 2) + '\n', {mode: 0o600});
  process.stdout.write(JSON.stringify({status: 'passed', actions: summary.actions, operations: summary.operationCount, summary: path.join(output, 'windows-native-lifecycle-summary.json')}) + '\n');
}

if (process.argv[1] === fileURLToPath(import.meta.url)) main().catch(error => { process.stderr.write('windows-native-lifecycle: ' + String(error?.message ?? error) + '\n'); process.exitCode = 1; });
