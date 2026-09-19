import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/build_smart_objects.py";S=importlib.util.spec_from_file_location("s",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def test_roles(self):
  self.assertEqual(M.role_for({"entrance":"yes"}),"building_entrance");self.assertEqual(M.role_for({"shop":"bakery"}),"shop");self.assertEqual(M.role_for({"amenity":"parking"}),"parking")
if __name__=="__main__":unittest.main()
