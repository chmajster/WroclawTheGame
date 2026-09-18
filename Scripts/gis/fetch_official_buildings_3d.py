#!/usr/bin/env python3
"""Download official GUGiK 3D building data for Wrocław and extract configured landmarks.

The source CityGML remains authoritative. Derived mesh JSON uses the same centimetre,
X-east/Y-south/Z-up coordinate convention as the rest of WroclawTheGame GIS.
"""
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
import xml.etree.ElementTree as ET

from pyproj import Transformer
from shapely.geometry import Polygon
from shapely.ops import triangulate
from shapely.validation import make_valid

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CONFIG = ROOT / "Data/wroclaw_landmarks_3d.json"
DEFAULT_CITY_DATA = ROOT / "Saved/CityData"
DEFAULT_OUTPUT = ROOT / "Saved/OfficialBuildings3D"
SERVICE_URL = "https://integracja.gugik.gov.pl/cgi-bin/ModeleBudynkow3D"
USER_AGENT = "WroclawTheGame/official-buildings-3d (+https://github.com/chmajster/WroclawTheGame)"


def local_name(tag: str) -> str:
    return tag.rsplit("}", 1)[-1]


def normalise(value: str) -> str:
    value = value.casefold()
    value = re.sub(r"[^0-9a-ząćęłńóśźż]+", " ", value)
    return " ".join(value.split())


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def request_bytes(url: str, timeout: int = 90) -> bytes:
    request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT, "Accept": "*/*"})
    with urllib.request.urlopen(request, timeout=timeout) as response:
        return response.read()


def wms_url(params: dict[str, str]) -> str:
    return SERVICE_URL + "?" + urllib.parse.urlencode(params)


def layer_records(capabilities: bytes) -> list[dict]:
    root = ET.fromstring(capabilities)
    records = []
    for layer in root.iter():
        if local_name(layer.tag) != "Layer":
            continue
        name = ""
        title = ""
        queryable = layer.attrib.get("queryable", "0") in {"1", "true", "TRUE"}
        for child in list(layer):
            key = local_name(child.tag)
            if key == "Name" and child.text:
                name = child.text.strip()
            elif key == "Title" and child.text:
                title = child.text.strip()
        if name:
            records.append({"name": name, "title": title or name, "queryable": queryable})
    return records


def select_layer(records: list[dict], preferred: str) -> dict:
    if not records:
        raise RuntimeError("GUGiK WMS returned no named layers")
    preferred = preferred.casefold()

    def score(record: dict) -> tuple[int, int, str]:
        text = f"{record['name']} {record['title']}".casefold()
        lod2 = "lod2" in text or "lod 2" in text
        lod1 = "lod1" in text or "lod 1" in text
        year = max([int(v) for v in re.findall(r"20\d{2}", text)] or [0])
        if preferred == "lod2":
            primary = 3 if lod2 else (1 if lod1 else 0)
        elif preferred == "lod1":
            primary = 3 if lod1 else (1 if lod2 else 0)
        else:
            primary = 3 if lod2 else (2 if lod1 else 0)
        return primary, year, record["name"]

    selected = max(records, key=score)
    if score(selected)[0] == 0:
        raise RuntimeError(f"Cannot identify LoD layer from GUGiK WMS: {[r['title'] for r in records]}")
    return selected


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


