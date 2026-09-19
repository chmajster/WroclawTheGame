import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/build_city_audio_plan.py";S=importlib.util.spec_from_file_location("a",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def setUp(self):self.cfg=json.loads((R/"Data/city_audio_profiles.json").read_text())
 def test_environment_layers(self):
  city={"sectors":[{"id":"s","district":"d","density":"High","counts":{"roads":2,"rail_lines":1,"green_areas":2,"water_areas":1,"industrial_areas":1}}]}
  ids={x["id"] for x in M.generate(self.cfg,city)["sectors"][0]["layers"]};self.assertEqual(ids,{"traffic","tram","crowd","park","river","industrial"})
 def test_night_and_weather_mix(self):
  city={"sectors":[{"id":"s","district":"d","density":"VeryHigh","counts":{"roads":10}}]}
  day=M.generate(self.cfg,city,{"phase":"day","traffic_multiplier":1,"crowd_multiplier":1,"night_alpha":0},{"weather":"Clear"});night=M.generate(self.cfg,city,{"phase":"night","traffic_multiplier":0.25,"crowd_multiplier":0.15,"night_alpha":1},{"weather":"Rain"})
  self.assertIn("night",{x["id"] for x in night["sectors"][0]["layers"]});self.assertLess(next(x["gain"] for x in night["sectors"][0]["layers"] if x["id"]=="traffic"),next(x["gain"] for x in day["sectors"][0]["layers"] if x["id"]=="traffic"))
 def test_acoustic_budget(self):
  p=M.acoustic_profile(self.cfg,{"density":"VeryHigh"});self.assertEqual(p["reverb_profile"],"city_canyon");self.assertGreater(p["voice_budget"],0)
if __name__=="__main__":unittest.main()
