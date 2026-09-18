#!/usr/bin/env python3
"""Aggregate Unreal-side runtime asset QA gates into one final PASS/FAIL report."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
QA = ROOT / "Saved/RuntimeAssetQA"
MARKER = ROOT / "Saved/RuntimeAssetQAPass.ok"
OUT = QA / "final.json"


def load(name: str) -> dict:
    path = QA / name
    if not path.is_file():
        raise RuntimeError(f"QA report missing: {path}")
    return json.loads(path.read_text(encoding="utf-8"))


def digest_payload(value) -> str:
    return hashlib.sha256(
        json.dumps(value, sort_keys=True, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
    ).hexdigest()


def main() -> int:
    MARKER.unlink(missing_ok=True)
    reports = {
        "model_quality": load("model_quality.json"),
        "retarget": load("retarget.json"),
        "scene": load("scene.json"),
        "capture": load("character_screenshots/capture.json"),
        "visual_review": load("visual_review.json"),
    }
    errors = []
    for name in ("model_quality", "retarget", "scene", "capture", "visual_review"):
        if reports[name].get("status") != "PASS":
            errors.append(f"{name}: status={reports[name].get('status')}")

    capture_fp = reports["capture"].get("input_fingerprint")
    review_fp = reports["visual_review"].get("capture_fingerprint")
    if not capture_fp or review_fp != capture_fp:
        errors.append("visual review is stale relative to current screenshot capture")

    reviewed = {
        (item.get("semantic"), item.get("sha256"))
        for item in reports["visual_review"].get("screenshots", [])
    }
    captured = {
        (item.get("semantic"), item.get("sha256"))
        for item in reports["capture"].get("screenshots", [])
    }
    if reviewed != captured:
        errors.append("visual review screenshot set does not match capture set")

    result = {
        "status": "PASS" if not errors else "FAIL",
        "errors": errors,
        "gates": {
            name: {
                "status": value.get("status"),
                "sha256": digest_payload(value),
            }
            for name, value in reports.items()
        },
        "capture_fingerprint": capture_fp,
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    if errors:
        print("Runtime asset QA FAIL")
        for error in errors:
            print("- " + error)
        return 1
    MARKER.write_text("PASS\n", encoding="utf-8")
    print("Runtime asset QA PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
