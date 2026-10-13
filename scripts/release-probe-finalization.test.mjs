import test from "node:test";
import assert from "node:assert/strict";
import { finalizeProbeOutcome, cleanupResources, closeReceiptSinks, createReceiptSinks, startNativeReceiptWriter, startReconnectProbe, finalizeReconnectExit, prepareSourceBuiltCore, parseProbe, waitForAcknowledgement, waitForFile } from "./verify-release-asset-conpty.mjs";

// Connected source cases only, ALL UNRUN; no native closed/EOF acknowledgement.
test("actual finalizer preserves exact thrown primary including undefined and private cleanup originals", async () => {
  const stop = new Error("PRIVATE stop"), server = new Error("PRIVATE server"), close = new Error("PRIVATE receipt close");
  for (const primary of [new Error("PRIVATE primary"), false, null, 0, undefined]) {
    for (const secondary of [false, true]) {
      const observation = { cleanupOriginals: [] }, calls = [];
      let caught = false, actual;
      try {
        await finalizeProbeOutcome(primary, failures => cleanupResources({probe: {}, apiServer: {stop: async () => {calls.push("server"); if (secondary) throw server;}}, tempRoot: "unused", retainTempRoot: true, failures}, {stop: async () => {calls.push("stop"); if (secondary) throw stop; return true;}}), async () => {calls.push("receipt-close"); if (secondary) throw close;}, {primaryPresent: true, observation});
      } catch (original) { caught = true; actual = original; }
      assert.equal(caught, true); assert.equal(actual, primary);
      assert.deepEqual(observation.cleanupOriginals, secondary ? [stop, server, close] : []);
      assert.deepEqual(calls, ["stop", "server", "receipt-close"]);
    }
  }
  const observation = {cleanupOriginals: []};
  await assert.rejects(finalizeProbeOutcome(undefined, async () => true, async () => {throw close;}, {primaryPresent: false, observation}), original => original === close);
  assert.deepEqual(observation.cleanupOriginals, [close]);
  await assert.rejects(finalizeProbeOutcome(undefined, async () => false, async () => {}, {primaryPresent: false}), /cleanup_unconfirmed/);
  await finalizeProbeOutcome(undefined, async () => true, async () => {}, {primaryPresent: false});
});
test("actual receipt teardown attempts both owned sinks and preserves both originals", async () => {
  const first = new Error("PRIVATE helper close"), second = new Error("PRIVATE node close"), failures = [], calls = [];
  await assert.rejects(closeReceiptSinks({helper: {close() {calls.push("helper"); throw first;}}, node: {async close() {calls.push("node"); throw second;}}}, {failures}), original => original === first);
  assert.deepEqual(calls, ["helper", "node"]);
  assert.deepEqual(failures, [first, second]);
  await closeReceiptSinks({helper: {async close() {}}, node: {async close() {}}});
});
test("actual writer close listener precedes synchronous close during request", async () => {
  const { EventEmitter } = await import("node:events");
  const child = new EventEmitter(), writer = {child, requests: []}, closeEvents = [];
  child.stdin = {write(_bytes, callback) {
    child.emit("close", 0, null);
    writer.requests.shift().resolve({ok: true, event: "closed"});
    callback();
  }};
  await closeReceiptSinks({helper: {writer}}, {closeEvents});
  assert.deepEqual(closeEvents, [{code: 0, signal: null}]);
});

