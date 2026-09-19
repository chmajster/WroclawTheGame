import importlib.util,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/validate_multidistrict_quests.py";S=importlib.util.spec_from_file_location("q",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def city(self):return {"sectors":[{"id":"a","district":"A"},{"id":"b","district":"B"},{"id":"c","district":"C"}],"links":[{"from":"a","to":"b","foot":True,"car":False},{"from":"b","to":"c","foot":True,"car":True}]}
 def test_connected_quest_compiles_route_and_objectives(self):
  quests={"quests":[{"id":"q","stages":[{"id":"s1","district":"A"},{"id":"s2","district":"C"}]}]};r=M.validate(quests,self.city());self.assertEqual(r["status"],"PASS");q=r["compiled_quests"][0];self.assertEqual(q["route_legs"][0]["district_path"],["A","B","C"]);self.assertEqual(q["stages"][0]["objective_id"],"q:s1");self.assertTrue(q["stages"][0]["gps_enabled"])
 def test_unreachable_fails(self):
  city={"sectors":[{"id":"a","district":"A"},{"id":"b","district":"B"}],"links":[]};quests={"quests":[{"id":"q","stages":[{"id":"a","district":"A"},{"id":"b","district":"B"}]}]};self.assertEqual(M.validate(quests,city)["status"],"FAIL")
 def test_optional_poi_binding_is_validated(self):
  quests={"quests":[{"id":"q","stages":[{"id":"s1","district":"A","target":{"type":"poi","id":"poi/1"}},{"id":"s2","district":"B"}]}]};pois={"pois":[{"id":"poi/1"}]};self.assertEqual(M.validate(quests,self.city(),pois=pois)["status"],"PASS")
 def test_save_state(self):self.assertEqual(M.save_state("q","s",["door.open"])["stage_id"],"s")
if __name__=="__main__":unittest.main()
