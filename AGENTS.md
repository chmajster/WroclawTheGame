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
