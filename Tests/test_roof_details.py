import importlib.util,json,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
P=ROOT/"Scripts"/"gis"/"generate_roof_details.py"
S=importlib.util.spec_from_file_location("roofs",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class RoofTests(unittest.TestCase):
 def setUp(self):self.cfg=json.loads((ROOT/"Data"/"roof_detail_profiles.json").read_text())
 def test_osm_shape_wins(self):
  city={"features":[{"id":"b","kind":"building","architecture_profile":"tenement","height_m":15,"rings":[[[0,0,0],[1000,0,0],[1000,1000,0],[0,1000,0],[0,0,0]]],"tags":{"roof:shape":"mansard","roof:height":"4"}}]}
  r=M.generate(self.cfg,city)["buildings"][0]
  self.assertEqual(r["roof"]["shape"],"mansard");self.assertEqual(r["roof"]["shape_source"],"osm");self.assertEqual(r["roof"]["height_m"],4)
 def test_fallback_is_deterministic(self):
  city={"features":[{"id":"b","kind":"building","architecture_profile":"villa","height_m":10,"rings":[[[0,0,0],[1200,0,0],[1200,800,0],[0,800,0],[0,0,0]]],"tags":{}}]}
  a=M.generate(self.cfg,city);b=M.generate(self.cfg,city);self.assertEqual(a,b)
 def test_estate_can_use_flat_roof(self):
  city={"features":[{"id":"e","kind":"building","architecture_profile":"estate","height_m":20,"rings":[[[0,0,0],[2000,0,0],[2000,1000,0],[0,1000,0],[0,0,0]]],"tags":{}}]}
  self.assertEqual(M.generate(self.cfg,city)["buildings"][0]["roof"]["shape"],"flat")
if __name__=="__main__":unittest.main()