test("actual finalizer keeps repeated and falsy cleanup occurrences", async () => {
  for (const value of [undefined, false, null, 0, -0]) {
    const observation = {cleanupOriginals: []}; let caught = false, actual;
    try { await finalizeProbeOutcome(undefined, async () => {throw value;}, async () => {throw value;}, {primaryPresent: false, observation}); }
    catch (original) {caught = true; actual = original;}
    assert.equal(caught, true); assert.ok(Object.is(actual, value));
    assert.equal(observation.cleanupOriginals.length, 2);
    assert.ok(observation.cleanupOriginals.every(original => Object.is(original, value)));
  }
});
test("actual receipt close joins original already-closed writer future", async () => {
  const event = {code: 0, signal: null}, closeEvents = [];
  const writer = {closedEvent: event, closeObserved: Promise.resolve(event), child: {once() {assert.fail("must join retained birth-time observation");}, stdin: {write() {assert.fail("already closed original must not receive request");}}}};
  await closeReceiptSinks({helper: {writer}}, {closeEvents});
  assert.equal(closeEvents[0], event);
});
test("actual receipt acquisition preserves open primary and secondary close", async () => {
  const primary = new Error("PRIVATE second open"), secondary = new Error("PRIVATE acquired close"), failures = [];
  let opened = 0, closed = 0;
  await assert.rejects(createReceiptSinks("unused", {platform: "linux", failures, createDirectory: async () => "owned", openFile: async () => {
    if (++opened === 2) throw primary;
    return {async close() {closed++; throw secondary;}};
  }}), original => original === primary);
  assert.equal(opened, 2); assert.equal(closed, 1); assert.deepEqual(failures, [secondary]);
});
test("actual receipt close refuses original writer crash and signal", async () => {
  for (const event of [{code: 1, signal: null}, {code: null, signal: "SIGTERM"}]) {
    const original = new Error("PRIVATE observed writer exit"), failures = [], closeEvents = [], handoff = {};
    const writer = {closedEvent: event, closeObserved: Promise.resolve(event), closeFailure: original, child: {once() {assert.fail("retained original future only");}, stdin: {write() {assert.fail("already closed");}}}};
    await assert.rejects(closeReceiptSinks({helper: {writer}}, {failures, closeEvents, handoff}), failure => failure === original);
    assert.equal(failures[0], original); assert.equal(closeEvents[0], event); assert.equal(handoff.recorded, true);
  }
});
test("actual reconnect keeps frozen foreign spawn primary and receipt original privately", async () => {
  const { EventEmitter } = await import("node:events");
  const primary = Object.freeze(new Error("PRIVATE spawn")), secondary = "PRIVATE receipt rejection";
  const child = new EventEmitter(); child.stdout = new EventEmitter(); child.stdout.setEncoding = () => {};
  const probe = startReconnectProbe("inert", "url", "ready", "reconnect", "shutdown", "ack", "token", {node: {async writeFile() {throw secondary;}}, helper: {}}, {sourceCommit: "a".repeat(40), binarySHA256: "b".repeat(64)}, {spawnProcess() {return child;}});
  child.emit("error", primary); child.emit("close", null, null);
  await assert.rejects(probe.completed, original => original === primary);
  assert.deepEqual(probe.observation.primaryFailures, [primary]);
  assert.deepEqual(probe.observation.receiptFailures, [secondary]);
  assert.equal(Object.hasOwn(primary, "receiptFailures"), false);
  assert.equal(Object.getOwnPropertyDescriptor(probe, "observation").enumerable, false);
});

test("actual preflight and parser failures keep actual causes under fixed messages", async () => {
  for (const primary of [new Error("PRIVATE command"), undefined, false]) {
    await assert.rejects(prepareSourceBuiltCore({coreRoot: "unused", tempRoot: "unused", command: async () => {throw primary;}}), failure => failure.reason === "source_identity_unavailable" && Object.hasOwn(failure, "cause") && failure.cause === primary);
  }
  assert.throws(() => parseProbe("{", "reconnect"), failure => failure.cause instanceof SyntaxError && !failure.message.includes("PRIVATE"));
  assert.throws(() => parseProbe("{}", "reconnect"), failure => !Object.hasOwn(failure, "cause"));
  const observation = {receiptFailures: [], parserFailures: [], primaryFailures: []};
  await assert.rejects(finalizeReconnectExit({helper: {}, node: {}}, 1, null, "{", {persist: async () => {}, observation}), /reconnect probe failed/);
  assert.ok(observation.parserFailures[0] instanceof SyntaxError);
});
test("actual wait classifiers preserve read primary before hostile code getter and useful ENOENT retry", async () => {
  const secondary = new Error("PRIVATE code getter"), original = {get code() {throw secondary;}};
  const failures = [], observations = [];
  assert.equal(await waitForAcknowledgement("unused", "token", 5000, failures, observations, async () => {throw original;}), false);
  assert.deepEqual(failures, [original, secondary]); assert.equal(observations[0], original);
  const fileObservations = [];
  await assert.rejects(waitForFile("unused", 20000, fileObservations, async () => {throw original;}), failure => failure === original);
  assert.deepEqual(fileObservations, [original, secondary]);
  const missing = Object.assign(new Error("PRIVATE not yet present"), {code: "ENOENT"}), retryFailures = [], retryObservations = [];
  let reads = 0;
  assert.equal(await waitForAcknowledgement("unused", "token", 5000, retryFailures, retryObservations, async () => {if (++reads === 1) throw missing; return "token\n";}), true);
  assert.equal(reads, 2); assert.deepEqual(retryFailures, []); assert.deepEqual(retryObservations, [missing]);
});

