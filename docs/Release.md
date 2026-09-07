# Tag releases

`release.yml` is the only workflow triggered by `v*` tag pushes. It calls the three
build workflows in parallel, waits for all three to succeed, generates notes, then
publishes a complete release. Pull requests still run the original build checks
and upload artifacts, but cannot generate notes or publish releases.

## GitHub Environment setup

Create an Environment named `release` in the repository settings. Configure:

| Kind | Name | Value |
| --- | --- | --- |
| Variable | `RELEASE_NOTES_PROVIDER` | `codex` (default) or `claude` |
| Variable | `RELEASE_NOTES_MODEL` | Exact model identifier supported by your endpoint; required |
| Secret | `RELEASE_NOTES_BASE_URL` | Publicly reachable private HTTPS API base URL; required |
| Secret | `RELEASE_NOTES_API_KEY` | Private API credential; required |

Both secrets are injected only into the notes-generation step. If the base URL
was previously configured as an Environment variable, recreate it as an Environment
secret with the same name and remove the old variable; there is no variable fallback.

Codex requires a **Responses-compatible** service: the CLI appends `/responses`
to the base URL (include `/v1` in the base URL if your service requires it), uses
Bearer authentication, and expects Responses SSE events including
`response.completed`. A Chat Completions-only endpoint is not sufficient.

Claude Code requires an **Anthropic-compatible** service and uses
`ANTHROPIC_BASE_URL` / `ANTHROPIC_API_KEY`. Normally configure the service root;
the CLI calls `/v1/messages`. Your gateway must support Anthropic streaming and
the configured model. OpenAI and Anthropic protocols are not translated.

Do not embed credentials in the URL. Query strings and fragments are rejected.
The endpoint must be reachable from GitHub-hosted Ubuntu runners with a publicly
trusted TLS certificate. This workflow does not provision private networking,
allow self-signed certificates, or bypass TLS verification.

Only the notes job uses this Environment. If you configure required reviewers,
approval happens before notes generation, not after reviewing the generated text.
Allow `v*` tags in any Environment deployment restrictions. The publish job runs
automatically after successful notes generation; it has no private API secret.
Enable Actions with read/write token permissions as permitted by repository and
organization policy. No personal access token is required.

## Build and publication lifecycle

- Existing Windows builds and package contents remain unchanged. The former
  `pull_request.tags` filters were invalid for that event; Windows PR checks now
  use an unfiltered `pull_request` trigger. BSP tools retain their path filter.
- The three build jobs upload `release-windows`, `release-windows-blob`, and
  `release-bsp-localization-tools` artifacts, retained for 14 days.
- `release-notes` contains only the generated Markdown, also retained for 14 days.
- The publisher downloads these four named artifacts from its own run. Archive
  downloads remain in separate directories to detect wrong or duplicate files.
- Publication requires exactly the existing five nonempty `.7z` archives. It
  verifies the remote tag still points to the built commit before making changes
  and again immediately before publication, including annotated tag resolution.
- It creates or reuses a draft named `MetaHookSv-<tag>`, uploads all archives,
  verifies every asset name, size and upload state, then makes the draft public.
  Published releases are never automatically modified. Releases are non-prerelease.

Builds receive only read permission and no private API secrets. The AI job has
read-only GitHub access; only the final job receives `contents: write`. Checkout
in the notes/publish jobs does not persist the GitHub token in Git configuration.
Concurrency is scoped to the tag and never cancels an in-progress publication.
Do not manually publish or edit the same draft while its workflow is running;
GitHub does not provide an atomic compare-and-publish transaction against manual edits.

## Notes evidence and isolation

The baseline is the most recently **published** non-draft, non-prerelease release
whose tag is an ancestor of the current commit, excluding the current tag.
Deleted/unresolvable historical tags and releases on unrelated branches are
skipped. Without a baseline, the notes identify the first release and use all
reachable commits plus a diff against the empty tree.

The initial prompt contains commit subjects/bodies, file statistics, filtered source
diff, and the latest three other official release bodies as style examples. It is
capped at 96 KiB of UTF-8: commits receive up to 40 KiB, statistics 12 KiB,
historical examples 12 KiB, and diffs use the remaining space. Truncation is marked.
The AI can then investigate history itself through the local `release_git`
stdio MCP server. Its `git_history` tool executes only these read-only queries:

- `log`: commit subjects/bodies, with optional `base..revision` scope and bounded
  `limit` and `skip` pagination.
- `show`: a commit patch, or a tracked text file at a selected revision and path.
- `diff`: compare an explicit base revision with the selected revision.
- `ls-tree`: list tracked paths at a selected revision.

