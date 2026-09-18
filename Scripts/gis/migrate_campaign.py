"""Resolve the legacy campaign blockout onto deterministic real-GIS anchors.

This tool does not claim final interiors. It produces transformed runtime data plus
an explicit acceptance report. Stable gameplay IDs are never changed.
"""
import argparse
import copy
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_INPUT = ROOT / "Data" / "processed" / "wroclaw"
DEFAULT_OUTPUT = ROOT / "Saved" / "CampaignGIS"


def _finite_point(value):
    return isinstance(value, list) and len(value) == 3 and all(isinstance(v, (int, float)) and math.isfinite(v) for v in value)


def _zone_for_x(manifest, x):
    zones = [z for z in manifest["zones"] if z["source_min_x"] <= x < z["source_max_x"]]
    if len(zones) != 1:
        raise ValueError(f"Campaign coordinate x={x} maps to {len(zones)} zones")
    return zones[0]


def _route_edge_for_street(data, name):
    nodes = {n["id"]: n for n in data["road_graph"]["nodes"]}
    edges = [
        e for e in data["road_graph"]["edges"]
        if e.get("name") == name and e.get("car_forward") and e.get("bridge", "no") == "no"
        and e.get("tunnel", "no") == "no" and e["from"] in nodes and e["to"] in nodes
    ]
    if not edges:
        raise ValueError(f"No legal GIS anchor edge for street: {name}")
    edge = max(edges, key=lambda e: (e.get("length_cm", 0), e["from"], e["to"]))
    a, b = nodes[edge["from"]]["position"], nodes[edge["to"]]["position"]
    midpoint = [(a[i] + b[i]) / 2 for i in range(3)]
    yaw = math.degrees(math.atan2(b[1] - a[1], b[0] - a[0]))
    return edge, midpoint, yaw


def _building_center(feature):
    rings = feature.get("rings")
    if feature.get("kind") != "building" or not rings or not rings[0]:
        return None
    ring = rings[0]
    return [sum(p[i] for p in ring) / len(ring) for i in range(3)]


def _nearest_building(data, point, radius):
    best = None
    best_dist = float("inf")
    for feature in data.get("features", []):
        center = _building_center(feature)
        if not center:
            continue
        dist = math.dist(center[:2], point[:2])
        if dist < best_dist and dist <= radius:
            best = (feature, center)
            best_dist = dist
    if not best:
        return None
    feature, center = best
    ring = feature["rings"][0]
    xs = [p[0] for p in ring]
    ys = [p[1] for p in ring]
    return {
        "building_id": feature["id"],
        "center": center,
        "distance_cm": best_dist,
        "footprint_size_cm": [max(xs) - min(xs), max(ys) - min(ys)],
    }


def _resolve(manifest, data):
    resolved = []
    for zone in manifest["zones"]:
        edge, street_point, yaw = _route_edge_for_street(data, zone["anchor_street"])
        building = _nearest_building(data, street_point, zone["preferred_building_radius_cm"])
        anchor = building["center"] if building else street_point
        resolved.append({
            **zone,
            "edge": {"from": edge["from"], "to": edge["to"], "name": edge["name"]},
            "target_anchor": anchor,
            "target_yaw": yaw,
            "building": building,
        })
    return resolved


def _transform(point, zone):
    ox, oy, oz = zone["source_anchor"]
    tx, ty, tz = zone["target_anchor"]
    radians = math.radians(zone["target_yaw"])
    dx, dy = point[0] - ox, point[1] - oy
    return [
        tx + dx * math.cos(radians) - dy * math.sin(radians),
        ty + dx * math.sin(radians) + dy * math.cos(radians),
        tz + (point[2] - oz),
    ]


def _transform_point(point, zones):
    zone = _zone_for_x({"zones": zones}, point[0])
    return _transform(point, zone)


def _transform_chapter(chapter, zones):
    out = copy.deepcopy(chapter)
    for action in out["actions"]:
        p = action["position"]
        if _finite_point(p) and p[:2] != [0, 0]:
            action["position"] = _transform_point(p, zones)
    return out


def _transform_environment(environment, zones):
    out = copy.deepcopy(environment)
    for record in out:
        p = record.get("position")
        if not _finite_point(p):
            continue
        zone = _zone_for_x({"zones": zones}, p[0])
        record["position"] = _transform(p, zone)
        rotation = record.get("rotation", [0, 0, 0])
        if len(rotation) == 3:
            rotation = list(rotation)
            # UE Rotator constructor used by the project is pitch,yaw,roll.
            rotation[1] += zone["target_yaw"]
            record["rotation"] = rotation
        record["campaign_zone"] = zone["id"]
    return out


def _transform_bounds(bounds, zones):
    if len(bounds) != 6:
        raise ValueError("Invalid source district bounds")
    points = []
    for x in (bounds[0], bounds[3]):
        # Bounds can straddle migration zones. Sample every zone boundary inside.
        xs = [x]
        for z in zones:
            if bounds[0] < z["source_min_x"] < bounds[3]:
                xs.append(z["source_min_x"] + 0.01)
            if bounds[0] < z["source_max_x"] < bounds[3]:
                xs.append(z["source_max_x"] - 0.01)
        for sx in xs:
            for y in (bounds[1], bounds[4]):
                for zc in (bounds[2], bounds[5]):
                    points.append(_transform_point([sx, y, zc], zones))
    return [
        min(p[0] for p in points), min(p[1] for p in points), min(p[2] for p in points),
        max(p[0] for p in points), max(p[1] for p in points), max(p[2] for p in points),
    ]


