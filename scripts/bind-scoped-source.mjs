import { writeFile } from "node:fs/promises";
import { execFileSync } from "node:child_process";
import { fileURLToPath } from "node:url";
import { assertSourcePolicy, POLICY_SHA256 } from "./scoped-policy.mjs";
import { candidateIdentity } from "./verify-scoped-candidate-publication.mjs";
import { currentAttempt } from "./scoped-actions.mjs";
const git=args=>execFileSync("git",args,{encoding:"utf8",stdio:["ignore","pipe","ignore"]}).trim();
export async function bindScopedSource(version){await assertSourcePolicy();if(process.env.GITHUB_WORKFLOW_REF!=="service-lasso/service-lasso-tui/.github/workflows/release-scoped.yml@refs/heads/develop"||process.env.GITHUB_WORKFLOW_SHA!==process.env.GITHUB_SHA)throw new Error("source-selected workflow identity denied");const identity=candidateIdentity({sourceRef:process.env.GITHUB_REF,sourceCommit:process.env.GITHUB_SHA,version,tag:`candidate-${version}`});if(git(["rev-parse","HEAD"])!==identity.sourceCommit||git(["status","--porcelain=v1","--untracked-files=all"])!=="")throw new Error("actual clean checkout denied");return identity;}
if(process.argv[1]===fileURLToPath(import.meta.url)) {
  const identity=await bindScopedSource(process.env.CANDIDATE_VERSION);
  if(process.argv[2]==="select-native"){
    const proof=await currentAttempt({source:{repository:"service-lasso/service-lasso-tui",commit:identity.sourceCommit,ref:identity.sourceRef},run:{id:Number(process.env.GITHUB_RUN_ID),attempt:Number(process.env.GITHUB_RUN_ATTEMPT),workflowSha:identity.sourceCommit},token:process.env.GITHUB_TOKEN});
    await writeFile(process.env.GITHUB_OUTPUT,`win32_id=${proof.artifacts.find(item=>item.name===`scoped-native-win32-${process.env.GITHUB_RUN_ID}-${process.env.GITHUB_RUN_ATTEMPT}`).id}\nlinux_id=${proof.artifacts.find(item=>item.name===`scoped-native-linux-${process.env.GITHUB_RUN_ID}-${process.env.GITHUB_RUN_ATTEMPT}`).id}\n`,{flag:"a"});
  }else await writeFile(process.env.GITHUB_OUTPUT,`sha=${identity.sourceCommit}\nversion=${identity.version}\ntag=${identity.tag}\nscope_policy_sha256=${POLICY_SHA256}\n`,{flag:"a"});
}
