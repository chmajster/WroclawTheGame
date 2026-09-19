import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/dynamic_city_events.py";S=importlib.util.spec_from_file_location("e",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def setUp(self):self.c=json.loads((R/"Data/dynamic_city_events.json").read_text());self.city={"road_graph":{"edges":[{"id":"edge/1","way":"1","car_forward":True,"car_backward":True}]},"sectors":[{"id":"sector.a"}]}
 def test_storm_outage_only_in_storm(self):
  e=next(x for x in self.c["events"] if x["id"]=="power_outage");self.assertTrue(M.eligible(e,21,"Storm",0));self.assertFalse(M.eligible(e,21,"Clear",0))
 def test_police_check_needs_heat(self):
  e=next(x for x in self.c["events"] if x["id"]=="police_check");self.assertFalse(M.eligible(e,20,"Clear",0));self.assertTrue(M.eligible(e,20,"Clear",30))
 def test_instance_uses_real_graph_location_and_mutations(self):
  e=next(x for x in self.c["events"] if x["id"]=="roadworks");i=M.instantiate(e,self.city,"seed",100);self.assertEqual(i["location"]["id"],"edge/1");self.assertIn("blocked_edge",{m["type"] for m in i["mutations"]});self.assertGreater(i["expires_minutes"],100)
 def test_cooldown_and_cleanup(self):
  e=next(x for x in self.c["events"] if x["id"]=="roadworks");state={"active":[],"last_triggered":{"roadworks":100}};self.assertFalse(M.eligible(e,10,"Clear",0,100,120));inst=M.instantiate(e,self.city,"s",0);new,cleanup=M.advance({"active":[inst],"last_triggered":{}},1000);self.assertEqual(new["active"],[]);self.assertEqual(cleanup["removed_instance_ids"],[inst["instance_id"]])
if __name__=="__main__":unittest.main()
