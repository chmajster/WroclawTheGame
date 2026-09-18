import importlib.util
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MODULE_PATH = ROOT / "Scripts" / "gis" / "resolve_landmark_catalog.py"
SPEC = importlib.util.spec_from_file_location("resolve_landmark_catalog", MODULE_PATH)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class LandmarkCatalogTests(unittest.TestCase):
    def test_catalog_is_large_unique_and_classified(self):
        data = json.loads((ROOT / "Data" / "wroclaw_landmark_catalog.json").read_text(encoding="utf-8"))
        ids = [r["id"] for r in data["records"]]
        self.assertGreaterEqual(len(ids), 30)
        self.assertEqual(len(ids), len(set(ids)))
        self.assertTrue(all(r["quality"] in {"hero","detailed"} for r in data["records"]))
        required = {"old_town_hall","centennial_hall","main_station","sky_tower","stadium_wroclaw"}
        self.assertTrue(required <= set(ids))

    def test_name_normalisation_handles_polish_diacritics(self):
        self.assertEqual(MODULE.normalise("Wrocławski Ratusz"), "wroclawski ratusz")

    def test_resolution_is_name_based_not_guessed(self):
        catalog={"records":[{"id":"x","name":"Hala Targowa","osm_names":["Hala Targowa"],"quality":"hero"}]}
        city={"features":[
            {"id":"way/1","kind":"building","name":"","tags":{"name":"Hala Targowa"},"height_m":20},
            {"id":"way/2","kind":"building","name":"","tags":{"name":"Inny budynek"},"height_m":40},
        ]}
        result=MODULE.resolve(catalog,city)
        self.assertEqual(result["counts"]["resolved"],1)
        self.assertEqual(result["records"][0]["feature_id"],"way/1")


if __name__ == "__main__":
    unittest.main()
