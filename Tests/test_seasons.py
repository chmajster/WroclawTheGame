import datetime,importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/season_state.py";S=importlib.util.spec_from_file_location("s",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def setUp(self):self.c=json.loads((R/"Data/city_seasons.json").read_text())
 def test_quarters(self):
  self.assertEqual(M.season(self.c,datetime.date(2026,1,10))["id"],"winter");self.assertEqual(M.season(self.c,datetime.date(2026,4,10))["id"],"spring");self.assertEqual(M.season(self.c,datetime.date(2026,7,10))["id"],"summer");self.assertEqual(M.season(self.c,datetime.date(2026,10,10))["id"],"autumn")
if __name__=="__main__":unittest.main()
