import importlib.util,gzip,tempfile,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/build_public_transport.py";S=importlib.util.spec_from_file_location("p",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def test_route_and_stop(self):
  xml=b'<osm><node id="1" lat="51" lon="17"><tag k="name" v="Stop"/></node><relation id="2"><member type="node" ref="1" role="stop"/><tag k="type" v="route"/><tag k="route" v="tram"/><tag k="ref" v="3"/></relation></osm>'
  with tempfile.TemporaryDirectory() as d:
   p=Path(d)/"x";p.write_bytes(gzip.compress(xml));old=M.source_bytes;M.source_bytes=lambda _:p.read_bytes()
   try:r=M.build(p)
   finally:M.source_bytes=old
  self.assertEqual(r["routes"][0]["mode"],"tram");self.assertEqual(r["stops"][0]["id"],"1")
if __name__=="__main__":unittest.main()
