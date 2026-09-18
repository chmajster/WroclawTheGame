#!/usr/bin/env python3
"""Fetch the pinned CC0 KayKit Character Animations 1.1 Rig_Medium libraries."""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
import urllib.parse
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MIRROR_REPO = "georg-doc/kayfabizarro"
MIRROR_COMMIT = "11d7df978c63b9e375707bd8d9431b4c8358cda8"
SOURCE_ROOT = "media/3D_Assets/KayKit_Character_Animations_1.1/Animations/gltf/Rig_Medium"
DEST_ROOT = "SourceAssets/Animations/KayKit/CharacterAnimations1_1/Rig_Medium"
OFFICIAL_PAGE = "https://kaylousberg.itch.io/kaykit-character-animations"
LICENSE_URL = "https://creativecommons.org/publicdomain/zero/1.0/"

SPECS = [
    ("General", "Rig_Medium_General.glb", "5d16cb6815fc8371705147188813f851c10ba26a"),
    ("MovementBasic", "Rig_Medium_MovementBasic.glb", "98e965e886ec539e80f8984a77a29b0c1c02e5e5"),
    ("MovementAdvanced", "Rig_Medium_MovementAdvanced.glb", "f3ea309627f3ad76b92b85877ebc46f945cd4f1d"),
    ("CombatMelee", "Rig_Medium_CombatMelee.glb", "0ab63f221ca5d5c7c485a245199fb6107f1f3764"),
    ("CombatRanged", "Rig_Medium_CombatRanged.glb", "df56bad0071108e4d735771483233aa4fc85456c"),
    ("Tools", "Rig_Medium_Tools.glb", "bc9db031f6608ce4b5e2b1c07f3cfae0c9b152a3"),
    ("Simulation", "Rig_Medium_Simulation.glb", "c64056cda5d2fca14ecb3b61d02d9dde56d1b401"),
    ("Special", "Rig_Medium_Special.glb", "4c2277dee56909c52800430f0a0daa419fe58654"),
]


def git_blob_sha(data: bytes) -> str:
    return hashlib.sha1(f"blob {len(data)}\0".encode("ascii") + data).hexdigest()


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def raw_url(filename: str) -> str:
    path = f"{SOURCE_ROOT}/{filename}"
    return (
        f"https://raw.githubusercontent.com/{MIRROR_REPO}/{MIRROR_COMMIT}/"
        + urllib.parse.quote(path, safe="/")
    )


def download(filename: str) -> bytes:
    request = urllib.request.Request(
        raw_url(filename),
        headers={"User-Agent": "WroclawTheGame-asset-fetcher/1.0"},
    )
    last = None
    for _ in range(4):
        try:
            with urllib.request.urlopen(request, timeout=90) as response:
                return response.read()
        except Exception as exc:  # pragma: no cover
            last = exc
    raise RuntimeError(f"Download failed: {filename}: {last}")


def verify(data: bytes, expected: str, label: str) -> None:
    actual = git_blob_sha(data)
    if actual != expected:
        raise RuntimeError(f"Git blob mismatch for {label}: expected {expected}, got {actual}")


def glb_animation_names(data: bytes) -> list[str]:
    if len(data) < 20 or data[:4] != b"glTF":
        raise RuntimeError("Not a GLB file")
    version, total = struct.unpack_from("<II", data, 4)
    if version != 2 or total != len(data):
        raise RuntimeError(f"Unsupported/corrupt GLB: version={version}, declared={total}, real={len(data)}")
    json_len, json_type = struct.unpack_from("<II", data, 12)
    if json_type != 0x4E4F534A:
        raise RuntimeError("GLB first chunk is not JSON")
    payload = data[20:20 + json_len].decode("utf-8").rstrip("\x00 ")
    doc = json.loads(payload)
    return [str(item.get("name") or f"Animation_{i}") for i, item in enumerate(doc.get("animations", []))]


def check_existing() -> dict[str, bytes]:
    blobs = {}
    for _, filename, expected in SPECS:
        rel = f"{DEST_ROOT}/{filename}"
        path = ROOT / rel
        if not path.is_file():
            raise RuntimeError(f"Missing fetched asset: {rel}")
        data = path.read_bytes()
        verify(data, expected, rel)
        blobs[filename] = data
    return blobs


def fetch_all() -> dict[str, bytes]:
    blobs = {}
    for _, filename, expected in SPECS:
        data = download(filename)
        verify(data, expected, filename)
        destination = ROOT / DEST_ROOT / filename
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(data)
        blobs[filename] = data
        print(f"{destination.relative_to(ROOT)}: {len(data)} bytes")
    return blobs


def write_metadata(blobs: dict[str, bytes]) -> None:
    catalog = []
    for category, filename, blob_sha in SPECS:
        data = blobs[filename]
        clips = glb_animation_names(data)
        if not clips:
            raise RuntimeError(f"No animation clips found in {filename}")
        catalog.append({
            "id": f"kaykit-ca11-rig-medium-{category.lower()}",
            "name": f"KayKit Character Animations 1.1 — Rig Medium {category}",
            "provider": "KayKit",
            "author": "Kay Lousberg",
            "pack": "KayKit Character Animations 1.1",
            "rig": "Rig_Medium",
            "category": category,
            "license": "CC0-1.0",
            "license_url": LICENSE_URL,
            "source_page": OFFICIAL_PAGE,
            "mirror_repo": MIRROR_REPO,
            "mirror_commit": MIRROR_COMMIT,
            "git_blob_sha1": blob_sha,
            "primary_path": f"{DEST_ROOT}/{filename}",
            "root_motion": False,
            "clip_count": len(clips),
            "clips": clips,
            "sha256": sha256(data),
        })

    data_dir = ROOT / "Data"
    data_dir.mkdir(parents=True, exist_ok=True)
    (data_dir / "free_kaykit_animation_catalog.json").write_text(
        json.dumps(catalog, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )

    dest = ROOT / "SourceAssets/Animations/KayKit/CharacterAnimations1_1"
    dest.mkdir(parents=True, exist_ok=True)
    notice = (
        "KayKit Character Animations 1.1\n"
        "Author: Kay Lousberg (KayKit)\n"
        "License: CC0 1.0 Universal\n"
        f"Official source: {OFFICIAL_PAGE}\n"
        f"License reference: {LICENSE_URL}\n"
        f"Pinned byte mirror: https://github.com/{MIRROR_REPO}/commit/{MIRROR_COMMIT}\n"
        "Only the Rig_Medium GLB animation libraries required by WroclawTheGame are vendored here.\n"
    )
    (dest / "LICENSE_CC0_NOTICE.txt").write_text(notice, encoding="utf-8")
    provenance = {
        "name": "KayKit Character Animations 1.1 — Rig_Medium",
        "author": "Kay Lousberg",
        "provider": "KayKit",
        "license": "CC0-1.0",
        "license_url": LICENSE_URL,
        "source_page": OFFICIAL_PAGE,
        "retrieval_transport": {
            "repo": MIRROR_REPO,
            "commit": MIRROR_COMMIT,
            "reason": "Pinned public byte mirror of the current free pack.",
        },
        "files": [
            {
                "path": f"{DEST_ROOT}/{filename}",
                "git_blob_sha1": blob_sha,
                "sha256": sha256(blobs[filename]),
            }
            for _, filename, blob_sha in SPECS
        ],
    }
    (dest / "asset.json").write_text(
        json.dumps(provenance, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    blobs = check_existing() if args.check else fetch_all()
    write_metadata(blobs)
    check_existing()
    print(f"Verified {len(SPECS)} pinned KayKit Rig_Medium GLB libraries.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise
