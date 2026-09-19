import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/build_crowd_plan.py";S=importlib.util.spec_from_file_location("c",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def cfg(self):return json.loads((R/"Data/crowd_simulation_policy.json").read_text())
 def city(self):return {"road_graph":{"nodes":[{"id":"a","position":[0,0,0]},{"id":"b","position":[10000,0,0]}],"edges":[{"id":"x","from":"a","to":"b","foot":True,"highway":"residential","length_cm":10000},{"id":"y","from":"a","to":"b","foot":False,"highway":"primary","length_cm":10000}]},"features":[]}
 def test_only_walkable_edges_and_capacity(self):
  r=M.generate(self.cfg(),self.city());self.assertEqual([x["edge"] for x in r["corridors"]],["x"]);self.assertGreater(r["corridors"][0]["capacity"],0);self.assertEqual(r["schema_version"],2)
 def test_day_night_and_weather_reduce_density(self):
  base=M.generate(self.cfg(),self.city());wet=M.generate(self.cfg(),self.city(),{"crowd_multiplier":0.5},{"crowd_multiplier":0.5})
  self.assertAlmostEqual(wet["activity_multiplier"],0.25);self.assertLess(wet["corridors"][0]["effective_density"],base["corridors"][0]["effective_density"])
if __name__=="__main__":unittest.main()
