#!/usr/bin/env python3
"""Record the required human/Astra visual review for runtime character screenshots."""
from __future__ import annotations

import argparse
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CAPTURE = ROOT / "Saved/RuntimeAssetQA/character_screenshots/capture.json"
OUT = ROOT / "Saved/RuntimeAssetQA/visual_review.json"


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--status", choices=("PASS", "FAIL"), required=True)
    parser.add_argument("--notes", required=True)
    parser.add_argument("--reviewer", default="manual")
    args = parser.parse_args()

    notes = args.notes.strip()
    if not notes:
        raise SystemExit("Visual review notes cannot be empty")
    capture = json.loads(CAPTURE.read_text(encoding="utf-8"))
    if capture.get("status") != "PASS":
        raise SystemExit("Character screenshot capture is not PASS")

    verified = []
    for item in capture.get("screenshots", []):
        path = ROOT / item["path"]
        if not path.is_file():
            raise SystemExit(f"Screenshot missing: {path}")
        actual = sha256_file(path)
        if actual != item.get("sha256"):
            raise SystemExit(f"Screenshot changed after capture: {path}")
        verified.append({"semantic": item["semantic"], "path": item["path"], "sha256": actual})

    if len(verified) != len(capture.get("poses", [])):
        raise SystemExit("Screenshot set is incomplete")

    report = {
        "status": args.status,
        "reviewer": args.reviewer,
        "notes": notes,
        "capture_fingerprint": capture["input_fingerprint"],
        "screenshots": verified,
        "reviewed_at": datetime.now(timezone.utc).isoformat(),
        "criteria": [
            "male/female body scale and orientation are plausible",
            "retargeted pose preserves limbs without visible inversion",
            "hands/feet do not visibly detach or collapse",
            "hair/beard/eyebrows do not visibly clip through the head in sampled poses",
            "stand-up and dodge pose have no severe self-intersection",
            "materials are present and no mesh renders as missing/default checkerboard",
        ],
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"{args.status} runtime asset visual review: {len(verified)} screenshots")
    return 0 if args.status == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
