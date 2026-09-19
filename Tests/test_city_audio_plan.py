import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/build_city_audio_plan.py";S=importlib.util.spec_from_file_location("a",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def test_road_and_rail_layers(self):
  cfg=json.loads((R/"Data/city_audio_profiles.json").read_text());city={"sectors":[{"id":"s","district":"d","density":"High","counts":{"roads":2,"rail_lines":1}}]};ids={x["id"] for x in M.generate(cfg,city)["sectors"][0]["layers"]};self.assertEqual(ids,{"traffic","tram","crowd"})
if __name__=="__main__":unittest.main()
