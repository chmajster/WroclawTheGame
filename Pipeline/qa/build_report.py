#!/usr/bin/env python3
"""Build the final machine-readable and Markdown QA report for one asset."""
from __future__ import annotations

import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def project_path(value: str) -> Path:
    path = Path(value)
    return path if path.is_absolute() else (ROOT / path).resolve()


def read_json(path: Path, required: bool = True):
    if not path.is_file():
        if required:
            raise SystemExit(f"Missing required report: {path}")
        return None
    return json.loads(path.read_text(encoding="utf-8"))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("manifest")
    args = parser.parse_args()

    manifest_path = project_path(args.manifest)
    manifest = read_json(manifest_path)
    asset_id = manifest["id"]

    blender = read_json(ROOT / "Saved" / "Pipeline" / "generated" / asset_id / "blender-report.json")
    unreal_import = read_json(ROOT / "Saved" / "Pipeline" / "import" / f"{asset_id}.json")
    automation = read_json(ROOT / "Saved" / "Pipeline" / "automation" / f"{asset_id}.json")
    capture = read_json(ROOT / "Saved" / "Pipeline" / "screenshots" / asset_id / "capture.json")
    visual = read_json(ROOT / "Saved" / "Pipeline" / "visual" / f"{asset_id}.json", required=bool(manifest["qa"]["cameras"]))

    checks = {
        "manifest": "PASS",
        "blender": blender.get("status"),
        "unreal_import": unreal_import.get("status"),
        "automation": automation.get("status"),
        "screenshots": capture.get("status"),
        "visual_review": visual.get("status") if visual else "SKIPPED",
    }
    acceptable = {
        "manifest": {"PASS"},
        "blender": {"PASS"},
        "unreal_import": {"PASS"},
        "automation": {"PASS"},
        "screenshots": {"PASS", "SKIPPED"},
        "visual_review": {"PASS", "SKIPPED"},
    }
    final_status = "PASS" if all(value in acceptable[key] for key, value in checks.items()) else "FAIL"

    report = {
        "asset_id": asset_id,
        "display_name": manifest["display_name"],
        "area": manifest["area"],
        "status": final_status,
        "checks": checks,
        "mesh": unreal_import.get("mesh"),
        "lods": unreal_import.get("lods", []),
        "nanite": unreal_import.get("nanite"),
        "triangles": blender.get("lods", []),
        "screenshots": capture.get("screenshots", []),
        "visual_notes": visual.get("notes") if visual else None,
    }

    output = ROOT / "Saved" / "Pipeline" / "reports"
    output.mkdir(parents=True, exist_ok=True)
    json_path = output / f"{asset_id}.json"
    md_path = output / f"{asset_id}.md"
    json_path.write_text(json.dumps(report, indent=2), encoding="utf-8")

    lines = [
        f"# Asset QA — {manifest['display_name']}",
        "",
        f"- ID: `{asset_id}`",
        f"- Area: {manifest['area']}",
        f"- Status: **{final_status}**",
        f"- Unreal mesh: `{unreal_import.get('mesh')}`",
        f"- Nanite: {unreal_import.get('nanite')}",
        "",
        "## Gates",
        "",
    ]
    lines.extend(f"- {name}: **{value}**" for name, value in checks.items())
    lines += ["", "## LOD", ""]
    for item in blender.get("lods", []):
        lines.append(f"- LOD{item['lod']}: {item['triangles']} triangles ({item['ratio']:.3f})")
    lines += ["", "## Screenshots", ""]
    screenshots = capture.get("screenshots", [])
    lines.extend(f"- `{path}`" for path in screenshots)
    if not screenshots:
        lines.append("- none")
    lines += ["", "## Visual review", "", visual.get("notes", "Not required.") if visual else "Not required.", ""]
    md_path.write_text("\n".join(lines), encoding="utf-8")

    print(json.dumps(report, indent=2))
    return 0 if final_status == "PASS" else 2


if __name__ == "__main__":
    raise SystemExit(main())