test("actual receipt writer retains parser originals and useful ready result with invocation-owned child", async () => {
  const {EventEmitter} = await import("node:events");
  const child = new EventEmitter(); child.stdout = new EventEmitter(); child.stdout.setEncoding = () => {};
  const writers = [], pending = startNativeReceiptWriter("unused", {spawnProcess: () => child, writers});
  assert.equal(writers.length, 1); assert.equal(writers[0].child, child);
  child.stdout.emit("data", "{\n");
  const original = writers[0].parserFailures[0]; assert.ok(original instanceof SyntaxError);
  child.stdout.emit("data", '{"event":"ready","ok":true,"root":"controlled-root"}\n');
  const writer = await pending;
  assert.equal(writer, writers[0]); assert.equal(writer.root, "controlled-root");
  assert.equal(writer.parserFailures[0], original); assert.deepEqual(writer.startupFailures, []);
});

test("actual receipt writer startup preserves opaque child and stream errors without mutating them", async () => {
  const {EventEmitter} = await import("node:events");
  for (const source of ["child", "stream"]) for (const original of [Object.freeze(new Error("PRIVATE event")), false, null, undefined]) {
    const child = new EventEmitter(); child.stdout = new EventEmitter(); child.stdout.setEncoding = () => {};
    const writers = [], pending = startNativeReceiptWriter("unused", {spawnProcess: () => child, writers});
    const caught = pending.then(() => ({present:false}), failure => ({present:true, failure}));
    (source === "child" ? child : child.stdout).emit("error", original);
    const result = await caught;
    assert.equal(result.present, true); assert.equal(result.failure, original);
    assert.equal(writers[0].child, child); assert.equal(writers[0].startupFailures.length, 1);
    assert.equal(writers[0].startupFailures[0], original);
    const failures = source === "child" ? writers[0].childFailures : writers[0].streamFailures;
    assert.equal(failures.length, 1); assert.equal(failures[0], original);
  }
});

test("actual ready writer stream error refuses cleanup even after a zero close event", async () => {
  const {EventEmitter} = await import("node:events");
  for (const original of [new Error("PRIVATE stream"), false, null, undefined]) {
    const child = new EventEmitter(); child.stdout = new EventEmitter(); child.stdout.setEncoding = () => {};
    const pending = startNativeReceiptWriter("unused", {spawnProcess: () => child});
    child.stdout.emit("data", '{"event":"ready","ok":true,"root":"controlled-root"}\n');
    const writer = await pending;
    child.stdout.emit("error", original); child.stdout.emit("error", original); child.emit("close", 0, null);
    const failures = [], handoff = {}, closeEvents = [];
    const result = await closeReceiptSinks({helper:{writer}}, {failures, handoff, closeEvents}).then(() => ({present:false}), failure => ({present:true, failure}));
    assert.equal(result.present, true); assert.equal(result.failure, original);
    assert.deepEqual(failures, [original, original]); assert.equal(handoff.recorded, true);
    assert.equal(closeEvents[0], writer.closedEvent);
    assert.equal(writer.eventFailures[0].source, "stdout"); assert.equal(writer.eventFailures[0].original, original);
    assert.equal(writer.eventFailures.length, 2);
    const fresh = [], secondHandoff = {};
    const repeated = await closeReceiptSinks({helper:{writer}}, {failures:fresh, handoff:secondHandoff}).then(() => ({present:false}), failure => ({present:true, failure}));
    assert.equal(repeated.present, true); assert.equal(repeated.failure, original);
    assert.deepEqual(fresh, [original, original]); assert.equal(secondHandoff.recorded, true);
    const sameGraph = await closeReceiptSinks({helper:{writer}}, {failures:fresh}).then(() => ({present:false}), failure => ({present:true, failure}));
    assert.equal(sameGraph.present, true); assert.equal(sameGraph.failure, original);
    assert.deepEqual(fresh, [original, original]);
  }
});

