# Agent instructions

## Git workflow

- Start every repository change on a dedicated branch created from `main`.
- Never commit implementation changes directly to `main`.
- Always create a pull request targeting `main`.
- After creating the pull request, merge it into `main` before considering the requested repository task complete when repository permissions allow it.
- Do not manually run, rerun, dispatch, or wait for GitHub Actions or other CI workflows unless the user explicitly requests workflow execution.
- Do not add new GitHub Actions workflow files unless the user explicitly requests them.
- Keep each pull request focused on the requested change and perform local or static validation where practical.