def feature_info_links(layer: str, longitude: float, latitude: float) -> list[tuple[str, str]]:
    east, north = Transformer.from_crs("EPSG:4326", "EPSG:2180", always_xy=True).transform(longitude, latitude)
    west, east_lon = longitude - 0.02, longitude + 0.02
    south, north_lat = latitude - 0.015, latitude + 0.015
    common = {
        "SERVICE": "WMS",
        "REQUEST": "GetFeatureInfo",
        "LAYERS": layer,
        "QUERY_LAYERS": layer,
        "STYLES": "",
        "WIDTH": "101",
        "HEIGHT": "101",
        "FORMAT": "image/png",
        "INFO_FORMAT": "text/html",
    }
    attempts = [
        {
            **common,
            "VERSION": "1.3.0",
            "CRS": "EPSG:2180",
            # EPSG:2180 authority axis order in WMS 1.3.0 is northing,easting.
            "BBOX": f"{north-500},{east-500},{north+500},{east+500}",
            "I": "50",
            "J": "50",
        },
        {
            **common,
            "VERSION": "1.3.0",
            "CRS": "EPSG:4326",
            "BBOX": f"{south},{west},{north_lat},{east_lon}",
            "I": "50",
            "J": "50",
        },
        {
            **common,
            "VERSION": "1.1.1",
            "SRS": "EPSG:4326",
            "BBOX": f"{west},{south},{east_lon},{north_lat}",
            "X": "50",
            "Y": "50",
        },
    ]
    errors = []
    for params in attempts:
        url = wms_url(params)
        try:
            payload = request_bytes(url)
            parser = LinkParser()
            parser.feed(payload.decode("utf-8", "replace"))
            if parser.links:
                return [(urllib.parse.urljoin(url, href), text) for href, text in parser.links]
            errors.append(f"no links in {params['VERSION']} {params.get('CRS',params.get('SRS'))} response")
        except Exception as exc:
            errors.append(f"{params['VERSION']}: {exc}")
    raise RuntimeError("GUGiK GetFeatureInfo failed: " + "; ".join(errors))


def choose_download_link(links: list[tuple[str, str]], preferred: str) -> str:
    if not links:
        raise RuntimeError("No download links returned by GUGiK")
    preferred = preferred.casefold()

    def score(item: tuple[str, str]) -> tuple[int, int, int, str]:
        href, text = item
        haystack = f"{href} {text}".casefold()
        lod2 = "lod2" in haystack or "lod 2" in haystack
        lod1 = "lod1" in haystack or "lod 1" in haystack
        archive = any(token in haystack for token in (".zip", "download", "pobierz", "gml"))
        year = max([int(v) for v in re.findall(r"20\d{2}", haystack)] or [0])
        if preferred == "lod2":
            lod = 4 if lod2 else (1 if lod1 else 0)
        elif preferred == "lod1":
            lod = 4 if lod1 else (1 if lod2 else 0)
        else:
            lod = 4 if lod2 else (3 if lod1 else 0)
        return lod, int(archive), year, href

    selected = max(links, key=score)
    if score(selected)[1] == 0:
        raise RuntimeError(f"GUGiK response does not contain a downloadable dataset URL: {links}")
    return selected[0]


def download_source(output: Path, preferred_lod: str, click_lon: float, click_lat: float, direct_url: str | None = None) -> tuple[Path, str]:
    source_dir = output / "Source"
    source_dir.mkdir(parents=True, exist_ok=True)
    if direct_url:
        layer = {"name": preferred_lod.casefold(), "title": f"Wrocław {preferred_lod}", "queryable": False}
        download_url = direct_url
    else:
        capabilities_url = wms_url({"SERVICE": "WMS", "REQUEST": "GetCapabilities", "VERSION": "1.3.0"})
        capabilities = request_bytes(capabilities_url)
        records = layer_records(capabilities)
        preferences = [preferred_lod]
        if preferred_lod.casefold() in {"auto", "lod2"}:
            preferences += ["LoD1"]
        errors = []
        layer = None
        download_url = None
        used = set()
        for preference in preferences:
            try:
                candidate = select_layer(records, preference)
                if candidate["name"] in used:
                    continue
                used.add(candidate["name"])
                links = feature_info_links(candidate["name"], click_lon, click_lat)
                candidate_url = choose_download_link(links, preference)
                layer, download_url = candidate, candidate_url
                break
            except Exception as exc:
                errors.append(f"{preference}: {exc}")
        if not layer or not download_url:
            raise RuntimeError("No GUGiK 3D building package covers Wrocław: " + "; ".join(errors))

    parsed = urllib.parse.urlparse(download_url)
    filename = Path(urllib.parse.unquote(parsed.path)).name or "gugik_buildings_3d.zip"
    if "." not in filename:
        filename += ".zip"
    archive = source_dir / filename
    meta_path = source_dir / "source.metadata.json"
    cached = False
    if archive.is_file() and meta_path.is_file():
        try:
            metadata = json.loads(meta_path.read_text(encoding="utf-8"))
            cached = metadata.get("download_url") == download_url and metadata.get("sha256") == sha256(archive)
        except (OSError, ValueError):
            cached = False
    if not cached:
        archive.write_bytes(request_bytes(download_url, timeout=300))
    digest = sha256(archive)
    metadata = {
        "source": "GUGiK Geoportal - Modele 3D budynków",
        "service_url": SERVICE_URL,
        "layer": layer,
        "requested_lod": preferred_lod,
        "download_url": download_url,
        "sha256": digest,
        "license_note": "Geoportal states that 3D building models are free and may be used for any purpose.",
    }
    meta_path.write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return archive, download_url


