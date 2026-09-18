import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MODULE_PATH = ROOT / "Scripts" / "gis" / "building_quality_resolver.py"
SPEC = importlib.util.spec_from_file_location("building_quality_resolver", MODULE_PATH)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class BuildingQualityResolverTests(unittest.TestCase):
    def setUp(self):
        self.policy = json.loads((ROOT / "Data" / "building_quality_policy.json").read_text(encoding="utf-8"))

    def test_passed_textured_mesh_wins(self):
        result = MODULE.resolve(self.policy, {
            "textured_mesh": [{"building_id":"b1","id":"mesh","mesh":"mesh.obj","textures":["a.jpg"],"qa_status":"PASS"}],
            "lod2": [{"building_id":"b1","id":"lod2","mesh":"lod2.json"}],
            "lod1": [{"building_id":"b1","id":"lod1","mesh":"lod1.json"}],
            "osm": [{"building_id":"b1","id":"osm","feature_id":"way/1"}],
        })
        self.assertEqual(result["decisions"][0]["selected_source"], "textured_mesh")

    def test_failed_mesh_falls_back_to_lod2(self):
        result = MODULE.resolve(self.policy, {
            "textured_mesh": [{"building_id":"b1","id":"mesh","mesh":"mesh.obj","textures":["a.jpg"],"qa_status":"FAIL"}],
            "lod2": [{"building_id":"b1","id":"lod2","mesh":"lod2.json"}],
            "lod1": [],
            "osm": [{"building_id":"b1","id":"osm","feature_id":"way/1"}],
        })
        decision = result["decisions"][0]
        self.assertEqual(decision["selected_source"], "lod2")
        self.assertTrue(any("qa_status" in item["reason"] for item in decision["audit"]))

    def test_osm_is_final_fallback(self):
        result = MODULE.resolve(self.policy, {
            "textured_mesh": [],
            "lod2": [],
            "lod1": [],
            "osm": [{"building_id":"b1","id":"osm","feature_id":"way/1"}],
        })
        self.assertEqual(result["decisions"][0]["selected_source"], "osm")

    def test_no_silent_missing(self):
        result = MODULE.resolve(self.policy, {
            "textured_mesh": [{"building_id":"b1","id":"bad","status":"invalid","mesh":"bad.obj","textures":["a.jpg"],"qa_status":"PASS"}],
            "lod2": [], "lod1": [], "osm": []
        })
        self.assertIsNone(result["decisions"][0]["selected_source"])
        self.assertEqual(result["counts"]["unresolved"], 1)


if __name__ == "__main__":
    unittest.main()
