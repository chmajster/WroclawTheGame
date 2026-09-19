import importlib.util,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/extract_city_pois.py";S=importlib.util.spec_from_file_location("p",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def test_categories(self):
  self.assertEqual(M.category({"shop":"bakery"}),"retail");self.assertEqual(M.category({"amenity":"cafe"}),"food");self.assertEqual(M.category({"amenity":"pharmacy"}),"health");self.assertIsNone(M.category({"highway":"residential"}))
 def test_dedupe_and_search_metadata(self):
  a=M.poi_record("node/1","food",{"name":"Cafe Test","opening_hours":"Mo-Fr 08:00-18:00","wheelchair":"yes"},[0,0,0],[17,51],"node")
  b=M.poi_record("way/2","food",{"name":"Cafe Test"},[100,0,0],[17,51],"way")
  kept,aliases=M.deduplicate([a,b],2000);self.assertEqual(len(kept),1);self.assertEqual(aliases["way/2"],"node/1");self.assertEqual(kept[0]["opening_hours_raw"],"Mo-Fr 08:00-18:00");self.assertIn("cafe",kept[0]["search_tokens"]);self.assertEqual(kept[0]["wheelchair"],"yes")
 def test_unnamed_pois_are_not_merged(self):
  a=M.poi_record("node/1","parking",{},[0,0,0],[17,51],"node");b=M.poi_record("way/2","parking",{},[1,0,0],[17,51],"way");kept,_=M.deduplicate([a,b],2000);self.assertEqual(len(kept),2)
if __name__=="__main__":unittest.main()
