#!/usr/bin/env python3
"""Fetch pinned CC0 Quaternius character and animation assets.

The upstream author distributes these packs as CC0. itch.io's free download flow is
interactive, so this script retrieves exact bytes from a public GitHub mirror pinned
to a commit and verifies every file with its Git blob SHA-1 before writing it.

No unpinned "latest" URLs are used.
"""
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
MIRROR_REPO = "lluancarlo/ReadyToStrategize"
MIRROR_COMMIT = "1a9c05693f705cd7ebe0dd7af022d6b9e250d5c5"
RAW_BASE = f"https://raw.githubusercontent.com/{MIRROR_REPO}/{MIRROR_COMMIT}/"

UBC_SOURCE = "assets/quaternius/Universal Base Characters[Standard]"
UAL1_SOURCE = "assets/quaternius/Universal Animation Library[Standard]"
UAL2_SOURCE = "assets/quaternius/Universal Animation Library 2[Standard]"

UBC_DEST = "SourceAssets/Characters/Quaternius/UniversalBaseCharacters"
UAL1_DEST = "SourceAssets/Animations/Quaternius/UniversalAnimationLibrary1"
UAL2_DEST = "SourceAssets/Animations/Quaternius/UniversalAnimationLibrary2"

OFFICIAL = {
    "ubc": "https://quaternius.com/packs/universalbasecharacters.html",
    "ual1": "https://quaternius.com/packs/universalanimationlibrary.html",
    "ual2": "https://quaternius.com/packs/universalanimationlibrary2.html",
}
LICENSE_URL = "https://creativecommons.org/publicdomain/zero/1.0/"