def extract_gml(archive: Path, output: Path) -> list[Path]:
    extracted = output / "Extracted"
    if extracted.exists():
        shutil.rmtree(extracted)
    extracted.mkdir(parents=True)
    if zipfile.is_zipfile(archive):
        with zipfile.ZipFile(archive) as package:
            for member in package.infolist():
                if member.is_dir():
                    continue
                suffix = Path(member.filename).suffix.casefold()
                if suffix not in {".gml", ".xml"}:
                    continue
                destination = (extracted / member.filename).resolve()
                if not destination.is_relative_to(extracted.resolve()):
                    raise RuntimeError(f"Unsafe archive path: {member.filename}")
                destination.parent.mkdir(parents=True, exist_ok=True)
                with package.open(member) as source, destination.open("wb") as target:
                    shutil.copyfileobj(source, target)
    elif archive.suffix.casefold() in {".gml", ".xml"}:
        destination = extracted / archive.name
        shutil.copy2(archive, destination)
    else:
        raise RuntimeError(f"Unsupported GUGiK package format: {archive}")
    result = sorted([*extracted.rglob("*.gml"), *extracted.rglob("*.xml")])
    if not result:
        raise RuntimeError("Downloaded GUGiK package contains no CityGML/XML files")
    return result


def crs_from_srs_name(value: str) -> str | None:
    if not value:
        return None
    if value.upper().startswith("EPSG:"):
        return value.upper()
    codes = [int(v) for v in re.findall(r"(?<!\d)(\d{4,5})(?!\d)", value)]
    horizontal_priority = [2180, 2176, 2177, 2178, 2179, 32633, 4326]
    for code in horizontal_priority:
        if code in codes:
            return f"EPSG:{code}"
    return f"EPSG:{codes[0]}" if codes else None


def detect_crs(path: Path) -> str:
    for _, element in ET.iterparse(path, events=("start",)):
        for key, value in element.attrib.items():
            if local_name(key) == "srsName":
                crs = crs_from_srs_name(value)
                if crs:
                    return crs
        if local_name(element.tag) in {"Building", "BuildingPart"}:
            break
    raise RuntimeError(f"Cannot detect CityGML horizontal CRS: {path}")


def dimensions(element: ET.Element, values: list[float]) -> int:
    for key, value in element.attrib.items():
        if local_name(key) in {"srsDimension", "dimension"}:
            try:
                result = int(value)
                if result in {2, 3}:
                    return result
            except ValueError:
                pass
    if len(values) % 3 == 0:
        return 3
    if len(values) % 2 == 0:
        return 2
    raise RuntimeError("Unsupported CityGML coordinate dimension")


def coordinate_sequence(element: ET.Element) -> list[tuple[float, float, float]]:
    for node in element.iter():
        name = local_name(node.tag)
        if name == "posList" and node.text:
            values = [float(v) for v in node.text.split()]
            dim = dimensions(node, values)
            return [
                (values[i], values[i + 1], values[i + 2] if dim == 3 else 0.0)
                for i in range(0, len(values), dim)
            ]
    result = []
    for node in element.iter():
        if local_name(node.tag) == "pos" and node.text:
            values = [float(v) for v in node.text.split()]
            if len(values) >= 2:
                result.append((values[0], values[1], values[2] if len(values) >= 3 else 0.0))
    return result


