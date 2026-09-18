import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MODULE_PATH = ROOT / "Scripts" / "gis" / "fetch_official_buildings_3d.py"
SPEC = importlib.util.spec_from_file_location("official_buildings_3d", MODULE_PATH)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class OfficialBuildingImporterTests(unittest.TestCase):
    def test_prefers_lod2_layer_and_download(self):
        records = [
            {"name": "buildings_2024", "title": "Budynki LoD1 - 2024", "queryable": True},
            {"name": "buildings_lod2", "title": "Budynki LoD2", "queryable": True},
        ]
        self.assertEqual(MODULE.select_layer(records, "LoD2")["name"], "buildings_lod2")
        links = [
            ("https://example.test/wroclaw_lod1_2024.zip", "LoD1 2024"),
            ("https://example.test/wroclaw_lod2.zip", "LoD2"),
        ]
        self.assertEqual(
            MODULE.choose_download_link(links, "LoD2"),
            "https://example.test/wroclaw_lod2.zip",
        )

    def test_citygml_building_becomes_game_mesh(self):
        e, n = 641700.0, 5665700.0

        def ring(points):
            values = " ".join(str(v) for point in points for v in point)
            return (
                '<gml:Polygon srsName="EPSG:32633">'
                '<gml:exterior><gml:LinearRing>'
                f'<gml:posList srsDimension="3">{values}</gml:posList>'
                '</gml:LinearRing></gml:exterior></gml:Polygon>'
            )

        surfaces = [
            [(e,n,120),(e+10,n,120),(e+10,n+10,120),(e,n+10,120),(e,n,120)],
            [(e,n,140),(e,n+10,140),(e+10,n+10,140),(e+10,n,140),(e,n,140)],
            [(e,n,120),(e,n,140),(e+10,n,140),(e+10,n,120),(e,n,120)],
            [(e+10,n,120),(e+10,n,140),(e+10,n+10,140),(e+10,n+10,120),(e+10,n,120)],
            [(e+10,n+10,120),(e+10,n+10,140),(e,n+10,140),(e,n+10,120),(e+10,n+10,120)],
            [(e,n+10,120),(e,n+10,140),(e,n,140),(e,n,120),(e,n+10,120)],
        ]
        xml = (
            '<?xml version="1.0" encoding="UTF-8"?>'
            '<core:CityModel xmlns:core="http://www.opengis.net/citygml/2.0" '
            'xmlns:gml="http://www.opengis.net/gml" '
            'xmlns:bldg="http://www.opengis.net/citygml/building/2.0">'
            '<gml:boundedBy><gml:Envelope srsName="EPSG:32633"/></gml:boundedBy>'
            '<core:cityObjectMember><bldg:Building gml:id="building-test">'
            + "".join(surfaces and [ring(points) for points in surfaces]) +
            '</bldg:Building></core:cityObjectMember></core:CityModel>'
        )

        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "test.gml"
            path.write_text(xml, encoding="utf-8")
            target = {
                "id": "test",
                "display_name": "Test building",
                "radius_m": 30,
                "material": "Brick",
                "reference": {
                    "easting": e + 5,
                    "northing": n + 5,
                    "base_z_cm": 250,
                    "feature_id": "way/1:0",
                    "name_match": True,
                },
            }
            candidates = MODULE.find_citygml_candidates([path], [target])
            self.assertIn("test", candidates)
            self.assertEqual(candidates["test"]["gml_id"], "building-test")
            mesh = MODULE.mesh_record(target, candidates["test"], [641702, 5665787, 115.0])
            self.assertEqual(mesh["kind"], "official_building")
            self.assertGreaterEqual(len(mesh["triangles"]), 36)
            self.assertEqual(len(mesh["triangles"]) % 3, 0)
            self.assertGreater(len(mesh["vertices"]), 0)

    def test_full_build_and_editor_are_wired(self):
        build = (ROOT / "Scripts" / "Build-FullGame.ps1").read_text(encoding="utf-8")
        geography = (ROOT / "Scripts" / "Build-Geography.ps1").read_text(encoding="utf-8")
        editor = (ROOT / "Scripts" / "prepare_geography.py").read_text(encoding="utf-8")
        city = (ROOT / "Scripts" / "gis" / "build_city.py").read_text(encoding="utf-8")
        meshes = (ROOT / "Scripts" / "gis" / "build_meshes.py").read_text(encoding="utf-8")
        self.assertIn("OfficialBuildings=$true", build)
        self.assertIn("fetch_official_buildings_3d.py", geography)
        self.assertIn("WTG_OFFICIAL_BUILDINGS_INPUT", geography)
        self.assertIn("/Game/Generated/OfficialBuildings/", editor)
        self.assertIn("--official-catalog", city)
        self.assertIn("exclude_feature_ids", meshes)

    def test_landmark_catalog_has_unique_real_targets(self):
        data = json.loads((ROOT / "Data" / "wroclaw_landmarks_3d.json").read_text(encoding="utf-8"))
        self.assertEqual(data["schema_version"], 1)
        self.assertEqual(data["preferred_lod"], "LoD2")
        ids = [item["id"] for item in data["targets"]]
        self.assertEqual(len(ids), len(set(ids)))
        self.assertGreaterEqual(len(ids), 6)
        for item in data["targets"]:
            self.assertTrue(16.9 < item["longitude"] < 17.2)
            self.assertTrue(51.0 < item["latitude"] < 51.2)
            self.assertGreater(item["radius_m"], 0)


if __name__ == "__main__":
    unittest.main()
