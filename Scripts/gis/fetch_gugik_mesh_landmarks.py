#!/usr/bin/env python3
"""Download and crop official textured GUGiK 3D-mesh tiles for Wrocław landmarks."""
from __future__ import annotations

import argparse
import hashlib
import html.parser
import json
import math
import re
import shutil
import sys
import urllib.parse
import urllib.request
import zipfile
from pathlib import Path

from pyproj import Transformer

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CONFIG = ROOT / "Data" / "gugik_mesh_landmarks.json"
DEFAULT_OUTPUT = ROOT / "Saved" / "GUGiKMeshLandmarks"
USER_AGENT = "WroclawTheGame/GUGiKMesh (+https://github.com/chmajster/WroclawTheGame)"
IMAGE_EXTENSIONS = {".jpg", ".jpeg", ".png", ".tif", ".tiff"}


class LinkParser(html.parser.HTMLParser):
    def __init__(self):
        super().__init__()
        self.links: list[tuple[str, str]] = []
        self._href: str | None = None
        self._text: list[str] = []

    def handle_starttag(self, tag, attrs):
        if tag.casefold() == "a":
            self._href = dict(attrs).get("href")
            self._text = []

    def handle_data(self, data):
        if self._href is not None:
            self._text.append(data)

    def handle_endtag(self, tag):
        if tag.casefold() == "a" and self._href is not None:
            self.links.append((self._href, " ".join(self._text).strip()))
            self._href = None
            self._text = []


def request_bytes(url: str, timeout: int = 180) -> bytes:
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT, "Accept": "*/*"})
    with urllib.request.urlopen(req, timeout=timeout) as response:
        return response.read()


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def feature_info_urls(service_url: str, easting: float, northing: float, radius: float) -> list[str]:
    params = {
        "SERVICE": "WMS",
        "VERSION": "1.3.0",
        "REQUEST": "GetFeatureInfo",
        "LAYERS": "SkorowidzeModeleSiatkowe3D",
        "QUERY_LAYERS": "SkorowidzeModeleSiatkowe3D",
        "CRS": "EPSG:2177",
        "BBOX": f"{northing-radius},{easting-radius},{northing+radius},{easting+radius}",
        "WIDTH": "101",
        "HEIGHT": "101",
        "I": "50",
        "J": "50",
        "FORMAT": "image/png",
        "INFO_FORMAT": "text/html",
    }
    url = service_url + "?" + urllib.parse.urlencode(params)
    payload = request_bytes(url)
    parser = LinkParser()
    parser.feed(payload.decode("utf-8", "replace"))
    links = []
    for href, text in parser.links:
        absolute = urllib.parse.urljoin(url, href)
        haystack = (absolute + " " + text).casefold()
        if ".zip" in haystack or "pobierz" in haystack or "download" in haystack:
            links.append(absolute)
    # Some feature-info templates print raw URLs without anchors.
    body = payload.decode("utf-8", "replace")
    links.extend(re.findall(r"https?://[^\"'<>\s]+\.zip(?:\?[^\"'<>\s]*)?", body, re.I))
    return sorted(set(links))


def safe_extract(archive: Path, destination: Path) -> list[Path]:
    destination.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(archive) as package:
        for member in package.infolist():
            if member.is_dir():
                continue
            target = (destination / member.filename).resolve()
            if not target.is_relative_to(destination.resolve()):
                raise RuntimeError(f"Unsafe archive member: {member.filename}")
            target.parent.mkdir(parents=True, exist_ok=True)
            with package.open(member) as source, target.open("wb") as output:
                shutil.copyfileobj(source, output)
    return sorted(destination.rglob("*"))


def parse_vertex_ref(token: str) -> int:
    value = token.split("/", 1)[0]
    return int(value)


