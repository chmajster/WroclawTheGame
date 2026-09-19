import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/day_night_schedule.py";S=importlib.util.spec_from_file_location("d",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def setUp(self):self.c=json.loads((R/"Data/day_night_policy.json").read_text())
 def test_phases_and_environment_state(self):
  self.assertEqual(M.state(self.c,2)["phase"],"night");self.assertEqual(M.state(self.c,12)["phase"],"day");self.assertEqual(M.state(self.c,19)["phase"],"evening")
  self.assertGreater(M.state(self.c,12)["sun_lux"],M.state(self.c,2)["sun_lux"]);self.assertGreater(M.state(self.c,2)["window_emissive"],M.state(self.c,12)["window_emissive"])
 def test_wrap(self):self.assertEqual(M.state(self.c,26)["phase"],"night")
 def test_transition_is_smooth(self):
  before=M.state(self.c,4.25);blend=M.state(self.c,4.75);self.assertEqual(blend["blend_to"],"morning");self.assertGreater(blend["transition_alpha"],0);self.assertLess(blend["street_light"],before["street_light"])
 def test_runtime_actor_source_exists(self):
  h=(R/"Source/WroclawTheGame/World/WTGDayNightEnvironment.h").read_text();cpp=(R/"Source/WroclawTheGame/World/WTGDayNightEnvironment.cpp").read_text()
  for token in ("UDirectionalLightComponent","USkyAtmosphereComponent","UVolumetricCloudComponent","UExponentialHeightFogComponent"):self.assertIn(token,h)
  for token in ("SetAtmosphereSunLightIndex(0)","SetAtmosphereSunLightIndex(1)","SetRealTimeCapture(true)","SetVolumetricFog(true)","WindowEmissive"):self.assertIn(token,cpp)
if __name__=="__main__":unittest.main()
