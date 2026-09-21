import importlib.util,json,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
P=ROOT/"Scripts"/"gis"/"generate_roof_details.py"
S=importlib.util.spec_from_file_location("roofs",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class RoofTests(unittest.TestCase):
 def setUp(self):
  self.cfg=json.loads((ROOT/"Data"/"roof_detail_profiles.json").read_text())
  self.bindings=json.loads((ROOT/"Data"/"roof_asset_bindings.json").read_text())
 def test_osm_shape_wins(self):
  city={"features":[{"id":"b","kind":"building","architecture_profile":"tenement","height_m":15,"rings":[[[0,0,0],[1000,0,0],[1000,1000,0],[0,1000,0],[0,0,0]]],"tags":{"roof:shape":"mansard","roof:height":"4"}}]}
  r=M.generate(self.cfg,city,self.bindings)["buildings"][0]
  self.assertEqual(r["roof"]["shape"],"mansard");self.assertEqual(r["roof"]["shape_source"],"osm");self.assertEqual(r["roof"]["height_m"],4)
 def test_fallback_is_deterministic(self):
  city={"features":[{"id":"b","kind":"building","architecture_profile":"villa","height_m":10,"rings":[[[0,0,0],[1200,0,0],[1200,800,0],[0,800,0],[0,0,0]]],"tags":{}}]}
  a=M.generate(self.cfg,city);b=M.generate(self.cfg,city);self.assertEqual(a,b)
 def test_estate_can_use_flat_roof(self):
  city={"features":[{"id":"e","kind":"building","architecture_profile":"estate","height_m":20,"rings":[[[0,0,0],[2000,0,0],[2000,1000,0],[0,1000,0],[0,0,0]]],"tags":{}}]}
  self.assertEqual(M.generate(self.cfg,city)["buildings"][0]["roof"]["shape"],"flat")

 def test_every_supported_shape_has_catalogued_mesh(self):
  external={item["id"] for item in json.loads((ROOT/"Data"/"free_external_model_catalog.json").read_text())}
  self.assertEqual(set(self.bindings["shapes"]),{"flat","gabled","hipped","pyramidal","mansard","dome","onion"})
  self.assertFalse(set(self.bindings["shapes"].values())-external)
  self.assertFalse(set(self.bindings["details"].values())-external)

 def test_generated_roof_contains_mesh_transform_and_detail_instances(self):
  city={"features":[{"id":"b","kind":"building","architecture_profile":"tenement","height_m":15,"rings":[[[0,0,10],[1600,0,10],[1600,1000,10],[0,1000,10],[0,0,10]]],"tags":{"roof:shape":"gabled","roof:height":"3"}}]}
  result=M.generate(self.cfg,city,self.bindings)
  self.assertEqual(result["schema_version"],2)
  record=result["buildings"][0];roof=record["roof"]
  self.assertEqual(roof["asset_id"],"wtg-roof-gabled")
  self.assertGreater(roof["width_m"],0);self.assertGreater(roof["depth_m"],0);self.assertEqual(len(roof["position"]),3)
  self.assertTrue(all(item["asset_id"]=="wtg-roof-chimney" for item in record["details"]["chimney_instances"]))
  self.assertTrue(all(item["asset_id"]=="wtg-roof-dormer" for item in record["details"]["dormer_instances"]))

 def test_unreal_bake_consumes_roof_catalog(self):
  text=(ROOT/"Scripts"/"prepare_geography.py").read_text()
  self.assertIn("roof_details.json",text);self.assertIn("WROCLAW_ROOF_INSTANCES",text);self.assertIn("roof_asset_bindings.json",text)

if __name__=="__main__":unittest.main()
