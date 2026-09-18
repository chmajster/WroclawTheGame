#!/usr/bin/env python3
"""Validate WroclawTheGame asset-production manifests without external dependencies."""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ID_RE = re.compile(r"^[a-z0-9][a-z0-9_\-]{2,63}$")
MODEL_EXTENSIONS = {".blend", ".fbx", ".obj", ".glb", ".gltf"}
TEXTURE_EXTENSIONS = {".png", ".jpg", ".jpeg", ".tga", ".exr", ".tif", ".tiff"}


class ManifestError(ValueError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ManifestError(message)


def project_path(value: str) -> Path:
    path = Path(value)
    if path.is_absolute():
        return path
    return (ROOT / path).resolve()


def validate_manifest(data: dict, check_files: bool = True) -> None:
    required = {
        "schema_version", "id", "display_name", "area", "type", "quality",
        "source", "mesh", "lod", "collision", "textures", "unreal", "qa",
    }
    require(isinstance(data, dict), "manifest root must be an object")
    missing = sorted(required - set(data))
    require(not missing, f"missing required fields: {', '.join(missing)}")
    require(data["schema_version"] == 1, "schema_version must be 1")
    require(isinstance(data["id"], str) and ID_RE.fullmatch(data["id"]) is not None, "invalid id")
    require(isinstance(data["display_name"], str) and data["display_name"].strip(), "display_name is required")
    require(isinstance(data["area"], str) and data["area"].strip(), "area is required")
    require(data["type"] == "static_mesh", "only static_mesh is currently supported")
    require(data["quality"] in {"background", "standard", "hero"}, "quality must be background, standard or hero")

    source = data["source"]
    require(isinstance(source, dict) and isinstance(source.get("model"), str) and source["model"], "source.model is required")
    model = project_path(source["model"])
    require(model.suffix.lower() in MODEL_EXTENSIONS, f"unsupported source model extension: {model.suffix}")
    if check_files:
        require(model.is_file(), f"missing source model: {model}")

    reference_directory = source.get("reference_directory")
    if reference_directory and check_files:
        require(project_path(reference_directory).is_dir(), f"missing reference directory: {reference_directory}")

    dimensions = data.get("dimensions", {})
    if "height_m" in dimensions:
        require(isinstance(dimensions["height_m"], (int, float)) and dimensions["height_m"] > 0, "dimensions.height_m must be > 0")

    mesh = data["mesh"]
    require(isinstance(mesh, dict), "mesh must be an object")
    require(isinstance(mesh.get("max_triangles"), int) and mesh["max_triangles"] > 0, "mesh.max_triangles must be > 0")
    require(mesh.get("combine") is True, "pipeline v1 requires mesh.combine=true")
    require(isinstance(mesh.get("generate_uv_if_missing"), bool), "mesh.generate_uv_if_missing must be boolean")

    lod = data["lod"]
    ratios = lod.get("ratios") if isinstance(lod, dict) else None
    require(isinstance(ratios, list) and 1 <= len(ratios) <= 6, "lod.ratios must contain 1..6 entries")
    require(all(isinstance(x, (int, float)) and 0 < x <= 1 for x in ratios), "all LOD ratios must be in (0, 1]")
    require(abs(float(ratios[0]) - 1.0) < 1e-9, "LOD0 ratio must be 1.0")
    require(all(float(ratios[i]) > float(ratios[i + 1]) for i in range(len(ratios) - 1)), "LOD ratios must strictly decrease")

    collision = data["collision"]
    require(isinstance(collision, dict) and collision.get("mode") in {"none", "simple"}, "collision.mode must be none or simple")
    if "max_hulls" in collision:
        require(isinstance(collision["max_hulls"], int) and 1 <= collision["max_hulls"] <= 32, "collision.max_hulls must be 1..32")

    textures = data["textures"]
    require(isinstance(textures, dict), "textures must be an object")
    for key in ("base_color", "normal", "orm"):
        value = textures.get(key)
        if value is None:
            continue
        require(isinstance(value, str) and value, f"textures.{key} must be a path or null")
        path = project_path(value)
        require(path.suffix.lower() in TEXTURE_EXTENSIONS, f"unsupported texture extension for {key}: {path.suffix}")
        if check_files:
            require(path.is_file(), f"missing texture {key}: {path}")

    unreal = data["unreal"]
    require(isinstance(unreal, dict), "unreal must be an object")
    require(isinstance(unreal.get("destination"), str) and unreal["destination"].startswith("/Game/"), "unreal.destination must start with /Game/")
    require(isinstance(unreal.get("nanite"), bool), "unreal.nanite must be boolean")
    require(isinstance(unreal.get("qa_map"), str) and unreal["qa_map"].startswith("/Game/"), "unreal.qa_map must start with /Game/")

    qa = data["qa"]
    require(isinstance(qa, dict), "qa must be an object")
    require(isinstance(qa.get("automation_filter"), str) and qa["automation_filter"], "qa.automation_filter is required")
    require(isinstance(qa.get("cameras"), list) and all(isinstance(x, str) and x for x in qa["cameras"]), "qa.cameras must be a string array")
    require(isinstance(qa.get("screenshot_width"), int) and 640 <= qa["screenshot_width"] <= 7680, "invalid qa.screenshot_width")
    require(isinstance(qa.get("screenshot_height"), int) and 360 <= qa["screenshot_height"] <= 4320, "invalid qa.screenshot_height")
    if data["quality"] == "hero":
        require(bool(qa["cameras"]), "hero assets require at least one QA camera")


def load(path: Path) -> dict:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise ManifestError(f"{path}: invalid JSON: {exc}") from exc


def discover() -> list[Path]:
    asset_root = ROOT / "Pipeline" / "assets"
    return sorted(asset_root.rglob("*.asset.json")) if asset_root.is_dir() else []


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("manifests", nargs="*", help="manifest paths relative to repository root")
    parser.add_argument("--all", action="store_true", help="validate Pipeline/assets/**/*.asset.json")
    parser.add_argument("--schema-only", action="store_true", help="validate structure without checking referenced files")
    args = parser.parse_args()

    paths = [project_path(item) for item in args.manifests]
    if args.all:
        paths.extend(discover())
    unique = []
    seen = set()
    for path in paths:
        resolved = path.resolve()
        if resolved not in seen:
            unique.append(resolved)
            seen.add(resolved)
    if not unique:\n        if args.all:\n            print("PASS no production asset manifests found")\n            return 0\n        require(False, "no manifests selected")

    failures = []
    for path in unique:
        try:
            require(path.is_file(), f"manifest not found: {path}")
            validate_manifest(load(path), check_files=not args.schema_only)
            print(f"PASS {path.relative_to(ROOT) if path.is_relative_to(ROOT) else path}")
        except (ManifestError, OSError) as exc:
            failures.append(str(exc))
            print(f"FAIL {path}: {exc}", file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ManifestError as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        raise SystemExit(1)
