import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/build_data_layer_plan.py";S=importlib.util.spec_from_file_location("d",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def setUp(self):self.c=json.loads((R/"Data/city_data_layers.json").read_text())
 def test_base_and_building_layers(self):
  self.assertEqual(M.lookup(self.c,kind="road"),"DL_BaseGeometry");self.assertEqual(M.lookup(self.c,kind="building"),"DL_Buildings");self.assertEqual(M.lookup(self.c,system="interior"),"DL_Interiors")
 def test_dependency_closure(self):
  self.assertEqual(M.validate_dependencies(self.c),[]);closure=M.activation_closure(self.c,["DL_Interiors"]);self.assertEqual(closure,["DL_BaseGeometry","DL_Buildings","DL_Interiors"])
 def test_role_and_system_assignments(self):
  city={"features":[{"id":"road/1","kind":"road"}]};r=M.generate(self.c,city,[{"id":"lamp/1","role":"street_lamp"}],[{"id":"npc/1","system":"crowd"}]);layers={x["id"]:x["layer"] for x in r["assignments"]};self.assertEqual(layers["lamp/1"],"DL_StreetFurniture");self.assertEqual(layers["npc/1"],"DL_Crowd");self.assertEqual(r["status"],"PASS")
 def test_cycle_is_rejected(self):
  c=json.loads(json.dumps(self.c));next(x for x in c["layers"] if x["id"]=="DL_BaseGeometry")["depends_on"]=["DL_Buildings"];self.assertIn("dependency_cycle",{x["code"] for x in M.validate_dependencies(c)})
if __name__=="__main__":unittest.main()
