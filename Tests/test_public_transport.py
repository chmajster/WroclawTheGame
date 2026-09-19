import importlib.util,gzip,json,tempfile,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/build_public_transport.py";S=importlib.util.spec_from_file_location("p",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def test_route_stop_sequence_and_continuity(self):
  xml=b'<osm><node id="1" lat="51" lon="17"><tag k="name" v="Stop"/></node><node id="2" lat="51.1" lon="17.1"/><node id="3" lat="51.2" lon="17.2"/><way id="10"><nd ref="1"/><nd ref="2"/></way><way id="11"><nd ref="2"/><nd ref="3"/></way><relation id="2"><member type="node" ref="1" role="stop"/><member type="way" ref="10" role=""/><member type="way" ref="11" role=""/><tag k="type" v="route"/><tag k="route" v="tram"/><tag k="ref" v="3"/></relation></osm>'
  cfg=json.loads((R/"Data/public_transport_policy.json").read_text())
  with tempfile.TemporaryDirectory() as d:
   p=Path(d)/"x";p.write_bytes(gzip.compress(xml));old=M.source_bytes;M.source_bytes=lambda _:p.read_bytes()
   try:r=M.build(p,cfg)
   finally:M.source_bytes=old
  route=r["routes"][0];self.assertEqual(r["schema_version"],2);self.assertTrue(route["continuity"]["continuous"]);self.assertEqual(route["stop_sequence"][0]["stop_id"],"1");self.assertEqual(route["simulation"]["headway_minutes"],10);self.assertTrue(route["service_id"].startswith("tram:3:"))
 def test_continuity_break_is_reported(self):
  xml=b'<osm><node id="1" lat="51" lon="17"/><node id="2" lat="51" lon="18"/><node id="3" lat="52" lon="19"/><node id="4" lat="53" lon="20"/><way id="10"><nd ref="1"/><nd ref="2"/></way><way id="11"><nd ref="3"/><nd ref="4"/></way><relation id="2"><member type="way" ref="10" role=""/><member type="way" ref="11" role=""/><tag k="type" v="route"/><tag k="route" v="bus"/></relation></osm>'
  with tempfile.TemporaryDirectory() as d:
   p=Path(d)/"x";p.write_bytes(gzip.compress(xml));old=M.source_bytes;M.source_bytes=lambda _:p.read_bytes()
   try:r=M.build(p,json.loads((R/"Data/public_transport_policy.json").read_text()))
   finally:M.source_bytes=old
  self.assertEqual(r["continuity_breaks"],1)
if __name__=="__main__":unittest.main()
