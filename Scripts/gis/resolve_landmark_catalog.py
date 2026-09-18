#!/usr/bin/env python3
"""Resolve the curated Wrocław landmark catalogue against generated GIS/OSM data."""
from __future__ import annotations

import argparse
import json
import re
import unicodedata
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CATALOG = ROOT / "Data" / "wroclaw_landmark_catalog.json"
DEFAULT_CITY = ROOT / "Saved" / "CityData" / "sector.json"


def normalise(value: str) -> str:
    value = unicodedata.normalize("NFKD", value.casefold())
    value = "".join(ch for ch in value if not unicodedata.combining(ch))
    return " ".join(re.sub(r"[^a-z0-9]+", " ", value).split())


def feature_names(feature: dict) -> set[str]:
    tags = feature.get("tags", {})
    values = [
        feature.get("name", ""),
        tags.get("name", ""),
        tags.get("official_name", ""),
        tags.get("short_name", ""),
        tags.get("alt_name", ""),
    ]
    return {normalise(value) for value in values if value}


def score(record: dict, feature: dict) -> tuple[int, int, str]:
    aliases = {normalise(record["name"]), *(normalise(v) for v in record.get("osm_names", []))}
    names = feature_names(feature)
    exact = bool(aliases & names)
    partial = any(a in n or n in a for a in aliases for n in names if len(a) >= 4 and len(n) >= 4)
    kind_penalty = 0 if feature.get("kind") == "building" else 1
    return (0 if exact else (1 if partial else 2), kind_penalty, str(feature.get("id", "")))


def resolve(catalog: dict, city: dict) -> dict:
    features = [f for f in city.get("features", []) if feature_names(f)]
    resolved = []
    counts = {"resolved": 0, "unresolved": 0}
    for record in catalog["records"]:
        ranked = sorted((score(record, feature), feature) for feature in features)
        match = ranked[0] if ranked and ranked[0][0][0] < 2 else None
        if match:
            _, feature = match
            resolved.append({
                **record,
                "status": "resolved",
                "feature_id": feature["id"],
                "feature_kind": feature.get("kind"),
                "matched_names": sorted(feature_names(feature)),
                "height_m": feature.get("height_m"),
                "sector": feature.get("sector"),
            })
            counts["resolved"] += 1
        else:
            resolved.append({**record, "status": "unresolved", "reason": "no exact/partial OSM name match in current GIS extract"})
            counts["unresolved"] += 1
    return {"schema_version": 1, "counts": counts, "records": resolved}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    parser.add_argument("--city", type=Path, default=DEFAULT_CITY)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    catalog = json.loads(args.catalog.read_text(encoding="utf-8"))
    city = json.loads(args.city.read_text(encoding="utf-8"))
    result = resolve(catalog, city)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result["counts"], sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
