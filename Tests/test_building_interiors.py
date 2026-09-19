import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/generate_building_interiors.py";S=importlib.util.spec_from_file_location("i",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def test_links_real_entrance_and_builds_nav(self):
  cfg=json.loads((R/"Data/interior_generation_profiles.json").read_text());city={"features":[{"id":"b","kind":"building","architecture_profile":"tenement","height_m":12,"rings":[[[0,0,0],[2000,0,0],[2000,2000,0],[0,2000,0],[0,0,0]]],"tags":{"building":"apartments","building:levels":"3"}}]};ent={"entrances":[{"id":"e","building_id":"b"}]};r=M.generate(cfg,city,ent)["interiors"][0]
  self.assertEqual(r["floors"],3);self.assertEqual(r["entrance_ids"],["e"]);self.assertEqual(r["classification"],"private");self.assertFalse(r["factual_layout"]);self.assertGreater(len(r["nav_edges"]),0)
  self.assertTrue(all(room["polygon_xy"] and room["area_m2"]>0 for room in r["rooms"]))
 def test_public_classification(self):
  cfg=json.loads((R/"Data/interior_generation_profiles.json").read_text());self.assertEqual(M.classification({"amenity":"library"},"x",cfg),"public")
if __name__=="__main__":unittest.main()
