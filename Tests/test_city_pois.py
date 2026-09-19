import importlib.util,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/extract_city_pois.py";S=importlib.util.spec_from_file_location("p",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def test_categories(self):
  self.assertEqual(M.category({"shop":"bakery"}),"retail");self.assertEqual(M.category({"amenity":"cafe"}),"food");self.assertEqual(M.category({"amenity":"pharmacy"}),"health");self.assertIsNone(M.category({"highway":"residential"}))
if __name__=="__main__":unittest.main()
