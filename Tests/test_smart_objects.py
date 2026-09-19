import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/build_smart_objects.py";S=importlib.util.spec_from_file_location("s",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def test_roles(self):
  self.assertEqual(M.role_for({"entrance":"yes"}),"building_entrance");self.assertEqual(M.role_for({"shop":"bakery"}),"shop");self.assertEqual(M.role_for({"amenity":"parking"}),"parking")
 def test_spatial_slots_and_capacity(self):
  cfg=json.loads((R/"Data/smart_object_roles.json").read_text());city={"features":[{"id":"n/1","kind":"entrance","points":[[100,200,0]],"tags":{"shop":"bakery","wheelchair":"yes"}}]}
  r=M.generate(cfg,city);obj=r["objects"][0];self.assertEqual(r["schema_version"],2);self.assertEqual(obj["capacity"],3);self.assertEqual(len(obj["slots"]),3);self.assertEqual(obj["accessibility"],"yes")
  self.assertTrue(all(s["id"].startswith("smart:n/1/slot/") for s in obj["slots"]));self.assertGreater(r["slot_count"],0)
if __name__=="__main__":unittest.main()