test("actual useful writer reentrant teardown shares one close request and retained terminal event", async () => {
  const {EventEmitter} = await import("node:events");
  const child = new EventEmitter(); child.stdout = new EventEmitter(); child.stdout.setEncoding = () => {};
  let writes = 0, observeWrite;
  const writeEntered = new Promise(resolve => {observeWrite = resolve;});
  child.stdin = {write(_message, callback) {writes++; callback(); observeWrite();}};
  const pending = startNativeReceiptWriter("unused", {spawnProcess: () => child});
  child.stdout.emit("data", '{"event":"ready","ok":true,"root":"controlled-root"}\n');
  const writer = await pending, firstEvents = [], secondEvents = [];
  const first = closeReceiptSinks({helper:{writer}}, {closeEvents:firstEvents}), second = closeReceiptSinks({helper:{writer}}, {closeEvents:secondEvents});
  await writeEntered;
  assert.equal(writes, 1);
  child.stdout.emit("data", '{"event":"closed","ok":true}\n'); child.emit("close", 0, null);
  await Promise.all([first, second]);
  assert.equal(firstEvents[0], writer.closedEvent); assert.equal(secondEvents[0], writer.closedEvent);
  await closeReceiptSinks({helper:{writer}}, {closeEvents:firstEvents}); assert.equal(writes, 1);
  assert.equal(firstEvents.length, 1);
});

test("actual writer publishes close admission before synchronous stdin reentry", async () => {
  const {EventEmitter} = await import("node:events");
  const child = new EventEmitter(); child.stdout = new EventEmitter(); child.stdout.setEncoding = () => {};
  let writes = 0, nested, writer, observeWrite;
  const writeEntered = new Promise(resolve => {observeWrite = resolve;});
  child.stdin = {write(_message, callback) {
    writes++;
    nested = closeReceiptSinks({helper:{writer}});
    callback();
    observeWrite();
  }};
  const pending = startNativeReceiptWriter("unused", {spawnProcess: () => child});
  child.stdout.emit("data", '{"event":"ready","ok":true,"root":"controlled-root"}\n');
  writer = await pending;
  const first = closeReceiptSinks({helper:{writer}});
  await writeEntered;
  assert.equal(writes, 1); assert.ok(nested instanceof Promise);
  assert.ok(writer.originalCloseRequest instanceof Promise); assert.notEqual(writer.closeRequest, writer.originalCloseRequest);
  child.stdout.emit("data", '{"event":"closed","ok":true}\n'); child.emit("close", 0, null);
  await Promise.all([first, nested]);
  assert.equal(writes, 1);
});