def crop_obj(source_obj: Path, output_obj: Path, center_e: float, center_n: float, radius: float) -> dict:
    lines = source_obj.read_text(encoding="utf-8", errors="replace").splitlines()
    vertices: list[tuple[float, float, float]] = []
    faces: list[tuple[int, str]] = []
    keep_meta: list[str] = []
    current_material = None
    for line in lines:
        stripped = line.strip()
        if stripped.startswith("v "):
            parts = stripped.split()
            if len(parts) >= 4:
                vertices.append((float(parts[1]), float(parts[2]), float(parts[3])))
        elif stripped.startswith("usemtl "):
            current_material = stripped
        elif stripped.startswith("f "):
            faces.append((len(keep_meta), (current_material + "\n" if current_material else "") + stripped))
        elif stripped.startswith(("mtllib ", "vt ", "vn ", "s ", "o ", "g ")):
            keep_meta.append(stripped)

    if not vertices or not faces:
        raise RuntimeError(f"OBJ has no usable geometry: {source_obj}")

    nearest = min(math.hypot(x-center_e, y-center_n) for x, y, _ in vertices)
    if nearest > radius * 4:
        raise RuntimeError(
            f"OBJ vertices are not in expected EPSG:2177 absolute coordinates; nearest vertex is {nearest:.1f} m away"
        )

    selected = []
    used = set()
    for _, face_line in faces:
        face = face_line.splitlines()[-1]
        refs = face.split()[1:]
        indices = []
        inside = False
        for ref in refs:
            index = parse_vertex_ref(ref)
            if index < 0:
                index = len(vertices) + 1 + index
            indices.append(index)
            x, y, _ = vertices[index-1]
            if math.hypot(x-center_e, y-center_n) <= radius:
                inside = True
        if inside:
            selected.append((face_line, indices))
            used.update(indices)
    if not selected:
        raise RuntimeError(f"No faces inside {radius:.1f} m crop for {source_obj.name}")

    mapping = {old: new for new, old in enumerate(sorted(used), 1)}
    output = []
    for line in lines:
        stripped = line.strip()
        if stripped.startswith("mtllib "):
            output.append(stripped)
    for old in sorted(used):
        x, y, z = vertices[old-1]
        output.append(f"v {x:.6f} {y:.6f} {z:.6f}")
    for line in lines:
        stripped = line.strip()
        if stripped.startswith(("vt ", "vn ")):
            output.append(stripped)

    last_material = None
    face_count = 0
    for face_line, indices in selected:
        pieces = face_line.splitlines()
        material = pieces[0] if len(pieces) > 1 else None
        face = pieces[-1]
        if material and material != last_material:
            output.append(material)
            last_material = material
        refs = face.split()[1:]
        rewritten = []
        for ref, old in zip(refs, indices):
            parts = ref.split("/")
            parts[0] = str(mapping[old])
            rewritten.append("/".join(parts))
        output.append("f " + " ".join(rewritten))
        face_count += 1
    output_obj.parent.mkdir(parents=True, exist_ok=True)
    output_obj.write_text("\n".join(output) + "\n", encoding="utf-8")
    return {
        "vertices": len(mapping),
        "faces": face_count,
        "nearest_vertex_m": round(nearest, 3),
        "crop_radius_m": radius,
    }


def copy_material_dependencies(source_obj: Path, destination: Path) -> list[str]:
    copied = []
    root = source_obj.parent
    for line in source_obj.read_text(encoding="utf-8", errors="replace").splitlines():
        if not line.strip().startswith("mtllib "):
            continue
        name = line.strip().split(maxsplit=1)[1]
        mtl = (root / name).resolve()
        if not mtl.is_file():
            continue
        target = destination / mtl.name
        shutil.copy2(mtl, target)
        copied.append(target.name)
        for mtl_line in mtl.read_text(encoding="utf-8", errors="replace").splitlines():
            parts = mtl_line.strip().split(maxsplit=1)
            if len(parts) != 2 or parts[0].casefold() not in {"map_kd", "map_ka", "map_ks", "map_bump", "bump"}:
                continue
            texture = (mtl.parent / parts[1]).resolve()
            if texture.is_file() and texture.suffix.casefold() in IMAGE_EXTENSIONS:
                out = destination / texture.name
                shutil.copy2(texture, out)
                copied.append(out.name)
    return sorted(set(copied))


def candidate_manifest(target: dict, model_path: Path) -> dict:
    return {
        "schema_version": 1,
        "id": "gugik_mesh_" + target["id"],
        "display_name": target["name"] + " — GUGiK textured mesh candidate",
        "area": "WroclawHeroLandmarks",
        "type": "static_mesh",
        "quality": "hero",
        "source": {
            "model": model_path.as_posix(),
            "license": "GUGiK/Geoportal public geodetic data; verify current source terms in docs/GUGIK_MESH_LANDMARKS.md",
            "reference_directory": None,
        },
        "mesh": {"max_triangles": 1500000, "combine": True, "generate_uv_if_missing": False},
        "lod": {"ratios": [1.0, 0.5, 0.2, 0.08]},
        "collision": {"mode": "simple", "max_hulls": 16},
        "textures": {"base_color": None, "normal": None, "orm": None},
        "unreal": {
            "destination": "/Game/Wroclaw/Landmarks/GUGiK",
            "nanite": True,
            "qa_map": "/Game/Maps/Nadodrze_GIS",
        },
        "qa": {
            "automation_filter": "WTG.GUGiKMesh",
            "cameras": ["front", "rear", "left", "right", "aerial"],
            "screenshot_width": 2560,
            "screenshot_height": 1440,
        },
    }


