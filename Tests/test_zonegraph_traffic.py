import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/build_zonegraph_traffic.py";S=importlib.util.spec_from_file_location("z",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def cfg(self):return json.loads((R/"Data/traffic_simulation_policy.json").read_text())
 def test_directional_lanes_have_runtime_metadata(self):
  city={"road_graph":{"nodes":[{"id":"a","position":[0,0,0]},{"id":"b","position":[10000,0,0]}],"edges":[{"id":"e","way":"99","from":"a","to":"b","highway":"residential","maxspeed":"40","lanes":"2","length_cm":10000,"car_forward":True,"car_backward":True}]}}
  r=M.generate(self.cfg(),city);self.assertEqual(len(r["lanes"]),2);self.assertEqual({x["direction"] for x in r["lanes"]},{"forward","backward"})
  self.assertTrue(all(x["speed_limit_kph"]==40 for x in r["lanes"]));self.assertTrue(all(x["capacity"]>0 for x in r["lanes"]));self.assertGreater(r["capacity_total"],0);self.assertIn("hysteresis_m",r["handoff"])
 def test_speed_parser_supports_mph_and_defaults(self):
  cfg=self.cfg();self.assertAlmostEqual(M.parse_speed("30 mph","residential",cfg),48.3,places=1);self.assertEqual(M.parse_speed(None,"living_street",cfg),20.0)
if __name__=="__main__":unittest.main()