test("actual writer request failure remains the same terminal result across close callers", async () => {
  const {EventEmitter} = await import("node:events");
  for (const original of [new Error("PRIVATE request"), false, null, undefined]) {
    const child = new EventEmitter(); child.stdout = new EventEmitter(); child.stdout.setEncoding = () => {};
    let writes = 0, observeWrite;
    const writeEntered = new Promise(resolve => {observeWrite = resolve;});
    child.stdin = {write() {writes++; observeWrite(); throw original;}};
    const pending = startNativeReceiptWriter("unused", {spawnProcess: () => child});
    child.stdout.emit("data", '{"event":"ready","ok":true,"root":"controlled-root"}\n');
    const writer = await pending, firstFailures = [];
    const first = closeReceiptSinks({helper:{writer}}, {failures:firstFailures}).then(() => ({present:false}), failure => ({present:true, failure}));
    await writeEntered; child.emit("close", 0, null);
    const firstResult = await first, terminal = writer.closeResult, request = writer.originalCloseRequest;
    const secondFailures = [];
    const secondResult = await closeReceiptSinks({helper:{writer}}, {failures:secondFailures}).then(() => ({present:false}), failure => ({present:true, failure}));
    assert.equal(firstResult.present, true); assert.equal(firstResult.failure, original);
    assert.equal(secondResult.present, true); assert.equal(secondResult.failure, original);
    assert.deepEqual(firstFailures, [original]); assert.deepEqual(secondFailures, [original]);
    assert.equal(writer.closeResult, terminal); assert.equal(writer.originalCloseRequest, request); assert.equal(writes, 1);
    assert.equal(writer.eventFailures.length, 1); assert.equal(writer.eventFailures[0].source, "close-request");
  }
});

test("actual writer failed birth close is one retained occurrence across repeated callers", async () => {
  const {EventEmitter} = await import("node:events");
  const child = new EventEmitter(); child.stdout = new EventEmitter(); child.stdout.setEncoding = () => {};
  const pending = startNativeReceiptWriter("unused", {spawnProcess: () => child});
  child.stdout.emit("data", '{"event":"ready","ok":true,"root":"controlled-root"}\n');
  const writer = await pending;
  child.emit("close", 17, null);
  const original = writer.closeFailure;
  for (let caller = 0; caller < 2; caller++) {
    const failures = [];
    await assert.rejects(closeReceiptSinks({helper:{writer}}, {failures}), failure => failure === original);
    assert.deepEqual(failures, [original]);
    assert.equal(writer.eventFailures.length, 1); assert.equal(writer.eventFailures[0].source, "close");
    assert.equal(writer.closeFailureObserved, true);
  }
});

test("actual reconnect owns repeated child and both pipe failures through physical close", async () => {
  const {EventEmitter} = await import("node:events");
  for (const source of ["child", "stdout", "stderr"]) {
    const child = new EventEmitter(); child.stdout = new EventEmitter(); child.stderr = new EventEmitter(); child.stdout.setEncoding = () => {};
    const observation = {receiptFailures: [], parserFailures: [], primaryFailures: []};
    const probe = startReconnectProbe("inert", "url", "ready", "reconnect", "shutdown", "ack", "token", {node:{async writeFile() {},async sync() {}},helper:{}}, {sourceCommit:"a".repeat(40),binarySHA256:"b".repeat(64)}, {spawnProcess:()=>child,observation});
    const completed = probe.completed.then(() => ({present:false}), original => ({present:true,original}));
    const first = Object.freeze(new Error("PRIVATE first"));
    const originals = [first,first,undefined,null,false,0,""];
    for (const original of originals) (source === "child" ? child : child[source]).emit("error",original);
    child.emit("close",0,null);
    const result = await completed; await probe.exited;
    assert.equal(result.present,true); assert.equal(result.original,first);
    assert.deepEqual(observation.primaryFailures,originals);
    assert.equal(probe.completedSuccessfully,false);
  }
});

test("actual ready writer owns repeated stdin failures despite zero close", async () => {
  const {EventEmitter} = await import("node:events");
  const child = new EventEmitter(); child.stdin = new EventEmitter(); child.stdout = new EventEmitter(); child.stdout.setEncoding = () => {};
  const pending = startNativeReceiptWriter("unused",{spawnProcess:()=>child});
  child.stdout.emit("data",'{"event":"ready","ok":true,"root":"controlled-root"}\n');
  const writer = await pending, first = Object.freeze(new Error("PRIVATE stdin"));
  const originals = [first,first,undefined,null,false,0,""];
  for (const original of originals) child.stdin.emit("error",original);
  child.emit("close",0,null);
  const failures = [];
  await assert.rejects(closeReceiptSinks({helper:{writer}},{failures}),original=>original===first);
  assert.deepEqual(failures,originals);
  assert.ok(writer.eventFailures.every(event=>event.source==="stdin"));
});
