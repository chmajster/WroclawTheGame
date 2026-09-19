import importlib.util,gzip,json,tempfile,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/build_intersections.py";S=importlib.util.spec_from_file_location("i",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def fixture(self):
  xml=b'<osm><node id="1"><tag k="highway" v="traffic_signals"/></node><relation id="9"><member type="way" ref="10" role="from"/><member type="node" ref="1" role="via"/><member type="way" ref="11" role="to"/><tag k="type" v="restriction"/><tag k="restriction" v="no_left_turn"/></relation></osm>'
  graph={"road_graph":{"edges":[{"id":"10:a:1:0","way":"10","from":"a","to":"1","car_forward":True,"car_backward":False},{"id":"11:1:b:0","way":"11","from":"1","to":"b","car_forward":True,"car_backward":False}]}}
  policy=json.loads((R/"Data/intersection_policy.json").read_text())
  return xml,graph,policy
 def test_relation_maps_to_directed_edges(self):
  xml,graph,policy=self.fixture()
  with tempfile.TemporaryDirectory() as d:
   p=Path(d)/"x.osm.gz";p.write_bytes(gzip.compress(xml));p.with_name("x.metadata.json").write_text('{"SourceParts":null}')
   old=M.source_bytes;M.source_bytes=lambda _:p.read_bytes()
   try:r=M.build(p,graph,policy)
   finally:M.source_bytes=old
  self.assertEqual(r["schema_version"],2);self.assertEqual(r["junctions"][0]["priority"],"signal")
  self.assertEqual(r["turn_restrictions"][0]["status"],"resolved");self.assertEqual(r["resolved_pairs"][0]["from_edge"],"10:a:1:0");self.assertEqual(r["resolved_pairs"][0]["to_edge"],"11:1:b:0")
 def test_unmapped_restriction_is_explicit(self):
  xml,_,policy=self.fixture()
  with tempfile.TemporaryDirectory() as d:
   p=Path(d)/"x.osm.gz";p.write_bytes(gzip.compress(xml));p.with_name("x.metadata.json").write_text('{"SourceParts":null}')
   old=M.source_bytes;M.source_bytes=lambda _:p.read_bytes()
   try:r=M.build(p,{"road_graph":{"edges":[]}},policy)
   finally:M.source_bytes=old
  self.assertEqual(r["unresolved_restrictions"][0]["reason"],"edge_mapping_failed")
if __name__=="__main__":unittest.main()
