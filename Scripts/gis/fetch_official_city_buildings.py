#!/usr/bin/env python3
"""Import all official GUGiK building models that overlap active WroclawTheGame sectors.

This is the scalable background-building path. It keeps the named-landmark importer
for targeted QA, but replaces every safely matched OSM blockout in the active GIS
with official CityGML geometry and groups results into 128 m mesh cells.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import shutil
import sys
from pathlib import Path
import xml.etree.ElementTree as ET

from pyproj import Transformer

import fetch_official_buildings_3d as source

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CONFIG = ROOT / "Data/wroclaw_landmarks_3d.json"
DEFAULT_CITY_DATA = ROOT / "Saved/CityData"
DEFAULT_OUTPUT = ROOT / "Saved/OfficialBuildings3D"
CELL_CM = 12800.0


def safe_id(value: str) -> str:
    value = "".join(c if c.isalnum() or c in "_-" else "_" for c in value)
    value = value.strip("_")
    if not value:
        value = hashlib.sha256(value.encode()).hexdigest()[:16]
    return value[:96]


def city_sector_bounds(city_data: Path) -> list[dict]:
    runtime = json.loads((city_data / "city.json").read_text(encoding="utf-8"))
    sectors = []
    to_utm = Transformer.from_crs("EPSG:4326", "EPSG:32633", always_xy=True)
    for sector in runtime.get("sectors", []):
        bbox = sector.get("bbox")
        if not bbox or len(bbox) != 4:
            continue
        west, south, east, north = bbox
        corners = [to_utm.transform(lon, lat) for lon, lat in (
            (west, south), (east, south), (east, north), (west, north)
        )]
        sectors.append({
            "id": sector["id"],
            "e0": min(p[0] for p in corners),
            "e1": max(p[0] for p in corners),
            "n0": min(p[1] for p in corners),
            "n1": max(p[1] for p in corners),
        })
    if not sectors:
        raise RuntimeError("City runtime catalogue has no geographic sector bounds")
    return sectors


def sector_at(easting: float, northing: float, sectors: list[dict]) -> str | None:
    for sector in sectors:
        if sector["e0"] <= easting <= sector["e1"] and sector["n0"] <= northing <= sector["n1"]:
            return sector["id"]
    return None


def osm_building_index(city: dict, cell_m: float = 50.0):
    origin_e, origin_n, _ = city["origin_projected_m"]
    records = {}
    grid: dict[tuple[int, int], list[str]] = {}
    for feature in city.get("features", []):
        if feature.get("kind") != "building" or not feature.get("rings") or not feature["rings"][0]:
            continue
        ring = feature["rings"][0]
        x = sum(point[0] for point in ring) / len(ring)
        y = sum(point[1] for point in ring) / len(ring)
        e = origin_e + x / 100.0
        n = origin_n - y / 100.0
        record = {
            "feature_id": feature["id"],
            "easting": e,
            "northing": n,
            "base_z_cm": max(point[2] for point in ring),
            "material": feature.get("material", "Brick"),
            "sector": feature.get("sector"),
        }
        records[feature["id"]] = record
        key = (math.floor(e / cell_m), math.floor(n / cell_m))
        grid.setdefault(key, []).append(feature["id"])
    if not records:
        raise RuntimeError("City data contains no OSM building blockouts")
    return records, grid, cell_m


def nearest_osm(easting: float, northing: float, records, grid, cell_m: float, max_distance: float):
    key = (math.floor(easting / cell_m), math.floor(northing / cell_m))
    radius = max(1, math.ceil(max_distance / cell_m))
    best = None
    for dx in range(-radius, radius + 1):
        for dy in range(-radius, radius + 1):
            for feature_id in grid.get((key[0] + dx, key[1] + dy), []):
                record = records[feature_id]
                distance = math.hypot(easting - record["easting"], northing - record["northing"])
                if distance <= max_distance and (best is None or distance < best[0]):
                    best = (distance, record)
    return best


def merge_mesh(group: dict, record: dict):
    offset = len(group["vertices"])
    group["vertices"].extend(record["vertices"])
    group["triangles"].extend(index + offset for index in record["triangles"])


def parse_active_buildings(
    paths: list[Path],
    city: dict,
    sectors: list[dict],
    max_match_distance: float,
    max_buildings: int = 0,
):
    records, grid, grid_size = osm_building_index(city)
    used_osm: set[str] = set()
    groups: dict[tuple[float, float, str], dict] = {}
    buildings = []
    skipped = {"outside_sectors": 0, "no_osm_match": 0, "duplicate_osm": 0, "no_surface": 0}
    by_sector: dict[str, int] = {}

    for path in paths:
        crs = source.detect_crs(path)
        to_utm = Transformer.from_crs(crs, "EPSG:32633", always_xy=True)
        for _, element in ET.iterparse(path, events=("end",)):
            if source.local_name(element.tag) != "Building":
                continue
            surfaces = source.building_surfaces(element)
            if not surfaces:
                skipped["no_surface"] += 1
                element.clear()
                continue
            points = [point for surface in surfaces for ring in surface for point in ring]
            cx = sum(point[0] for point in points) / len(points)
            cy = sum(point[1] for point in points) / len(points)
            ce, cn = to_utm.transform(cx, cy)
            sector = sector_at(ce, cn, sectors)
            if not sector:
                skipped["outside_sectors"] += 1
                element.clear()
                continue
            match = nearest_osm(ce, cn, records, grid, grid_size, max_match_distance)
            if not match:
                skipped["no_osm_match"] += 1
                element.clear()
                continue
            distance, reference = match
            if reference["feature_id"] in used_osm:
                skipped["duplicate_osm"] += 1
                element.clear()
                continue

            gml_id = source.building_id(element)
            candidate = {
                "distance_m": distance,
                "gml_id": gml_id,
                "source_file": path.name,
                "source_crs": crs,
                "centroid_utm": [ce, cn],
                "surfaces": source.transform_surfaces(surfaces, to_utm),
            }
            target = {
                "id": "gugik_" + safe_id(gml_id),
                "display_name": gml_id,
                "material": reference["material"],
                "reference": reference,
            }
            mesh = source.mesh_record(target, candidate, city["origin_projected_m"])
            key = (mesh["origin"][0], mesh["origin"][1], mesh["material"])
            group = groups.setdefault(key, {
                "kind": "official_building",
                "material": mesh["material"],
                "origin": mesh["origin"],
                "vertices": [],
                "triangles": [],
                "ids": [],
            })
            merge_mesh(group, mesh)
            group["ids"].append(target["id"])
            used_osm.add(reference["feature_id"])
            by_sector[sector] = by_sector.get(sector, 0) + 1
            buildings.append({
                "id": target["id"],
                "display_name": gml_id,
                "status": "generated",
                "gml_id": gml_id,
                "source_file": path.name,
                "source_crs": crs,
                "distance_to_osm_reference_m": round(distance, 3),
                "replaced_feature_id": reference["feature_id"],
                "sector": sector,
                "material": reference["material"],
                "mesh_group": None,
            })
            element.clear()
            if max_buildings and len(buildings) >= max_buildings:
                return buildings, groups, skipped, by_sector
    return buildings, groups, skipped, by_sector


def write_groups(output: Path, buildings: list[dict], groups: dict):
    mesh_dir = output / "Meshes"
    if mesh_dir.exists():
        shutil.rmtree(mesh_dir)
    mesh_dir.mkdir(parents=True)
    mesh_files = []
    id_to_file = {}
    for index, (key, group) in enumerate(sorted(groups.items(), key=lambda item: item[0])):
        ox, oy, material = key
        filename = f"official_{int(ox//CELL_CM)}_{int(oy//CELL_CM)}_{safe_id(material)}_{index:04d}.json"
        payload = {k: v for k, v in group.items() if k != "ids"}
        (mesh_dir / filename).write_text(json.dumps(payload, separators=(",", ":")) + "\n", encoding="utf-8")
        mesh_files.append(filename)
        for building_id in group["ids"]:
            id_to_file[building_id] = filename
    for building in buildings:
        building["mesh"] = id_to_file[building["id"]]
        building.pop("mesh_group", None)
    (mesh_dir / "meshes.json").write_text(json.dumps(mesh_files) + "\n", encoding="utf-8")
    return mesh_files


def run(
    config_path: Path,
    city_data: Path,
    output: Path,
    archive: Path | None,
    max_match_distance: float,
    max_buildings: int,
):
    config = json.loads(config_path.read_text(encoding="utf-8"))
    city = json.loads((city_data / "sector.json").read_text(encoding="utf-8"))
    if city.get("projected_crs") != "EPSG:32633":
        raise RuntimeError("Unsupported city CRS")
    sectors = city_sector_bounds(city_data)
    output.mkdir(parents=True, exist_ok=True)

    if archive is None:
        lon = sum((sector["e0"] + sector["e1"]) / 2 for sector in sectors) / len(sectors)
        lat = sum((sector["n0"] + sector["n1"]) / 2 for sector in sectors) / len(sectors)
        to_geo = Transformer.from_crs("EPSG:32633", "EPSG:4326", always_xy=True)
        click_lon, click_lat = to_geo.transform(lon, lat)
        try:
            archive, download_url = source.download_source(
                output, "LoD1", click_lon, click_lat, None
            )
        except Exception:
            fallback = config.get("fallback_direct_url")
            if not fallback:
                raise
            archive, download_url = source.download_source(
                output, "LoD1", click_lon, click_lat, fallback
            )
    else:
        archive = archive.resolve()
        if not archive.is_file():
            raise RuntimeError(f"Archive not found: {archive}")
        download_url = f"file://{archive}"

    paths = source.extract_gml(archive, output)
    buildings, groups, skipped, by_sector = parse_active_buildings(
        paths, city, sectors, max_match_distance, max_buildings
    )
    if not buildings:
        raise RuntimeError("No official GUGiK buildings matched active game sectors")
    mesh_files = write_groups(output, buildings, groups)
    catalog = {
        "schema_version": 1,
        "source": "GUGiK Geoportal - Modele 3D budynków",
        "scope": "active_game_sectors",
        "download_url": download_url,
        "archive_sha256": source.sha256(archive),
        "city_origin_projected_m": city["origin_projected_m"],
        "match_distance_m": max_match_distance,
        "building_count": len(buildings),
        "mesh_group_count": len(mesh_files),
        "counts_by_sector": by_sector,
        "skipped": skipped,
        "vertical_alignment": (
            "CityGML relative height aligned to the existing GIS/OSM building ground Z; "
            "authoritative GUGiK horizontal coordinates are preserved."
        ),
        "buildings": buildings,
    }
    (output / "catalog.json").write_text(
        json.dumps(catalog, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    return catalog


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", type=Path, default=DEFAULT_CONFIG)
    parser.add_argument("--city-data", type=Path, default=DEFAULT_CITY_DATA)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--archive", type=Path)
    parser.add_argument("--match-distance", type=float, default=35.0)
    parser.add_argument("--max-buildings", type=int, default=0, help="0 = all matched buildings")
    args = parser.parse_args()
    if args.match_distance <= 0 or args.max_buildings < 0:
        raise SystemExit("Invalid matching limits")
    catalog = run(
        args.config,
        args.city_data,
        args.output,
        args.archive,
        args.match_distance,
        args.max_buildings,
    )
    print(
        f"Official Wrocław city buildings generated: {catalog['building_count']} "
        f"in {catalog['mesh_group_count']} mesh groups"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, RuntimeError, ET.ParseError) as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        raise SystemExit(1)
