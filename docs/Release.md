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
| Variable | `RELEASE_NOTES_BASE_URL` | Publicly reachable private HTTPS API base URL; required |
| Secret | `RELEASE_NOTES_API_KEY` | Private API credential; required |

Codex requires a **Responses-compatible** service: the script appends `/responses`
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

The prompt contains commit subjects/bodies, file statistics, filtered source diff,
and the latest three other official release bodies as style examples. Input is
capped at 200 KiB of UTF-8: commits receive up to 72 KiB, statistics 24 KiB,
historical examples 24 KiB, and diffs use the remaining space. Truncation is marked.
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
tokens. Claude disables tools, MCP, settings sources, skills and session storage.
Codex uses a local model catalog, read-only sandbox, disabled shell/patch/search
features, and no repository instructions. Because Codex still exposes built-in
utility tools, a loopback-only adapter removes all upstream tools, forces
`tool_choice: none`, and buffers/validates Responses SSE before delivering it to
the CLI. Tool-call responses are rejected before execution. Only the adapter
receives the real API key; Codex receives an ephemeral loopback credential.
The loopback hop is HTTP on the same runner; the private upstream connection is
HTTPS. Response buffering is capped at 2 MiB. No external proxy is deployed.

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
credential isolation, retries, tool-call rejection, assets, tag movement, upload
failure/retry and refusal to overwrite a public release. They do not snapshot
workflow, prompt or documentation text.

For opt-in **real CLI / fake API** smoke tests on Linux, install the two pinned
packages above and Node 22 on `PATH`, then run:

```sh
RELEASE_CLI_SMOKE=1 python3 -B -m unittest discover -s scripts/tests -p 'test_release*.py'
```

These tests use loopback HTTP, synthetic responses and a fake key, deliberately
calling the adapter beneath the production HTTPS validation. They do not create
a GitHub release or call a paid API. Tests are not a substitute for hosted-runner
acceptance: in a controlled test repository, adjust repository guards in **all four workflows**,
configure the Environment, push a disposable `v*` tag and verify both providers,
failure gates, draft retry, complete assets and bilingual notes. Revert test-only
guard changes before deploying. Production integration requires administrator-
provided credentials and must not be claimed from local smoke tests alone.
