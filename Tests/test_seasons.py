import datetime,importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/season_state.py";S=importlib.util.spec_from_file_location("s",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def setUp(self):self.c=json.loads((R/"Data/city_seasons.json").read_text())
 def test_quarters(self):
  self.assertEqual(M.season(self.c,datetime.date(2026,1,10))["id"],"winter");self.assertEqual(M.season(self.c,datetime.date(2026,4,10))["id"],"spring");self.assertEqual(M.season(self.c,datetime.date(2026,7,10))["id"],"summer");self.assertEqual(M.season(self.c,datetime.date(2026,10,10))["id"],"autumn")
 def test_daylight_and_variants(self):
  winter=M.state(self.c,datetime.date(2026,1,10));summer=M.state(self.c,datetime.date(2026,7,10));self.assertLess(winter["daylight_hours"],summer["daylight_hours"]);self.assertGreater(winter["sunrise_hour"],summer["sunrise_hour"]);self.assertEqual(winter["foliage_variant"],"winter_bare");self.assertTrue(winter["snow_allowed"]);self.assertEqual(winter["decoration_layer"],"DL_Season_Winter")
 def test_transition_blends_near_boundary(self):
  r=M.state(self.c,datetime.date(2026,2,25));self.assertEqual(r["blend_to"],"spring");self.assertGreater(r["transition_alpha"],0);self.assertGreater(r["foliage"],0.15)
 def test_day_night_runtime_accepts_seasonal_hours(self):
  h=(R/"Source/WroclawTheGame/World/WTGDayNightEnvironment.h").read_text();cpp=(R/"Source/WroclawTheGame/World/WTGDayNightEnvironment.cpp").read_text();self.assertIn("SeasonalDaylightHours",h);self.assertIn("SetSeasonalDaylightHours",h);self.assertIn('TEXT("SeasonDaylightHours")',cpp)
if __name__=="__main__":unittest.main()
