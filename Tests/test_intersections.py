import importlib.util,gzip,tempfile,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/build_intersections.py";S=importlib.util.spec_from_file_location("i",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def test_relation_and_signal(self):
  xml=b'<osm><node id="1"><tag k="highway" v="traffic_signals"/></node><relation id="9"><member type="way" ref="10" role="from"/><member type="node" ref="1" role="via"/><member type="way" ref="11" role="to"/><tag k="type" v="restriction"/><tag k="restriction" v="no_left_turn"/></relation></osm>'
  with tempfile.TemporaryDirectory() as d:
   p=Path(d)/"x.osm.gz";p.write_bytes(gzip.compress(xml));meta=p.with_name("x.metadata.json");meta.write_text('{"SourceParts":null}')
   old=M.source_bytes;M.source_bytes=lambda _:p.read_bytes()
   try:r=M.build(p)
   finally:M.source_bytes=old
  self.assertEqual(r["junctions"][0]["control"],"traffic_signals");self.assertEqual(r["turn_restrictions"][0]["restriction"],"no_left_turn")
if __name__=="__main__":unittest.main()
