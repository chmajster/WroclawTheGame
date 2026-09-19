import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/generate_city_dressing.py";S=importlib.util.spec_from_file_location("d",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def test_deterministic_and_runtime_ready(self):
  cfg=json.loads((R/"Data/city_dressing_rules.json").read_text());city={"features":[{"id":"r","kind":"road","points":[[0,0,0],[10000,0,0]],"tags":{"highway":"residential"}}]}
  a=M.generate(cfg,city);b=M.generate(cfg,city);self.assertEqual(a,b);self.assertGreater(a["count"],0);self.assertEqual(a["schema_version"],2)
  self.assertTrue(all(x["procedural"] for x in a["placements"]))
  self.assertTrue(all("variant" in x and "rotation_yaw_deg" in x and "uniform_scale" in x for x in a["placements"]))
  self.assertTrue(all(x["data_layer"]=="StreetFurniture" for x in a["placements"]))
 def test_road_orientation_and_variants_are_stable(self):
  cfg=json.loads((R/"Data/city_dressing_rules.json").read_text());city={"features":[{"id":"r","kind":"road","points":[[0,0,0],[0,10000,0]],"tags":{"highway":"residential"}}]}
  r=M.generate(cfg,city);lamps=[x for x in r["placements"] if x["role"]=="street_lamp"];self.assertGreater(len(lamps),0)
  self.assertTrue(all(x["rotation_yaw_deg"] in {0.0,180.0} for x in lamps));self.assertTrue(all(x["variant"] in {"street_lamp_modern","street_lamp_classic"} for x in lamps))
if __name__=="__main__":unittest.main()
