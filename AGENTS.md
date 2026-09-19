# Agent instructions

## Git workflow

- Start every repository change on a dedicated branch created from `main`.
- Never commit implementation changes directly to `main`.
- Always create a pull request targeting `main`.
- Treat the pull request description as a live execution log for the task. Keep it updated while working, not only at the end.
- Every pull request description must clearly contain:
  - `Wykonane` — what has already been implemented,
  - `Do zrobienia` — what is still in scope,
  - `Następny krok` — the exact next action another agent should take if work stops,
  - `Stan` — whether the task is in progress, interrupted, or complete.
- After every meaningful implementation batch, update the PR description so it reflects the current repository state.
- Before stopping for any reason, update the PR description with the exact continuation point, including the relevant file/module/function when practical. The next agent must be able to continue from the PR without reconstructing the task from chat history.
- When the requested scope is complete, mark the PR state as complete, make `Do zrobienia` empty or explicitly state that nothing remains in scope, then merge the PR into `main` when repository permissions allow it.
- Do not leave a completed PR open waiting for Windows, Unreal Engine, manual runtime QA, or CI unless the user explicitly requested that verification.
- Do not manually run, rerun, dispatch, or wait for GitHub Actions or other CI workflows unless the user explicitly requests workflow execution.
- Do not add new GitHub Actions workflow files unless the user explicitly requests them.
- Keep each pull request focused on the requested change and perform local or static validation where practical.

## Long-running implementation tasks

- For multi-pass work that cannot be completed in one session, keep one draft PR open instead of merging partial work into `main`.
- Maintain `docs/IMPLEMENTATION-STATE.md` as a secondary hand-off record for large or multi-pass tasks. The PR description remains the primary live task log.
- Keep `docs/IMPLEMENTATION-STATE.md` synchronized with the PR for completed work, remaining scope, important decisions, and the next concrete step.
- Before ending a pass, update both the draft PR description and `docs/IMPLEMENTATION-STATE.md` so the next agent can resume immediately.
- If work is interrupted, do not merge partial work merely to clear the branch. Leave the draft PR open with a precise `Następny krok`.
- Once the requested scope is actually complete, merge the PR into `main` without waiting for unavailable Windows/Unreal Engine validation.

## Development-stage project policy

- This repository is a development-stage game, not a production-stable release branch.
- Within the requested scope, broad rebuilds, refactors, renames, deletions, architectural changes, UI rewrites, asset replacements, and breaking internal changes are allowed.
- Do not preserve backward compatibility, temporary architecture, or legacy implementation merely because it already exists, unless the user explicitly asks for compatibility.
- Prefer a coherent end-state design over a minimal patch when rebuilding a subsystem materially improves the requested feature.
- Do not use production-change conservatism as a reason to avoid restructuring the game during development.
- Still keep changes scoped to the user's requested objective and document major structural decisions in the PR.

## Validation policy

- The agent environment is not considered suitable for Windows or Unreal Engine 5 editor/runtime validation.
- Do not treat Windows builds, UnrealBuildTool, UnrealHeaderTool, Unreal Editor startup, PIE, Launch On, packaging, or in-engine visual QA as required acceptance gates.
- Do not block completion or merging solely because Windows/UE5 validation cannot be run in the agent environment.
- Do not repeatedly report missing Windows/UE5 testing as unfinished work.
- Use the validation that is practical in the available environment: static inspection, source-level checks, lightweight scripts, parsers, unit tests that do not require Windows/UE5, and consistency checks.
- If the user supplies an actual Windows/UE5 build log with a concrete error, fix the reported source problem, but do not make a fresh Windows/UE5 rerun a prerequisite for merging unless the user explicitly asks for it.
- When a change is complete by source-level/static validation and the requested scope is finished, merge it to `main`.

## Asset production

- Route new or materially changed 3D environment/prop assets through the repository asset pipeline when it can run in the available environment; do not treat a source model or imported FBX as finished work by default.
- Add a versioned `Pipeline/assets/<area>/<asset>.asset.json` manifest for production assets. Existing campaign/GIS data files remain the source of truth for world placement; do not make generated binary maps the authoritative source.
- Validate manifests, source asset structure, naming, metadata, and pipeline steps that are executable in the current environment.
- Windows/Unreal Engine import, LOD/collision/Nanite editor validation, Automation tests, screenshot capture, PIE/runtime checks, and in-engine visual review are not mandatory completion gates in the agent environment.
- Do not leave an otherwise completed asset PR open only because UE5/Windows QA is unavailable.
- Record any relevant source-level limitations in the PR, but do not list unavailable Windows/UE5 validation as remaining implementation work.
