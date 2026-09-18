#!/usr/bin/env python3
"""Record Astra/human visual comparison after Unreal screenshots are available."""
from __future__ import annotations

import argparse
import json
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def project_path(value: str) -> Path:
    path = Path(value)
    return path if path.is_absolute() else (ROOT / path).resolve()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("manifest")
    parser.add_argument("--status", required=True, choices=("PASS", "FAIL"))
    parser.add_argument("--notes", required=True)
    parser.add_argument("--reviewer", default="astra")
    args = parser.parse_args()

    manifest = json.loads(project_path(args.manifest).read_text(encoding="utf-8"))
    asset_id = manifest["id"]
    capture_path = ROOT / "Saved" / "Pipeline" / "screenshots" / asset_id / "capture.json"
    if not capture_path.is_file():
        raise SystemExit(f"Missing screenshot QA report: {capture_path}")
    capture = json.loads(capture_path.read_text(encoding="utf-8"))
    if capture.get("status") not in {"PASS", "SKIPPED"}:
        raise SystemExit(f"Screenshot QA is not complete: {capture.get('status')}")

    if manifest["qa"]["cameras"] and capture.get("status") != "PASS":
        raise SystemExit("Configured QA cameras require successful screenshot capture")

    output = ROOT / "Saved" / "Pipeline" / "visual"
    output.mkdir(parents=True, exist_ok=True)
    record = {
        "asset_id": asset_id,
        "status": args.status,
        "reviewer": args.reviewer,
        "notes": args.notes.strip(),
        "screenshots": capture.get("screenshots", []),
        "recorded_at": datetime.now(timezone.utc).isoformat(),
    }
    (output / f"{asset_id}.json").write_text(json.dumps(record, indent=2), encoding="utf-8")
    print(json.dumps(record, indent=2))
    return 0 if args.status == "PASS" else 2


if __name__ == "__main__":
    raise SystemExit(main())
