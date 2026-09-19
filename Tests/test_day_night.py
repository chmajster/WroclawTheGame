import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/day_night_schedule.py";S=importlib.util.spec_from_file_location("d",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def setUp(self):self.c=json.loads((R/"Data/day_night_policy.json").read_text())
 def test_phases(self):
  self.assertEqual(M.state(self.c,2)["phase"],"night");self.assertEqual(M.state(self.c,12)["phase"],"day");self.assertEqual(M.state(self.c,19)["phase"],"evening")
 def test_wrap(self):self.assertEqual(M.state(self.c,26)["phase"],"night")
if __name__=="__main__":unittest.main()
