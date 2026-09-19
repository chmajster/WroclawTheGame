import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/weather_gameplay.py";S=importlib.util.spec_from_file_location("w",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def setUp(self):self.c=json.loads((R/"Data/weather_gameplay_policy.json").read_text())
 def test_rain_reduces_traction_and_visibility(self):
  r=M.state(self.c,"Rain");self.assertLess(r["traction"],1);self.assertLess(r["visibility"],1);self.assertEqual(r["street_wetness"],1);self.assertGreater(r["precipitation"],0)
 def test_transition_interpolates_runtime_values(self):
  r=M.transition(self.c,"Clear","Storm",0.5);self.assertGreater(r["street_wetness"],0);self.assertLess(r["traction"],1);self.assertEqual(r["target_weather"],"Storm")
 def test_unknown_rejected(self):
  with self.assertRaises(ValueError):M.state(self.c,"SnowstormUnknown")
 def test_runtime_component_exists(self):
  h=(R/"Source/WroclawTheGame/World/WTGWeatherRuntimeComponent.h").read_text();cpp=(R/"Source/WroclawTheGame/World/WTGWeatherRuntimeComponent.cpp").read_text();self.assertIn("EWTGWeatherType",h);self.assertIn("WeatherWetness",cpp);self.assertIn("FMath::InterpEaseInOut",cpp)
if __name__=="__main__":unittest.main()
