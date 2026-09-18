import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MODULE_PATH = ROOT / "Scripts" / "gis" / "fetch_gugik_mesh_landmarks.py"
SPEC = importlib.util.spec_from_file_location("gugik_mesh_landmarks", MODULE_PATH)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class GUGiKMeshLandmarkTests(unittest.TestCase):
    def test_catalog_is_unique_and_in_wroclaw(self):
        data = json.loads((ROOT / "Data" / "gugik_mesh_landmarks.json").read_text(encoding="utf-8"))
        self.assertEqual(data["source_crs"], "EPSG:2177")
        self.assertIn("ModeleSiatkowe3D", data["service_url"])
        ids = [item["id"] for item in data["targets"]]
        self.assertEqual(len(ids), len(set(ids)))
        self.assertGreaterEqual(len(ids), 10)
        for item in data["targets"]:
            self.assertTrue(16.9 < item["longitude"] < 17.2)
            self.assertTrue(51.0 < item["latitude"] < 51.2)
            self.assertEqual(item["quality"], "hero")

    def test_obj_crop_preserves_faces_and_uv_indices(self):
        obj = """mtllib test.mtl
v 1000 1000 10
v 1010 1000 10
v 1010 1010 10
v 1000 1010 10
v 2000 2000 10
vt 0 0
vt 1 0
vt 1 1
vt 0 1
usemtl facade
f 1/1 2/2 3/3 4/4
f 2/1 5/2 3/3
"""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "source.obj"
            output = root / "crop.obj"
            source.write_text(obj, encoding="utf-8")
            stats = MODULE.crop_obj(source, output, 1005, 1005, 20)
            text = output.read_text(encoding="utf-8")
            self.assertEqual(stats["faces"], 1)
            self.assertEqual(stats["vertices"], 4)
            self.assertIn("usemtl facade", text)
            self.assertIn("f 1/1 2/2 3/3 4/4", text)

    def test_candidate_manifest_requires_visual_qa(self):
        target = {"id": "hall", "name": "Hall", "quality": "hero"}
        manifest = MODULE.candidate_manifest(target, Path("Saved/test.obj"))
        self.assertEqual(manifest["quality"], "hero")
        self.assertTrue(manifest["unreal"]["nanite"])
        self.assertGreaterEqual(len(manifest["qa"]["cameras"]), 4)
        self.assertEqual(manifest["source"]["model"], "Saved/test.obj")


if __name__ == "__main__":
    unittest.main()