# source path, target path, expected Git blob SHA-1
SPECS: list[tuple[str, str, str]] = [
    (f"{UBC_SOURCE}/License_Standard.txt", f"{UBC_DEST}/License_Standard.txt", "383cc15f00799f91d6e69407ea8f4037be9492e9"),

    (f"{UBC_SOURCE}/Base Characters/Godot - UE/Superhero_Male_FullBody.gltf", f"{UBC_DEST}/BaseCharacters/Superhero_Male_FullBody.gltf", "7e4b5b57dca66cced9127b25a6f111a0919da00a"),
    (f"{UBC_SOURCE}/Base Characters/Godot - UE/Superhero_Male_FullBody.bin", f"{UBC_DEST}/BaseCharacters/Superhero_Male_FullBody.bin", "54309de6c6ccad823a8ced11056f4013b197b1c6"),
    (f"{UBC_SOURCE}/Base Characters/Godot - UE/Superhero_Female_FullBody.gltf", f"{UBC_DEST}/BaseCharacters/Superhero_Female_FullBody.gltf", "804aa3d3bbbd1977d31ec25c7dbf9a00705be49d"),
    (f"{UBC_SOURCE}/Base Characters/Godot - UE/Superhero_Female_FullBody.bin", f"{UBC_DEST}/BaseCharacters/Superhero_Female_FullBody.bin", "6e6507c85a4993e97d31f187b4d0715af300400d"),

    (f"{UBC_SOURCE}/Base Characters/Godot - UE/T_Eye_Brown.png", f"{UBC_DEST}/BaseCharacters/T_Eye_Brown.png", "0d037febc2789c86fdb40d7ccd4c0a1d0591cfbe"),
    (f"{UBC_SOURCE}/Base Characters/Godot - UE/T_Eye_Normal.png", f"{UBC_DEST}/BaseCharacters/T_Eye_Normal.png", "2da59128bac4b6d4a87fc0ee1d1835cd52e21028"),
    (f"{UBC_SOURCE}/Base Characters/Godot - UE/T_Eye_Normal.png", f"{UBC_DEST}/BaseCharacters/T_Eye_Normal_png.png", "2da59128bac4b6d4a87fc0ee1d1835cd52e21028"),
    (f"{UBC_SOURCE}/Base Characters/Godot - UE/T_Hair_1_BaseColor.png", f"{UBC_DEST}/BaseCharacters/T_Hair_1_BaseColor.png", "c398ca7d10a12d0a1c3fb19d459280bde868594b"),
    (f"{UBC_SOURCE}/Base Characters/Godot - UE/T_Hair_1_Normal.png", f"{UBC_DEST}/BaseCharacters/T_Hair_1_Normal.png", "5aa652eded3befcd3cab03950309c715de42c297"),
    (f"{UBC_SOURCE}/Base Characters/Godot - UE/T_Hair_1_Normal.png", f"{UBC_DEST}/BaseCharacters/T_Hair_1_Normal_png.png", "5aa652eded3befcd3cab03950309c715de42c297"),
    (f"{UBC_SOURCE}/Base Characters/Godot - UE/T_Hair_2_BaseColor.png", f"{UBC_DEST}/BaseCharacters/T_Hair_2_BaseColor.png", "bda8d959b61fc57184bfc287ce2e62143c6faff6"),
    (f"{UBC_SOURCE}/Base Characters/Godot - UE/T_Hair_2_Normal.png", f"{UBC_DEST}/BaseCharacters/T_Hair_2_Normal.png", "2d77b1dd52d5e2954b953364e2cf3555305971ed"),
    (f"{UBC_SOURCE}/Base Characters/Godot - UE/T_Superhero_Male_Dark.png", f"{UBC_DEST}/BaseCharacters/T_Superhero_Male_Dark.png", "6391f45275f6c904edbde5daa500bab26b103366"),
    (f"{UBC_SOURCE}/Base Characters/Godot - UE/T_Superhero_Male_Normal.png", f"{UBC_DEST}/BaseCharacters/T_Superhero_Male_Normal.png", "88303e2ad3f435853cd10d509adbbae960af4279"),
    (f"{UBC_SOURCE}/Base Characters/Godot - UE/T_Superhero_Male_Roughness.png", f"{UBC_DEST}/BaseCharacters/T_Superhero_Male_Roughness.png", "2bbc618d20047d14fd51faf5be5bc272f5a8bf6"),
    (f"{UBC_SOURCE}/Base Characters/Godot - UE/T_Superhero_Female_Dark_BaseColor.png", f"{UBC_DEST}/BaseCharacters/T_Superhero_Female_Dark_BaseColor.png", "06d49a50a69873d30f687f59e74e94d34e69ad9a"),
    (f"{UBC_SOURCE}/Base Characters/Godot - UE/T_Superhero_Female_Normal.png", f"{UBC_DEST}/BaseCharacters/T_Superhero_Female_Normal.png", "20af328bc43a49a54b2b0fab62e5267b9a5993fa"),
    (f"{UBC_SOURCE}/Base Characters/Godot - UE/T_Superhero_Female_Roughness.png", f"{UBC_DEST}/BaseCharacters/T_Superhero_Female_Roughness.png", "d496b5331598258460a00aaedda2b9bc96ab9c8f"),

    (f"{UBC_SOURCE}/Hairstyles/Rigged to Head Bone/glTF (Godot -Unreal)/Eyebrows_Female.gltf", f"{UBC_DEST}/Hairstyles/Eyebrows_Female.gltf", "8766cedc146e8031bc7964e02c6e76c1969be638"),
    (f"{UBC_SOURCE}/Hairstyles/Rigged to Head Bone/glTF (Godot -Unreal)/Eyebrows_Female.bin", f"{UBC_DEST}/Hairstyles/Eyebrows_Female.bin", "5d8f9e3bd4058203904f743b3e1ebb36af5a1e50"),
    (f"{UBC_SOURCE}/Hairstyles/Rigged to Head Bone/glTF (Godot -Unreal)/Eyebrows_Regular.gltf", f"{UBC_DEST}/Hairstyles/Eyebrows_Regular.gltf", "35a9469378aa1fc4bda9a0a06b7c720788abc560"),
    (f"{UBC_SOURCE}/Hairstyles/Rigged to Head Bone/glTF (Godot -Unreal)/Eyebrows_Regular.bin", f"{UBC_DEST}/Hairstyles/Eyebrows_Regular.bin", "93c9302b00e9dcc77583db8e4ee5d901921e61a2"),
    (f"{UBC_SOURCE}/Hairstyles/Rigged to Head Bone/glTF (Godot -Unreal)/Hair_Beard.gltf", f"{UBC_DEST}/Hairstyles/Hair_Beard.gltf", "ca79e6c675f69e1b3410d9315a3b9e46a61d930d"),
    (f"{UBC_SOURCE}/Hairstyles/Rigged to Head Bone/glTF (Godot -Unreal)/Hair_Beard.bin", f"{UBC_DEST}/Hairstyles/Hair_Beard.bin", "dd570929cb4e7c929681b149b4fa9e770ba9b530"),
    (f"{UBC_SOURCE}/Hairstyles/Rigged to Head Bone/glTF (Godot -Unreal)/Hair_Buns.gltf", f"{UBC_DEST}/Hairstyles/Hair_Buns.gltf", "eb63768cc5b783f1d6b06fe25d10ab8910d53116"),
    (f"{UBC_SOURCE}/Hairstyles/Rigged to Head Bone/glTF (Godot -Unreal)/Hair_Buns.bin", f"{UBC_DEST}/Hairstyles/Hair_Buns.bin", "edf82d5d9021529e3272b9e54ed0c7a2d67bcb20"),
    (f"{UBC_SOURCE}/Hairstyles/Rigged to Head Bone/glTF (Godot -Unreal)/Hair_Buzzed.gltf", f"{UBC_DEST}/Hairstyles/Hair_Buzzed.gltf", "41b3436160b05c669ffe5d0bdfd822a0dd5d8a05"),
    (f"{UBC_SOURCE}/Hairstyles/Rigged to Head Bone/glTF (Godot -Unreal)/Hair_Buzzed.bin", f"{UBC_DEST}/Hairstyles/Hair_Buzzed.bin", "9d617b82901bb41fdbfc47f1ecdd69876c6aef93"),
    (f"{UBC_SOURCE}/Hairstyles/Rigged to Head Bone/glTF (Godot -Unreal)/Hair_BuzzedFemale.gltf", f"{UBC_DEST}/Hairstyles/Hair_BuzzedFemale.gltf", "3ab863003205a7e6a8528585b2ecbbeefb37f13d"),
    (f"{UBC_SOURCE}/Hairstyles/Rigged to Head Bone/glTF (Godot -Unreal)/Hair_BuzzedFemale.bin", f"{UBC_DEST}/Hairstyles/Hair_BuzzedFemale.bin", "70c8b956586e68e50a17cd54fdd48d3b8aa61c13"),
    (f"{UBC_SOURCE}/Hairstyles/Rigged to Head Bone/glTF (Godot -Unreal)/Hair_Long.gltf", f"{UBC_DEST}/Hairstyles/Hair_Long.gltf", "cbe0388d7937ee3b271d4b9e6f508aa8241e168a"),
    (f"{UBC_SOURCE}/Hairstyles/Rigged to Head Bone/glTF (Godot -Unreal)/Hair_Long.bin", f"{UBC_DEST}/Hairstyles/Hair_Long.bin", "6dd35df4eb7157b51dff4f537a0e2191e778b0ab"),
    (f"{UBC_SOURCE}/Hairstyles/Rigged to Head Bone/glTF (Godot -Unreal)/Hair_SimpleParted.gltf", f"{UBC_DEST}/Hairstyles/Hair_SimpleParted.gltf", "1f62944bd4b6a00accf32c37ca1c08dbe5cbb421"),
    (f"{UBC_SOURCE}/Hairstyles/Rigged to Head Bone/glTF (Godot -Unreal)/Hair_SimpleParted.bin", f"{UBC_DEST}/Hairstyles/Hair_SimpleParted.bin", "7acd443dbed9640224dda0d9a11dfa4b55bb5c2d"),
    (f"{UBC_SOURCE}/Hairstyles/Rigged to Head Bone/glTF (Godot -Unreal)/T_Hair_1_BaseColor.png", f"{UBC_DEST}/Hairstyles/T_Hair_1_BaseColor.png", "c398ca7d10a12d0a1c3fb19d459280bde868594b"),
    (f"{UBC_SOURCE}/Hairstyles/Rigged to Head Bone/glTF (Godot -Unreal)/T_Hair_1_Normal.png", f"{UBC_DEST}/Hairstyles/T_Hair_1_Normal.png", "5aa652eded3befcd3cab03950309c715de42c297"),
    (f"{UBC_SOURCE}/Hairstyles/Rigged to Head Bone/glTF (Godot -Unreal)/T_Hair_2_BaseColor.png", f"{UBC_DEST}/Hairstyles/T_Hair_2_BaseColor.png", "bda8d959b61fc57184bfc287ce2e62143c6faff6"),
    (f"{UBC_SOURCE}/Hairstyles/Rigged to Head Bone/glTF (Godot -Unreal)/T_Hair_2_Normal.png", f"{UBC_DEST}/Hairstyles/T_Hair_2_Normal.png", "2d77b1dd52d5e2954b953364e2cf3555305971ed"),

    (f"{UAL1_SOURCE}/License.txt", f"{UAL1_DEST}/License.txt", "e29809e680411990a999d5a73ee134c97116d98f"),
    (f"{UAL1_SOURCE}/README.txt", f"{UAL1_DEST}/README.txt", "06b0ae602dbe900709fbbfb7cb5dab7f62a9e5d3"),
    (f"{UAL1_SOURCE}/Unreal-Godot/UAL1_Standard.glb", f"{UAL1_DEST}/UAL1_Standard.glb", "473e59080288428d0b6da826ba19324d07b191f0"),
    (f"{UAL1_SOURCE}/Unreal-Godot/UAL1_Standard_RM.glb", f"{UAL1_DEST}/UAL1_Standard_RM.glb", "49ab49b9abac1c1195c6b2560102b525f82634ec"),

    (f"{UAL2_SOURCE}/License.txt", f"{UAL2_DEST}/License.txt", "e29809e680411990a999d5a73ee134c97116d98f"),
    (f"{UAL2_SOURCE}/README.txt", f"{UAL2_DEST}/README.txt", "06b0ae602dbe900709fbbfb7cb5dab7f62a9e5d3"),
    (f"{UAL2_SOURCE}/Unreal-Godot/UAL2_Standard.glb", f"{UAL2_DEST}/UAL2_Standard.glb", "9a30163d96d890e8849a7956e2d6a61df0276049"),
    (f"{UAL2_SOURCE}/Unreal-Godot/UAL2_Standard_RM.glb", f"{UAL2_DEST}/UAL2_Standard_RM.glb", "391bd82960457b3d48216be6e15557c986958df9"),
]