def polygon_rings(polygon: ET.Element) -> list[list[tuple[float, float, float]]]:
    rings = []
    for role in ("exterior", "interior"):
        for child in polygon.iter():
            if local_name(child.tag) != role:
                continue
            points = coordinate_sequence(child)
            if len(points) >= 4 and points[0] == points[-1]:
                points = points[:-1]
            if len(points) >= 3:
                rings.append(points)
    if not rings:
        points = coordinate_sequence(polygon)
        if len(points) >= 4 and points[0] == points[-1]:
            points = points[:-1]
        if len(points) >= 3:
            rings.append(points)
    return rings


def building_surfaces(element: ET.Element) -> list[list[list[tuple[float, float, float]]]]:
    surfaces = []
    for node in element.iter():
        if local_name(node.tag) == "Polygon":
            rings = polygon_rings(node)
            if rings:
                surfaces.append(rings)
    return surfaces


def transform_surfaces(surfaces, transformer: Transformer):
    return [
        [[(*transformer.transform(point[0], point[1]), point[2]) for point in ring] for ring in surface]
        for surface in surfaces
    ]


def building_id(element: ET.Element) -> str:
    for key, value in element.attrib.items():
        if local_name(key) == "id":
            return value
    return "unknown"


def nearest_city_reference(target: dict, city: dict, to_utm: Transformer) -> dict:
    names = {normalise(name) for name in target.get("osm_names", [])}
    fallback_e, fallback_n = to_utm.transform(target["longitude"], target["latitude"])
    origin_e, origin_n, _ = city["origin_projected_m"]
    candidates = []
    for feature in city.get("features", []):
        if feature.get("kind") != "building" or not feature.get("rings"):
            continue
        ring = feature["rings"][0]
        if not ring:
            continue
        x = sum(point[0] for point in ring) / len(ring)
        y = sum(point[1] for point in ring) / len(ring)
        e = origin_e + x / 100.0
        n = origin_n - y / 100.0
        feature_name = normalise(feature.get("tags", {}).get("name", "") or feature.get("name", ""))
        exact = feature_name in names if feature_name else False
        name_related = bool(feature_name and any(name in feature_name or feature_name in name for name in names))
        distance = math.hypot(e - fallback_e, n - fallback_n)
        score = (0 if exact else (1 if name_related else 2), distance)
        candidates.append((score, feature, e, n, max(point[2] for point in ring)))
    if not candidates:
        raise RuntimeError("City data contains no building references")
    score, feature, e, n, base_z = min(candidates, key=lambda item: item[0])
    if score[0] == 2 and score[1] > target["radius_m"]:
        raise RuntimeError(f"No OSM reference building near {target['display_name']} ({score[1]:.1f} m)")
    return {
        "feature_id": feature["id"],
        "easting": e,
        "northing": n,
        "base_z_cm": base_z,
        "match_distance_m": score[1],
        "name_match": score[0] < 2,
    }


def find_citygml_candidates(paths: list[Path], targets: list[dict]) -> dict[str, dict]:
    best: dict[str, dict] = {}
    for path in paths:
        crs = detect_crs(path)
        to_utm = Transformer.from_crs(crs, "EPSG:32633", always_xy=True)
        for _, element in ET.iterparse(path, events=("end",)):
            if local_name(element.tag) != "Building":
                continue
            surfaces = building_surfaces(element)
            if not surfaces:
                element.clear()
                continue
            points = [point for surface in surfaces for ring in surface for point in ring]
            cx = sum(point[0] for point in points) / len(points)
            cy = sum(point[1] for point in points) / len(points)
            ce, cn = to_utm.transform(cx, cy)
            for target in targets:
                distance = math.hypot(ce - target["reference"]["easting"], cn - target["reference"]["northing"])
                if distance > target["radius_m"]:
                    continue
                existing = best.get(target["id"])
                if existing is None or distance < existing["distance_m"]:
                    best[target["id"]] = {
                        "distance_m": distance,
                        "gml_id": building_id(element),
                        "source_file": path.name,
                        "source_crs": crs,
                        "centroid_utm": [ce, cn],
                        "surfaces": transform_surfaces(surfaces, to_utm),
                    }
            element.clear()
    return best


