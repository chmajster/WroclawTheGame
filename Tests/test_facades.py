import importlib.util,json,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];P=ROOT/"Scripts"/"gis"/"generate_facades.py";S=importlib.util.spec_from_file_location("facades",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class FacadeTests(unittest.TestCase):
 def setUp(self):self.cfg=json.loads((ROOT/"Data"/"facade_profiles.json").read_text())
 def feature(self,profile="tenement",levels="4"):
  return {"features":[{"id":"b","kind":"building","architecture_profile":profile,"height_m":16,"rings":[[[0,0,0],[1200,0,0],[1200,800,0],[0,800,0],[0,0,0]]],"tags":{"building:levels":levels}}]}
 def test_levels_and_bays(self):
  b=M.generate(self.cfg,self.feature())["buildings"][0];self.assertEqual(b["floors"],4);self.assertTrue(all(e["bays"]>=1 for e in b["facades"]))
 def test_deterministic(self):
  self.assertEqual(M.generate(self.cfg,self.feature()),M.generate(self.cfg,self.feature()))
 def test_profiles_differ(self):
  a=M.generate(self.cfg,self.feature("tenement"))["buildings"][0];b=M.generate(self.cfg,self.feature("estate"))["buildings"][0];self.assertNotEqual(a["architecture_profile"],b["architecture_profile"])
if __name__=="__main__":unittest.main()
