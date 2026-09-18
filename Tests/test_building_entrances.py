import importlib.util,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];P=ROOT/"Scripts"/"gis"/"resolve_building_entrances.py";S=importlib.util.spec_from_file_location("entries",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class EntranceTests(unittest.TestCase):
 def city(self):
  return {"features":[
   {"id":"way/1","kind":"building","rings":[[[0,0,0],[1000,0,0],[1000,1000,0],[0,1000,0],[0,0,0]]],"tags":{"addr:street":"Testowa","addr:housenumber":"1"}},
   {"id":"node/1","kind":"entrance","points":[[500,0,0]],"tags":{"entrance":"yes","addr:housenumber":"1"}}
  ]}
 def test_real_node_wins(self):
  r=M.generate(self.city());self.assertEqual(r["count"],1);self.assertEqual(r["entrances"][0]["source"],"osm_node");self.assertEqual(r["entrances"][0]["building_id"],"way/1")
 def test_addressed_building_gets_explicit_fallback(self):
  c=self.city();c["features"]=c["features"][:1];r=M.generate(c);self.assertEqual(r["entrances"][0]["source"],"procedural_edge_candidate")
if __name__=="__main__":unittest.main()
