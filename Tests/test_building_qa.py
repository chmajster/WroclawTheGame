import importlib.util,json,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];P=ROOT/"Scripts"/"gis"/"qa_buildings.py";S=importlib.util.spec_from_file_location("qa",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class BuildingQATests(unittest.TestCase):
 def setUp(self):self.p=json.loads((ROOT/"Data"/"building_qa_policy.json").read_text())
 def test_duplicate_replacement_fails(self):
  o={"buildings":[{"id":"a","replaced_feature_id":"x","status":"generated"},{"id":"b","replaced_feature_id":"x","status":"generated"}]}
  self.assertEqual(M.qa(self.p,official=o)["status"],"FAIL")
 def test_hero_without_pass_fails(self):
  q={"decisions":[{"building_id":"b","selected_source":"textured_mesh","quality":"hero","selected":{"qa_status":"FAIL"}}]}
  r=M.qa(self.p,quality=q);self.assertEqual(r["status"],"FAIL");self.assertTrue(any(i["code"]=="hero_without_visual_qa" for i in r["issues"]))
 def test_valid_records_pass(self):
  o={"buildings":[{"id":"a","replaced_feature_id":"x","status":"generated","distance_to_osm_reference_m":2,"footprint_overlap_ratio":0.8}]}
  q={"decisions":[{"building_id":"x","selected_source":"lod1","quality":"standard","selected":{}}]}
  self.assertEqual(M.qa(self.p,o,q)["status"],"PASS")
if __name__=="__main__":unittest.main()
