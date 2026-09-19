import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/weather_gameplay.py";S=importlib.util.spec_from_file_location("w",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def test_rain_reduces_traction_and_visibility(self):
  c=json.loads((R/"Data/weather_gameplay_policy.json").read_text());r=M.state(c,"Rain");self.assertLess(r["traction"],1);self.assertLess(r["visibility"],1);self.assertEqual(r["street_wetness"],1)
 def test_unknown_rejected(self):
  with self.assertRaises(ValueError):M.state(json.loads((R/"Data/weather_gameplay_policy.json").read_text()),"SnowstormUnknown")
if __name__=="__main__":unittest.main()
