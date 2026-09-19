import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/build_crowd_plan.py";S=importlib.util.spec_from_file_location("c",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def test_only_walkable_edges(self):
  cfg=json.loads((R/"Data/crowd_simulation_policy.json").read_text());city={"road_graph":{"nodes":[{"id":"a","position":[0,0,0]},{"id":"b","position":[1,0,0]}],"edges":[{"id":"x","from":"a","to":"b","foot":True},{"id":"y","from":"a","to":"b","foot":False}]},"features":[]}
  r=M.generate(cfg,city);self.assertEqual([x["edge"] for x in r["corridors"]],["x"])
if __name__=="__main__":unittest.main()
