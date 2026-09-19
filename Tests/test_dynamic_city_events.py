import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/dynamic_city_events.py";S=importlib.util.spec_from_file_location("e",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def setUp(self):self.c=json.loads((R/"Data/dynamic_city_events.json").read_text())
 def test_storm_outage_only_in_storm(self):
  e=next(x for x in self.c["events"] if x["id"]=="power_outage");self.assertTrue(M.eligible(e,21,"Storm",0));self.assertFalse(M.eligible(e,21,"Clear",0))
 def test_police_check_needs_heat(self):
  e=next(x for x in self.c["events"] if x["id"]=="police_check");self.assertFalse(M.eligible(e,20,"Clear",0));self.assertTrue(M.eligible(e,20,"Clear",30))
if __name__=="__main__":unittest.main()
