import copy,json,math,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'Scripts/gis'))
from import_sector import build,validate,source_bytes
from road_routes import route,adjacency
class GeographyTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory();cls.data,cls.geo=build(output=Path(cls.tmp.name))
 @classmethod
 def tearDownClass(cls):cls.tmp.cleanup()
 def test_geographic_roundtrip_and_scale(self):
  for lon,lat,height in [(17.03,51.12,112),(17.037,51.125,118)]:
   p=self.geo.world(lon,lat,height);q=self.geo.geographic(p)
   self.assertAlmostEqual(q[0],lon,7);self.assertAlmostEqual(q[1],lat,7);self.assertAlmostEqual(q[2],height,4)
  e,n,h=self.geo.origin;lon,lat=self.geo.inverse.transform(e+100,n-100);point=self.geo.world(lon,lat,h)
  self.assertAlmostEqual(point[0],10000,2);self.assertAlmostEqual(point[1],10000,2)
 def test_real_streets_and_provenance(self):
  names={e['name'] for e in self.data['road_graph']['edges']}
  self.assertTrue({'Pomorska','Ludwika Rydygiera','Trzebnicka'}<=names)
  self.assertEqual(self.data['sources'][0]['SourceLicense'],'ODbL-1.0')
  self.assertGreater(len(self.data['features']),100)
 def test_oneway_and_pedestrian_topology(self):
  graph={'nodes':[{'id':str(n)} for n in range(3)],'edges':[
   {'id':'a','from':'0','to':'1','length_cm':100,'car_forward':True,'car_backward':False,'foot':True,'bridge':'no','layer':'0'},
   {'id':'b','from':'1','to':'2','length_cm':100,'car_forward':False,'car_backward':False,'foot':True,'bridge':'no','layer':'0'}]}
  self.assertEqual(route(graph,'0','1'),['0','1'])
  with self.assertRaises(ValueError):route(graph,'1','0')
  with self.assertRaises(ValueError):route(graph,'0','2')
  self.assertEqual(route(graph,'2','0','foot'),['2','1','0'])
 def test_race_routes_follow_real_directed_edges(self):
  links=adjacency(self.data['road_graph'])
  for race in json.loads((ROOT/'Data/processed/wroclaw/races.json').read_text()):
   for a,b in zip(race['source_nodes'],race['source_nodes'][1:]):self.assertIn(b,{e[0] for e in links[a]})
 def test_invalid_graph_rejected(self):
  broken=copy.deepcopy(self.data);broken['road_graph']['edges'][0]['to']='missing'
  with self.assertRaises(ValueError):validate(broken)
 def test_source_heights_are_not_flat(self):
  heights=list(self.geo.elevations.values());self.assertGreater(max(heights)-min(heights),5)
if __name__=='__main__':unittest.main()


class SourcePartsTests(unittest.TestCase):
 def test_order_corruption_and_path_traversal_are_rejected(self):
  import hashlib
  with tempfile.TemporaryDirectory() as tmp:
   root=Path(tmp);path=root/'example.osm.gz';meta=root/'example.metadata.json'
   parts=[]
   for i,raw in enumerate((b'first',b'second'),1):
    name=f'example.osm.gz.part{i:03d}';(root/name).write_bytes(raw)
    parts.append({'File':name,'SHA256':hashlib.sha256(raw).hexdigest()})
   meta.write_text(json.dumps({'SourceParts':parts}))
   self.assertEqual(source_bytes(path),b'firstsecond')
   (root/parts[1]['File']).write_bytes(b'corrupt')
   with self.assertRaises(ValueError):source_bytes(path)
   parts[0]['File']='../outside'
   meta.write_text(json.dumps({'SourceParts':parts}))
   with self.assertRaises(ValueError):source_bytes(path)