For example, the AI can request `{"command":"log","limit":10}`, then
`{"command":"show","revision":"HEAD","path":"Plugins/Renderer/gl_rmain.cpp"}`.
The model chooses queries; Python validates their structured arguments and invokes
Git without a shell. Arbitrary options, write subcommands, absolute paths, path
traversal and revisions outside the release commit's ancestry are rejected.
There is no general shell, editor, patch, fetch or push tool. Git subprocesses
receive no API/GitHub credentials; external diff/textconv, hooks, filesystem
monitoring, optional index locks and remote transports are disabled.

Each query has a 30-second timeout and returns at most 16 KiB. At most 32 queries
are accepted per attempt. Initial evidence plus returned Git evidence is capped
at 200 KiB per attempt; CLI protocol overhead is not included. Narrow a query or
paginate when output is truncated. A budget error tells the AI to use evidence
already collected rather than inventing omitted details.
Binary bodies, `thirdparty`, build outputs and common generated/lock files are
excluded from source diffs; filenames may still appear in statistics. Review
whether this source material may be sent to your configured API before enabling it.

The model produces matching `## English` and `## ä¸­æ` sections,
using module labels such as `[Renderer]`.
Historical notes are style-only data, not claims about this tag. Empty, oversized,
non-bilingual, failed or credential-containing results are rejected. Generation
has a 10-minute timeout per attempt and at most two attempts. Failure blocks
publication; there is no automatic fallback to generic notes.

The pinned packages are `@openai/codex@0.114.0` and
`@anthropic-ai/claude-code@2.1.79`. They run outside the checkout in fresh temporary
home/work directories with an allowlisted environment and no GitHub/artifact
tokens. Claude disables built-in tools, settings sources, skills and session
storage; strict MCP configuration exposes only `mcp__release_git__git_history`,
with other permission requests denied noninteractively. Codex uses a local model
catalog, read-only sandbox, disabled shell/patch/search features, no repository
instructions, and the same allowlisted MCP tool. Its event validator rejects
tool activity outside that allowlist. Repository write prevention comes from not
exposing general execution/editing tools and from constructing only validated
read-only Git commands, not from asking the model to obey a prompt. The stdio
server is not an OS sandbox for arbitrary commands and does not allow them.

Both CLIs connect directly to the private HTTPS API and receive the API key only
through their isolated process environment. The former loopback HTTP request
filter is removed; tool calls must now round-trip through your private service.
The endpoint must support Responses function calls (Codex) or Anthropic tool use
(Claude), not just text generation. Repository instructions and tool results are
untrusted source data; they must not change the release task or permission policy.

Raw CLI output, prompts and credentials are not printed or uploaded for debugging.
The notes artifact is retained if the later publisher fails. AI summaries still
require human judgement: structural checks cannot guarantee factual accuracy.

## Failure recovery

1. Fix the failed build, Environment configuration or endpoint availability first.
2. Use GitHub's **Re-run failed jobs** on the original tag run. Successfully
   uploaded artifacts remain usable for 14 days; partial reruns replace artifacts
   of the same name rather than introducing duplicates.
3. If artifacts expired, use **Re-run all jobs**. CLI packages and provider/model
   configuration are selected afresh for a rerun; workflow code remains tied to
   the original run's commit.
4. A failed upload leaves a draft. Rerunning reuses it, replaces only the expected
   asset names and regenerates/updates its notes. Unexpected draft assets stop the
   run without deleting them; review those assets manually.
5. A moved/deleted tag or an already-public release stops publication. Do not
   force-move tags to repair public releases; publish a new tag instead.

## Validation

Run deterministic script tests (Python 3.12, Git; no API credentials needed):

```sh
python -B -m unittest discover -s scripts/tests -p 'test_release*.py'
actionlint .github/workflows/release.yml .github/workflows/windows.yml .github/workflows/windows_blob.yml .github/workflows/bsp-localization-tools.yml
git diff --check
```

The tests cover ancestry, first release, byte limits, provider validation,
credential isolation, retries, read-only Git queries, write/argument injection
rejection, unchanged repository contents (including `.git`), query budgets,
assets, tag movement, upload
failure/retry and refusal to overwrite a public release. They do not snapshot
workflow, prompt or documentation text.

For opt-in **real CLI / fake API** smoke tests on Linux, install the two pinned
packages above and Node 22 on `PATH`, then run:

```sh
RELEASE_CLI_SMOKE=1 python3 -B -m unittest discover -s scripts/tests -p 'test_release*.py'
```

These tests use loopback HTTP, synthetic responses and a fake key, deliberately
calling the CLI beneath the production HTTPS validation. They verify that both
real CLIs return Git history/source evidence to the model, reject Git write
requests, and cannot execute a native shell write against the test repository.
They do not create
a GitHub release or call a paid API. Tests are not a substitute for hosted-runner
acceptance: in a controlled test repository, adjust repository guards in **all four workflows**,
configure the Environment, push a disposable `v*` tag and verify both providers,
failure gates, draft retry, complete assets and bilingual notes. Revert test-only
guard changes before deploying. Production integration requires administrator-
provided credentials and must not be claimed from local smoke tests alone.
