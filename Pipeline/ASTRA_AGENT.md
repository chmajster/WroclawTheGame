# Astra asset-production runbook

Use this contract when an autonomous agent works on 3D content in WroclawTheGame.

## Goal

Do not stop at generating a mesh. Complete the evidence loop:

real/reference material -> source model -> Blender build -> Unreal import -> gameplay automation -> screenshots -> visual comparison -> correction -> final QA -> PR -> merge.

## Execution contract

1. Read `AGENTS.md`, `Pipeline/README.md` and the target `*.asset.json`.
2. Preserve real Wrocław references as the visual source of truth. Do not invent a major facade/detail when the reference is available.
3. Modify source assets and versioned manifests, not files under `Saved/`.
4. Run `Pipeline/Invoke-WTGAssetPipeline.ps1`.
5. If the pipeline stops at `visual_review`, inspect every image in `Saved/Pipeline/screenshots/<asset>/` against the supplied references.
6. Record the result with `Pipeline/qa/record_visual_review.py`.
7. On FAIL, fix the source model/material/placement input and rerun from the earliest invalidated stage. A new screenshot capture invalidates the previous visual review automatically.
8. Continue until `Saved/Pipeline/status/<asset>.json` is `done/PASS` and `Saved/Pipeline/reports/<asset>.json` is `PASS`.
9. Only then publish. The publisher must refuse incomplete work.

## Visual review

Check at least:
- silhouette and real-world proportions,
- scale against neighboring architecture/props,
- facade openings and architectural details,
- material identity, roughness, metallic response and normal intensity,
- texture stretching and UV artifacts,
- visible LOD popping,
- collision/gameplay obstruction,
- floating/intersecting geometry,
- lighting-dependent artifacts,
- obvious mismatch against the reference viewpoint.

The review notes must describe observed evidence. Do not write `PASS` merely because the asset imported or the game launched.

## Recovery

The authoritative recovery record is `Saved/Pipeline/status/<asset>.json`.

- `FAIL`: fix the failed stage or its input and rerun with `-Resume` or `-From <stage>`.
- `WAITING_FOR_VISUAL_REVIEW`: inspect screenshots and record PASS/FAIL; do not rebuild unless the images reveal a defect.
- `done/PASS`: the asset is eligible for publication.
- Missing report/marker: treat the stage as incomplete.

Do not infer success from source-code compilation, an FBX file existing, Unreal Editor opening, or a command returning before its required marker/report exists.