def git_blob_sha(data: bytes) -> str:
    header = f"blob {len(data)}\0".encode("ascii")
    return hashlib.sha1(header + data).hexdigest()


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def url_for(path: str) -> str:
    return RAW_BASE + urllib.parse.quote(path, safe="/")


def download(path: str) -> bytes:
    request = urllib.request.Request(
        url_for(path),
        headers={"User-Agent": "WroclawTheGame-asset-fetcher/1.0"},
    )
    last = None
    for _ in range(4):
        try:
            with urllib.request.urlopen(request, timeout=90) as response:
                return response.read()
        except Exception as exc:  # pragma: no cover - network retry
            last = exc
    raise RuntimeError(f"Download failed: {path}: {last}")


def verify_bytes(data: bytes, expected_blob: str, label: str) -> None:
    actual = git_blob_sha(data)
    if actual != expected_blob:
        raise RuntimeError(f"Git blob mismatch for {label}: expected {expected_blob}, got {actual}")


def glb_animation_names(data: bytes) -> list[str]:
    if len(data) < 20 or data[:4] != b"glTF":
        raise RuntimeError("Not a GLB file")
    version, total = struct.unpack_from("<II", data, 4)
    if version != 2 or total != len(data):
        raise RuntimeError(f"Unsupported/corrupt GLB: version={version}, declared={total}, real={len(data)}")
    chunk_len, chunk_type = struct.unpack_from("<II", data, 12)
    if chunk_type != 0x4E4F534A:
        raise RuntimeError("GLB first chunk is not JSON")
    payload = data[20:20 + chunk_len].decode("utf-8").rstrip("\x00 ")
    doc = json.loads(payload)
    return [str(item.get("name") or f"Animation_{i}") for i, item in enumerate(doc.get("animations", []))]


