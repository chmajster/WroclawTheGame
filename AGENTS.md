# Agent instructions

## Git workflow

- Start every repository change on a dedicated branch created from `main`.
- Never commit implementation changes directly to `main`.
- Always create a pull request targeting `main`.
- After creating the pull request, merge it into `main` before considering the requested repository task complete when repository permissions allow it.
- Do not manually run, rerun, dispatch, or wait for GitHub Actions or other CI workflows unless the user explicitly requests workflow execution.
- Do not add new GitHub Actions workflow files unless the user explicitly requests them.
- Keep each pull request focused on the requested change and perform local or static validation where practical.

## Long-running implementation tasks

- For multi-pass work that cannot be completed in one session, keep one draft PR open instead of merging partial work into `main`.
- Maintain `docs/IMPLEMENTATION-STATE.md` as the source of truth for completed work, verification gaps, and the next concrete step.
- Before ending a pass, update both that file and the draft PR description so the next agent can resume without reconstructing state from chat history.
- Merge the PR only when the requested multi-pass scope reaches its acceptance gates or the user explicitly asks to merge the current partial state.

## Asset production

- Route new or materially changed 3D environment/prop assets through `Pipeline/Invoke-WTGAssetPipeline.ps1`; do not treat a source model or imported FBX as finished work.
- Add a versioned `Pipeline/assets/<area>/<asset>.asset.json` manifest for production assets. Existing campaign/GIS data files remain the source of truth for world placement; do not make generated binary maps the authoritative source.
- A production asset is complete only after manifest validation, Blender export, Unreal import/LOD/collision/Nanite validation, Automation tests, required screenshot capture, required visual review, and the final `Saved/Pipeline/reports/<asset>.json` status is `PASS`.
- If visual review is required, compare the generated screenshots against the real/reference material, record explicit PASS/FAIL with `Pipeline/qa/record_visual_review.py`, and rebuild/retest after a failure.
- Use `Pipeline/git/Publish-Asset.ps1` only after the final QA report and pipeline state are PASS. Never bypass the QA gates merely to create or merge a pull request.