def process_target(config: dict, target: dict, output_root: Path) -> dict:
    to_projected = Transformer.from_crs("EPSG:4326", config["source_crs"], always_xy=True)
    easting, northing = to_projected.transform(target["longitude"], target["latitude"])
    radius = float(target.get("crop_radius_m", config.get("default_crop_radius_m", 90)))
    target_dir = output_root / target["id"]
    source_dir = target_dir / "source"
    extract_dir = target_dir / "extracted"
    crop_dir = target_dir / "candidate"
    for directory in (source_dir, extract_dir, crop_dir):
        directory.mkdir(parents=True, exist_ok=True)

    urls = feature_info_urls(config["service_url"], easting, northing, radius)
    if not urls:
        raise RuntimeError(f"No GUGiK 3D mesh tiles returned for {target['name']}")

    archives = []
    for index, url in enumerate(urls):
        path = source_dir / f"tile_{index:03d}.zip"
        if not path.is_file():
            path.write_bytes(request_bytes(url, timeout=300))
        archives.append({"path": path, "url": url, "sha256": sha256(path)})

    if extract_dir.exists():
        shutil.rmtree(extract_dir)
    extract_dir.mkdir(parents=True)
    for index, archive in enumerate(archives):
        safe_extract(archive["path"], extract_dir / f"tile_{index:03d}")

    obj_files = sorted(extract_dir.rglob("*.obj"))
    if not obj_files:
        raise RuntimeError(f"No OBJ files found for {target['name']}")

    candidates = []
    errors = []
    for index, obj in enumerate(obj_files):
        cropped = crop_dir / f"{target['id']}_{index:03d}.obj"
        try:
            stats = crop_obj(obj, cropped, easting, northing, radius)
            dependencies = copy_material_dependencies(obj, crop_dir)
            candidates.append({"model": cropped, "stats": stats, "dependencies": dependencies})
        except RuntimeError as exc:
            errors.append(str(exc))
    if not candidates:
        raise RuntimeError(f"No crop candidate generated for {target['name']}: {'; '.join(errors[:3])}")

    # Multiple adjacent 100 m tiles can overlap a landmark. Keep them separate for
    # Blender merge/decimation rather than silently discarding geometry.
    index_file = target_dir / "candidate_index.json"
    manifest = {
        "schema_version": 1,
        "target": target,
        "projected_center": [easting, northing],
        "source_crs": config["source_crs"],
        "service_url": config["service_url"],
        "archives": [{**a, "path": a["path"].relative_to(ROOT).as_posix() if a["path"].is_relative_to(ROOT) else str(a["path"])} for a in archives],
        "candidates": [{
            "model": c["model"].relative_to(ROOT).as_posix() if c["model"].is_relative_to(ROOT) else str(c["model"]),
            "stats": c["stats"],
            "dependencies": c["dependencies"],
        } for c in candidates],
        "next_gate": "Merge candidate OBJ tiles in Blender, decimate/clean, generate a production Pipeline/assets manifest, then run Unreal import and screenshot QA.",
    }
    index_file.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    first_model = candidates[0]["model"]
    (target_dir / "candidate.asset.json").write_text(
        json.dumps(candidate_manifest(target, first_model.relative_to(ROOT) if first_model.is_relative_to(ROOT) else first_model), ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    return manifest


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", type=Path, default=DEFAULT_CONFIG)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--target", action="append", help="target id; repeatable; default = all")
    args = parser.parse_args()
    config = json.loads(args.config.read_text(encoding="utf-8"))
    wanted = set(args.target or [])
    targets = [t for t in config["targets"] if not wanted or t["id"] in wanted]
    if wanted - {t["id"] for t in targets}:
        raise RuntimeError("Unknown target ids: " + ", ".join(sorted(wanted - {t["id"] for t in targets})))
    results = []
    for target in targets:
        print("Preparing", target["name"])
        results.append(process_target(config, target, args.output))
    print(f"Prepared {len(results)} GUGiK textured-mesh landmark candidates")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, RuntimeError, zipfile.BadZipFile) as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        raise SystemExit(1)