def character_catalog() -> list[dict]:
    base = f"{UBC_DEST}/BaseCharacters"
    hair = f"{UBC_DEST}/Hairstyles"
    common = {
        "provider": "Quaternius",
        "pack": "Universal Base Characters [Standard]",
        "license": "CC0-1.0",
        "source_page": OFFICIAL["ubc"],
        "mirror_repo": MIRROR_REPO,
        "mirror_commit": MIRROR_COMMIT,
    }
    return [
        {**common, "id": "quaternius-ubc-superhero-male", "name": "Superhero Male", "category": "body", "sex": "male", "primary_path": f"{base}/Superhero_Male_FullBody.gltf"},
        {**common, "id": "quaternius-ubc-superhero-female", "name": "Superhero Female", "category": "body", "sex": "female", "primary_path": f"{base}/Superhero_Female_FullBody.gltf"},
        {**common, "id": "quaternius-ubc-hair-simple-parted", "name": "Hair Simple Parted", "category": "hair", "primary_path": f"{hair}/Hair_SimpleParted.gltf"},
        {**common, "id": "quaternius-ubc-hair-buzzed", "name": "Hair Buzzed", "category": "hair", "primary_path": f"{hair}/Hair_Buzzed.gltf"},
        {**common, "id": "quaternius-ubc-hair-buzzed-female", "name": "Hair Buzzed Female", "category": "hair", "primary_path": f"{hair}/Hair_BuzzedFemale.gltf"},
        {**common, "id": "quaternius-ubc-hair-long", "name": "Hair Long", "category": "hair", "primary_path": f"{hair}/Hair_Long.gltf"},
        {**common, "id": "quaternius-ubc-hair-buns", "name": "Hair Buns", "category": "hair", "primary_path": f"{hair}/Hair_Buns.gltf"},
        {**common, "id": "quaternius-ubc-beard", "name": "Beard", "category": "beard", "primary_path": f"{hair}/Hair_Beard.gltf"},
        {**common, "id": "quaternius-ubc-eyebrows-regular", "name": "Eyebrows Regular", "category": "eyebrows", "primary_path": f"{hair}/Eyebrows_Regular.gltf"},
        {**common, "id": "quaternius-ubc-eyebrows-female", "name": "Eyebrows Female", "category": "eyebrows", "primary_path": f"{hair}/Eyebrows_Female.gltf"},
    ]