def _transform_openworld(world, zones):
    out = copy.deepcopy(world)
    for district in out["districts"]:
        district["bounds"] = _transform_bounds(district["bounds"], zones)
    for guard in out["guards"]:
        guard["position"] = _transform_point(guard["position"], zones)
        guard["patrol"] = [_transform_point(p, zones) for p in guard["patrol"]]
    for location in out["locations"]:
        location["position"] = _transform_point(location["position"], zones)
    for event in out["events"]:
        event["position"] = _transform_point(event["position"], zones)
    for hide in out["hides"]:
        hide["position"] = _transform_point(hide["position"], zones)
        hide["exit"] = _transform_point(hide["exit"], zones)
    for camera in out["cameras"]:
        source = camera["position"]
        zone = _zone_for_x({"zones": zones}, source[0])
        camera["position"] = _transform(source, zone)
        camera["rotation"][1] += zone["target_yaw"]
    for npc in out["npc"]:
        npc["position"] = _transform_point(npc["position"], zones)
        for schedule in npc.get("schedule", []):
            schedule["location"] = _transform_point(schedule["location"], zones)
        npc["destinations"] = [_transform_point(p, zones) for p in npc["destinations"]]
    return out


def _zone_source_extent(chapter, environment, zone):
    points = []
    for action in chapter["actions"]:
        p = action.get("position")
        if _finite_point(p) and p[:2] != [0, 0] and zone["source_min_x"] <= p[0] < zone["source_max_x"]:
            points.append(p)
    for record in environment:
        p = record.get("position")
        if _finite_point(p) and zone["source_min_x"] <= p[0] < zone["source_max_x"]:
            points.append(p)
    if not points:
        return [0, 0]
    return [max(p[0] for p in points) - min(p[0] for p in points),
            max(p[1] for p in points) - min(p[1] for p in points)]


def generate(input_dir=DEFAULT_INPUT, output=DEFAULT_OUTPUT):
    manifest = json.loads((ROOT / "Data" / "campaign_gis.json").read_text(encoding="utf-8"))
    if manifest.get("schema_version") != 1 or not manifest.get("zones"):
        raise ValueError("Unsupported campaign GIS manifest")

    source_chapter = json.loads((ROOT / "Data" / "chapter1.json").read_text(encoding="utf-8"))
    source_world = json.loads((ROOT / "Data" / "openworld.json").read_text(encoding="utf-8"))
    source_environment = json.loads((ROOT / "Data" / "environment.json").read_text(encoding="utf-8"))
    source_opening = json.loads((ROOT / "Data" / "opening_scene_models.json").read_text(encoding="utf-8"))
    data = json.loads((input_dir / "sector.json").read_text(encoding="utf-8"))
    zones = _resolve(manifest, data)

    output.mkdir(parents=True, exist_ok=True)
    chapter = _transform_chapter(source_chapter, zones)
    world = _transform_openworld(source_world, zones)
    environment = _transform_environment(source_environment, zones)
    opening = _transform_environment(source_opening, zones)

    report_zones = []
    for zone in zones:
        extent = _zone_source_extent(source_chapter, source_environment, zone)
        footprint = zone["building"]["footprint_size_cm"] if zone["building"] else [0, 0]
        fits = bool(zone["building"]) and (
            (extent[0] <= footprint[0] and extent[1] <= footprint[1]) or
            (extent[0] <= footprint[1] and extent[1] <= footprint[0])
        )
        blockers = []
        if not zone["building"]:
            blockers.append("no_nearby_building")
        if not fits:
            blockers.append("blockout_exceeds_real_building_footprint")
        if zone["placement"] == "linked_blockout":
            blockers.append("replace_linked_blockout_with_authored_real_interior")
        report_zones.append({
            "id": zone["id"],
            "anchor_street": zone["anchor_street"],
            "edge": zone["edge"],
            "building_id": zone["building"]["building_id"] if zone["building"] else None,
            "target_anchor": zone["target_anchor"],
            "target_yaw": zone["target_yaw"],
            "source_extent_cm": extent,
            "building_footprint_cm": footprint,
            "fits_building_footprint": fits,
            "placement": zone["placement"],
            "blockers": blockers,
        })

    report = {
        "schema_version": 1,
        "source_action_count": len(source_chapter["actions"]),
        "stable_ids_preserved": [a["id"] for a in chapter["actions"]] == [a["id"] for a in source_chapter["actions"]],
        "zones": report_zones,
        "runtime_data_generated": True,
        "playable": all(not z["blockers"] for z in report_zones),
        "acceptance_note": "Generated coordinates are migration scaffolding. Linked blockout is not a final real-building interior.",
    }
    products = {
        "chapter1.json": chapter,
        "openworld.json": world,
        "environment.json": environment,
        "opening_scene_models.json": opening,
        "campaign_migration.json": report,
    }
    for name, payload in products.items():
        (output / name).write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return report


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, default=DEFAULT_INPUT)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--require-playable", action="store_true")
    args = parser.parse_args()
    report = generate(args.input, args.output)
    print(json.dumps(report, ensure_ascii=False, indent=2))
    if args.require_playable and not report["playable"]:
        raise SystemExit("Campaign GIS migration is not Playable: inspect campaign_migration.json blockers")
