import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/build_zonegraph_traffic.py";S=importlib.util.spec_from_file_location("z",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def test_directional_lanes(self):
  cfg=json.loads((R/"Data/traffic_simulation_policy.json").read_text());city={"road_graph":{"nodes":[{"id":"a","position":[0,0,0]},{"id":"b","position":[10,0,0]}],"edges":[{"id":"e","from":"a","to":"b","car_forward":True,"car_backward":True}]}}
  r=M.generate(cfg,city);self.assertEqual(len(r["lanes"]),2);self.assertEqual({x["direction"] for x in r["lanes"]},{"forward","backward"})
if __name__=="__main__":unittest.main()
