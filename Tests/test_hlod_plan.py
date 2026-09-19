import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/build_hlod_plan.py";S=importlib.util.spec_from_file_location("h",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def setUp(self):self.c=json.loads((R/"Data/building_hlod_profiles.json").read_text())
 def test_hero_has_larger_transition_and_less_aggressive_reduction(self):
  self.assertGreater(self.c["profiles"]["hero"]["transition_m"],self.c["profiles"]["background"]["transition_m"]);self.assertGreater(self.c["profiles"]["hero"]["target_triangle_ratio"],self.c["profiles"]["background"]["target_triangle_ratio"])
 def test_assignment_has_budget_layer_and_stable_rebuild_key(self):
  q={"decisions":[{"building_id":"b","quality":"hero","triangles":100000,"materials":12}]};dl={"assignments":[{"id":"b","layer":"DL_Buildings"}]};a=M.generate(self.c,q,dl);b=M.generate(self.c,q,dl);x=a["assignments"][0];self.assertEqual(x["layer"],"HLOD_Hero");self.assertEqual(x["data_layer"],"DL_Buildings");self.assertEqual(x["estimated_triangles"],35000);self.assertLessEqual(x["estimated_materials"],self.c["max_materials_per_proxy"]);self.assertEqual(x["rebuild_key"],b["assignments"][0]["rebuild_key"]);self.assertTrue(x["silhouette_protected"])
 def test_background_uses_small_budget(self):
  r=M.generate(self.c,{"decisions":[{"building_id":"b","quality":"background","triangles":10000,"materials":2}]});self.assertLess(r["assignments"][0]["memory_budget_mb"],self.c["profiles"]["hero"]["memory_budget_mb"])
if __name__=="__main__":unittest.main()
