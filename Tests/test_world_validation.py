import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/validate_world.py";S=importlib.util.spec_from_file_location("v",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def setUp(self):self.p=json.loads((R/"Data/world_validation_policy.json").read_text())
 def base(self):return {"features":[{"id":"b","kind":"building","rings":[[[0,0,0],[100,0,0],[100,100,0],[0,0,0]]]}],"road_graph":{"nodes":[{"id":"a"},{"id":"z"}],"edges":[{"id":"e","from":"a","to":"z","length_cm":100}]}}
 def test_valid_minimal_world_passes(self):
  city={"sectors":[{"id":"s","district":"d"}]};self.assertEqual(M.validate(self.p,self.base(),city)["status"],"PASS")
 def test_orphan_entrance_fails(self):
  city={"sectors":[{"id":"s","district":"d"}]};r=M.validate(self.p,self.base(),city,{"entrances":[{"id":"x","building_id":"missing"}]});self.assertEqual(r["status"],"FAIL");self.assertTrue(any(x["code"]=="orphan_entrance" for x in r["issues"]))
 def test_unresolved_hero_fails(self):
  city={"sectors":[{"id":"s","district":"d"}]};r=M.validate(self.p,self.base(),city,landmarks={"records":[{"id":"h","quality":"hero","status":"unresolved"}]});self.assertEqual(r["status"],"FAIL")
if __name__=="__main__":unittest.main()