def newell(points):
    nx = ny = nz = 0.0
    for current, following in zip(points, points[1:] + points[:1]):
        nx += (current[1] - following[1]) * (current[2] + following[2])
        ny += (current[2] - following[2]) * (current[0] + following[0])
        nz += (current[0] - following[0]) * (current[1] + following[1])
    return nx, ny, nz


def projected(point, dropped: int):
    if dropped == 0:
        return point[1], point[2]
    if dropped == 1:
        return point[0], point[2]
    return point[0], point[1]


def lifted(uv, dropped: int, plane_point, normal):
    x0, y0, z0 = plane_point
    nx, ny, nz = normal
    u, v = uv
    if dropped == 0:
        y, z = u, v
        x = x0 - (ny * (y - y0) + nz * (z - z0)) / nx
        return x, y, z
    if dropped == 1:
        x, z = u, v
        y = y0 - (nx * (x - x0) + nz * (z - z0)) / ny
        return x, y, z
    x, y = u, v
    z = z0 - (nx * (x - x0) + ny * (y - y0)) / nz
    return x, y, z


def triangulated_surface(rings):
    exterior = rings[0]
    normal = newell(exterior)
    dropped = max(range(3), key=lambda index: abs(normal[index]))
    if abs(normal[dropped]) < 1e-9:
        return []
    exterior_2d = [projected(point, dropped) for point in exterior]
    holes_2d = [[projected(point, dropped) for point in ring] for ring in rings[1:]]
    polygon = make_valid(Polygon(exterior_2d, holes_2d))
    polygons = [polygon] if polygon.geom_type == "Polygon" else [g for g in getattr(polygon, "geoms", []) if g.geom_type == "Polygon"]
    result = []
    for component in polygons:
        for triangle in triangulate(component):
            if not component.covers(triangle):
                continue
            coords = list(triangle.exterior.coords)[:3]
            result.append([lifted(coord, dropped, exterior[0], normal) for coord in coords])
    return result


def mesh_record(target: dict, candidate: dict, city_origin: list[float]) -> dict:
    reference = target["reference"]
    shift_e = reference["easting"] - candidate["centroid_utm"][0]
    shift_n = reference["northing"] - candidate["centroid_utm"][1]
    source_z = [point[2] for surface in candidate["surfaces"] for ring in surface for point in ring]
    min_z = min(source_z)
    origin_e, origin_n, _ = city_origin
    world_triangles = []
    for surface in candidate["surfaces"]:
        shifted = [[(p[0] + shift_e, p[1] + shift_n, p[2]) for p in ring] for ring in surface]
        for triangle in triangulated_surface(shifted):
            world_triangles.append([
                (
                    (e - origin_e) * 100.0,
                    (origin_n - n) * 100.0,
                    reference["base_z_cm"] + (z - min_z) * 100.0,
                )
                for e, n, z in triangle
            ])
    if not world_triangles:
        raise RuntimeError(f"No triangulatable surfaces for {target['display_name']}")
    all_points = [point for tri in world_triangles for point in tri]
    center_x = sum(point[0] for point in all_points) / len(all_points)
    center_y = sum(point[1] for point in all_points) / len(all_points)
    origin = [math.floor(center_x / 12800.0) * 12800.0, math.floor(center_y / 12800.0) * 12800.0, 0.0]
    vertices = []
    triangles = []
    for tri in world_triangles:
        base = len(vertices)
        vertices.extend([[round(point[i] - origin[i], 3) for i in range(3)] for point in tri])
        triangles.extend([base, base + 2, base + 1])
    return {
        "kind": "official_building",
        "material": target.get("material", "Brick"),
        "origin": [round(v, 3) for v in origin],
        "vertices": vertices,
        "triangles": triangles,
    }


