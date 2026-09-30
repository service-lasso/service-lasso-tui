import { execFileSync } from "node:child_process";
import path from "node:path";

function output(program, args) {
  return execFileSync(program, args, { cwd: process.cwd(), encoding: "utf8", stdio: ["ignore", "pipe", "pipe"] }).trim();
}

function fail(reason) {
  throw new Error(reason);
}

const root = path.resolve(process.cwd());
if (output("git", ["status", "--porcelain=v1", "--untracked-files=all"]) !== "") {
  fail("source checkout must be clean before source test or build");
}
if ((process.env.GOFLAGS ?? "") !== "") fail("ambient GOFLAGS are not admitted");
if (process.env.GOWORK !== "off") fail("GOWORK must be explicitly off");
if (output("go", ["env", "GOFLAGS"]) !== "") fail("effective GOFLAGS are not empty");
if (output("go", ["env", "GOWORK"]) !== "off") fail("effective GOWORK is not off");
if (path.resolve(output("go", ["env", "GOMOD"])) !== path.join(root, "go.mod")) fail("effective Go module path is unexpected");
if (output("go", ["list", "-mod=readonly", "-m"]) !== "github.com/service-lasso/service-lasso-tui") fail("effective Go module identity is unexpected");

process.stdout.write(`${JSON.stringify({ module: "github.com/service-lasso/service-lasso-tui", goflags: "", gowork: "off" })}\n`);
