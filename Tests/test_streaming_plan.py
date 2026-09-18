import importlib.util,json,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];P=ROOT/"Scripts"/"gis"/"build_streaming_plan.py";S=importlib.util.spec_from_file_location("streaming",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class StreamingTests(unittest.TestCase):
 def setUp(self):self.cfg=json.loads((ROOT/"Data"/"city_streaming_budgets.json").read_text())
 def test_inner_ring_has_higher_budget(self):
  self.assertGreater(self.cfg["rings"]["1"]["max_estimated_visible_triangles"],self.cfg["rings"]["3"]["max_estimated_visible_triangles"])
  self.assertGreater(self.cfg["rings"]["1"]["full_detail_radius_m"],self.cfg["rings"]["3"]["full_detail_radius_m"])
 def test_fallback_sector_plan(self):
  city={"sectors":[{"id":"s","district":"d","ring":2,"counts":{"buildings":100}}]}
  r=M.plan(self.cfg,city);self.assertEqual(r["sectors"][0]["count_source"],"sector_building_count_fallback")
 def test_quality_over_budget_is_reported(self):
  city={"sectors":[{"id":"s","district":"d","ring":3,"counts":{"buildings":1}}]}
  quality={"decisions":[{"building_id":f"b{i}","sector":"s","quality":"hero","selected":{}} for i in range(20)]}
  r=M.plan(self.cfg,city,quality);self.assertEqual(r["status"],"REQUIRES_TUNING");self.assertTrue(any(x["code"]=="hero_count" for x in r["issues"]))
if __name__=="__main__":unittest.main()