def run(config_path: Path, city_data: Path, output: Path, archive: Path | None, require_all: bool) -> dict:
    config = json.loads(config_path.read_text(encoding="utf-8"))
    city = json.loads((city_data / "sector.json").read_text(encoding="utf-8"))
    if config.get("schema_version") != 1 or city.get("projected_crs") != "EPSG:32633":
        raise RuntimeError("Unsupported landmark/city schema")
    to_utm = Transformer.from_crs("EPSG:4326", "EPSG:32633", always_xy=True)
    targets = []
    for raw in config["targets"]:
        target = dict(raw)
        target["reference"] = nearest_city_reference(target, city, to_utm)
        targets.append(target)

    output.mkdir(parents=True, exist_ok=True)
    if archive is None:
        click_lon = sum(t["longitude"] for t in targets) / len(targets)
        click_lat = sum(t["latitude"] for t in targets) / len(targets)
        archive, download_url = download_source(output, config.get("preferred_lod", "LoD1"), click_lon, click_lat, config.get("source_direct_url"))
    else:
        archive = archive.resolve()
        if not archive.is_file():
            raise RuntimeError(f"Archive not found: {archive}")
        download_url = f"file://{archive}"
    paths = extract_gml(archive, output)
    candidates = find_citygml_candidates(paths, targets)
    missing = [target["id"] for target in targets if target["id"] not in candidates]
    if require_all and missing:
        raise RuntimeError("Missing configured landmarks in CityGML: " + ", ".join(missing))

    mesh_dir = output / "Meshes"
    if mesh_dir.exists():
        shutil.rmtree(mesh_dir)
    mesh_dir.mkdir(parents=True)
    mesh_files = []
    records = []
    for target in targets:
        candidate = candidates.get(target["id"])
        if candidate is None:
            records.append({"id": target["id"], "display_name": target["display_name"], "status": "missing"})
            continue
        record = mesh_record(target, candidate, city["origin_projected_m"])
        filename = target["id"] + ".json"
        (mesh_dir / filename).write_text(json.dumps(record, separators=(",", ":")) + "\n", encoding="utf-8")
        mesh_files.append(filename)
        records.append({
            "id": target["id"],
            "display_name": target["display_name"],
            "status": "generated",
            "gml_id": candidate["gml_id"],
            "source_file": candidate["source_file"],
            "source_crs": candidate["source_crs"],
            "distance_to_osm_reference_m": round(candidate["distance_m"], 3),
            "replaced_feature_id": target["reference"]["feature_id"],
            "osm_reference_name_match": target["reference"]["name_match"],
            "material": target.get("material", "Brick"),
            "mesh": filename,
        })
    (mesh_dir / "meshes.json").write_text(json.dumps(mesh_files) + "\n", encoding="utf-8")
    catalog = {
        "schema_version": 1,
        "source": config["source"],
        "source_service": config["source_service"],
        "source_info": config["source_info"],
        "preferred_lod": config.get("preferred_lod", "LoD2"),
        "download_url": download_url,
        "archive_sha256": sha256(archive),
        "city_origin_projected_m": city["origin_projected_m"],
        "vertical_alignment": "CityGML relative height aligned to the existing GIS/OSM building ground Z; source vertical datum is not mixed directly.",
        "buildings": records,
    }
    (output / "catalog.json").write_text(json.dumps(catalog, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return catalog


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", type=Path, default=DEFAULT_CONFIG)
    parser.add_argument("--city-data", type=Path, default=DEFAULT_CITY_DATA)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--archive", type=Path, help="Use a previously downloaded GUGiK ZIP/GML instead of network download")
    parser.add_argument("--require-all", action="store_true")
    args = parser.parse_args()
    catalog = run(args.config, args.city_data, args.output, args.archive, args.require_all)
    generated = sum(item["status"] == "generated" for item in catalog["buildings"])
    print(f"Official Wrocław 3D buildings generated: {generated}/{len(catalog['buildings'])}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, RuntimeError, ET.ParseError) as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        raise SystemExit(1)
