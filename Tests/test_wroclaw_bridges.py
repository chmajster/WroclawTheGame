import importlib.util,json,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];P=ROOT/"Scripts"/"gis"/"resolve_wroclaw_bridges.py";S=importlib.util.spec_from_file_location("bridges",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class BridgeTests(unittest.TestCase):
 def test_catalog_has_major_bridges(self):
  d=json.loads((ROOT/"Data"/"wroclaw_bridges.json").read_text());ids={b["id"] for b in d["bridges"]};self.assertGreaterEqual(len(ids),18);self.assertTrue({"grunwaldzki","tumski","redzinski","milenijny"}<=ids)
 def test_resolves_bridge_by_name(self):
  cat={"bridges":[{"id":"g","name":"Most Grunwaldzki","aliases":[],"quality":"hero"}]}
  city={"features":[{"id":"way/1:line:0","kind":"road","name":"Most Grunwaldzki","points":[[0,0,0],[1,1,0]],"tags":{"bridge":"yes"}}]}
  r=M.resolve(cat,city,{})
  self.assertEqual(r["counts"]["resolved"],1);self.assertTrue(r["bridges"][0]["requires_surveyed_elevation"])
 def test_non_bridge_way_does_not_match(self):
  cat={"bridges":[{"id":"g","name":"Most Grunwaldzki","aliases":[],"quality":"hero"}]}
  city={"features":[{"id":"way/1","kind":"road","name":"Most Grunwaldzki","points":[],"tags":{"bridge":"no"}}]}
  self.assertEqual(M.resolve(cat,city,{})["counts"]["unresolved"],1)
if __name__=="__main__":unittest.main()