def write_catalogs(file_hashes: dict[str, str]) -> None:
    data_dir = ROOT / "Data"
    data_dir.mkdir(parents=True, exist_ok=True)
    (data_dir / "free_character_catalog.json").write_text(
        json.dumps(character_catalog(), indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )

    anim_specs = [
        ("quaternius-ual1-standard", "Universal Animation Library 1 — Standard", OFFICIAL["ual1"], f"{UAL1_DEST}/UAL1_Standard.glb", False),
        ("quaternius-ual1-standard-rm", "Universal Animation Library 1 — Standard Root Motion", OFFICIAL["ual1"], f"{UAL1_DEST}/UAL1_Standard_RM.glb", True),
        ("quaternius-ual2-standard", "Universal Animation Library 2 — Standard", OFFICIAL["ual2"], f"{UAL2_DEST}/UAL2_Standard.glb", False),
        ("quaternius-ual2-standard-rm", "Universal Animation Library 2 — Standard Root Motion", OFFICIAL["ual2"], f"{UAL2_DEST}/UAL2_Standard_RM.glb", True),
    ]
    catalog = []
    for asset_id, name, source_page, rel, root_motion in anim_specs:
        path = ROOT / rel
        blob = path.read_bytes()
        clips = glb_animation_names(blob)
        if not clips:
            raise RuntimeError(f"No animation clips found in {rel}")
        catalog.append({
            "id": asset_id,
            "name": name,
            "provider": "Quaternius",
            "pack": "Universal Animation Library" + (" 2 [Standard]" if "ual2" in asset_id else " [Standard]"),
            "license": "CC0-1.0",
            "license_url": LICENSE_URL,
            "source_page": source_page,
            "mirror_repo": MIRROR_REPO,
            "mirror_commit": MIRROR_COMMIT,
            "primary_path": rel,
            "root_motion": root_motion,
            "clip_count": len(clips),
            "clips": clips,
            "sha256": file_hashes.get(rel, sha256(blob)),
        })
    (data_dir / "free_animation_catalog.json").write_text(
        json.dumps(catalog, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )


def write_provenance(file_hashes: dict[str, str]) -> None:
    groups = [
        (
            ROOT / UBC_DEST / "asset.json",
            "Universal Base Characters [Standard]",
            OFFICIAL["ubc"],
            [rel for _, rel, _ in SPECS if rel.startswith(UBC_DEST + "/")],
        ),
        (
            ROOT / UAL1_DEST / "asset.json",
            "Universal Animation Library 1 [Standard]",
            OFFICIAL["ual1"],
            [rel for _, rel, _ in SPECS if rel.startswith(UAL1_DEST + "/")],
        ),
        (
            ROOT / UAL2_DEST / "asset.json",
            "Universal Animation Library 2 [Standard]",
            OFFICIAL["ual2"],
            [rel for _, rel, _ in SPECS if rel.startswith(UAL2_DEST + "/")],
        ),
    ]
    for destination, name, source_page, files in groups:
        destination.parent.mkdir(parents=True, exist_ok=True)
        payload = {
            "name": name,
            "author": "Quaternius",
            "license": "CC0-1.0",
            "license_url": LICENSE_URL,
            "source_page": source_page,
            "retrieval_transport": {
                "repo": MIRROR_REPO,
                "commit": MIRROR_COMMIT,
                "reason": "Pinned public byte mirror; official free itch.io flow is interactive.",
            },
            "files": [{"path": p, "sha256": file_hashes[p]} for p in files],
        }
        destination.write_text(json.dumps(payload, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def check_existing() -> dict[str, str]:
    file_hashes: dict[str, str] = {}
    for _, target, expected_blob in SPECS:
        path = ROOT / target
        if not path.is_file():
            raise RuntimeError(f"Missing fetched asset: {target}")
        data = path.read_bytes()
        verify_bytes(data, expected_blob, target)
        file_hashes[target] = sha256(data)
    return file_hashes


def fetch_all() -> dict[str, str]:
    file_hashes: dict[str, str] = {}
    cache: dict[tuple[str, str], bytes] = {}
    for source, target, expected_blob in SPECS:
        key = (source, expected_blob)
        data = cache.get(key)
        if data is None:
            data = download(source)
            verify_bytes(data, expected_blob, source)
            cache[key] = data
        destination = ROOT / target
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(data)
        file_hashes[target] = sha256(data)
        print(f"{target}: {len(data)} bytes")
    return file_hashes


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true", help="Verify committed files without network access")
    args = parser.parse_args()

    hashes = check_existing() if args.check else fetch_all()
    write_catalogs(hashes)
    write_provenance(hashes)

    # Re-check after generated metadata to fail early on any truncated binary.
    check_existing()
    print(f"Verified {len(SPECS)} pinned CC0 source files.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise
