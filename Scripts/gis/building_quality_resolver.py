#!/usr/bin/env python3
"""Resolve the best available building representation with explicit fallbacks."""
from __future__ import annotations

import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_POLICY = ROOT / "Data" / "building_quality_policy.json"


def load_records(path: Path | None, source: str) -> list[dict]:
    if not path:
        return []
    data = json.loads(path.read_text(encoding="utf-8"))
    records = data.get("buildings", data.get("records", data if isinstance(data, list) else []))
    if not isinstance(records, list):
        raise ValueError(f"{path}: expected buildings/records array")
    result = []
    for item in records:
        if not isinstance(item, dict):
            continue
        record = dict(item)
        record.setdefault("source", source)
        identifier = record.get("building_id") or record.get("id") or record.get("replaced_feature_id")
        if not identifier:
            continue
        record["building_id"] = str(identifier)
        result.append(record)
    return result


def available(record: dict, requirements: dict, accepted_qa: set[str], rejected: set[str]) -> tuple[bool, str]:
    status = str(record.get("status", "ready")).casefold()
    if status in rejected:
        return False, f"status={status}"
    if requirements.get("requires_geometry") and not (
        record.get("geometry") or record.get("mesh") or record.get("model") or record.get("feature_id")
    ):
        return False, "geometry missing"
    if requirements.get("requires_texture") and not (
        record.get("texture") or record.get("textures") or record.get("textured") is True
    ):
        return False, "texture missing"
    if requirements.get("requires_qa_pass"):
        qa = str(record.get("qa_status", ""))
        if qa not in accepted_qa:
            return False, f"qa_status={qa or 'missing'}"
    return True, "eligible"


def resolve(policy: dict, source_records: dict[str, list[dict]]) -> dict:
    indexed: dict[str, dict[str, list[dict]]] = {}
    for source_name, records in source_records.items():
        for record in records:
            indexed.setdefault(record["building_id"], {}).setdefault(source_name, []).append(record)

    accepted_qa = set(policy.get("accepted_qa_statuses", ["PASS"]))
    rejected = {str(x).casefold() for x in policy.get("reject_statuses", [])}
    decisions = []
    counts = {name: 0 for name in policy["precedence"]}
    counts["unresolved"] = 0

    for building_id in sorted(indexed):
        candidates = indexed[building_id]
        audit = []
        selected = None
        for source_name in policy["precedence"]:
            requirements = policy["requirements"][source_name]
            options = candidates.get(source_name, [])
            if not options:
                audit.append({"source": source_name, "eligible": False, "reason": "not present"})
                continue
            ranked = sorted(
                options,
                key=lambda item: (
                    0 if item.get("preferred") else 1,
                    -float(item.get("footprint_overlap_ratio", 0.0)),
                    float(item.get("distance_to_reference_m", item.get("distance_to_osm_reference_m", 0.0))),
                    str(item.get("id", "")),
                ),
            )
            for option in ranked:
                ok, reason = available(option, requirements, accepted_qa, rejected)
                audit.append({"source": source_name, "candidate": option.get("id"), "eligible": ok, "reason": reason})
                if ok:
                    selected = dict(option)
                    selected["source"] = source_name
                    break
            if selected:
                break

        if selected:
            source_name = selected["source"]
            counts[source_name] += 1
            decision = {
                "building_id": building_id,
                "selected_source": source_name,
                "quality": policy["quality_labels"][source_name],
                "selected": selected,
                "audit": audit,
            }
        else:
            counts["unresolved"] += 1
            decision = {
                "building_id": building_id,
                "selected_source": None,
                "quality": "missing",
                "selected": None,
                "audit": audit,
            }
        decisions.append(decision)

    return {"schema_version": 1, "counts": counts, "decisions": decisions}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--policy", type=Path, default=DEFAULT_POLICY)
    parser.add_argument("--textured-mesh", type=Path)
    parser.add_argument("--lod2", type=Path)
    parser.add_argument("--lod1", type=Path)
    parser.add_argument("--osm", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    policy = json.loads(args.policy.read_text(encoding="utf-8"))
    if policy.get("schema_version") != 1:
        raise ValueError("Unsupported building quality policy schema")
    sources = {
        "textured_mesh": load_records(args.textured_mesh, "textured_mesh"),
        "lod2": load_records(args.lod2, "lod2"),
        "lod1": load_records(args.lod1, "lod1"),
        "osm": load_records(args.osm, "osm"),
    }
    result = resolve(policy, sources)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result["counts"], sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
