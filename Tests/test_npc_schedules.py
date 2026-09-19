import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/simulate_npc_schedules.py";S=importlib.util.spec_from_file_location("n",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def policy(self):return json.loads((R/"Data/npc_schedule_policy.json").read_text())
 def npc(self):return {"id":"n","position":[0,0,0],"schedule":[{"id":"home","hour":0,"location":[0,0,0]},{"id":"work","hour":8,"location":[972000,0,0]},{"id":"home2","hour":18,"location":[0,0,0]}]}
 def test_wrap_uses_previous_slot(self):
  self.assertEqual(M.state(self.npc(),12,policy=self.policy())["schedule_hour"],8);self.assertEqual(M.state(self.npc(),23,policy=self.policy())["schedule_hour"],18)
 def test_travel_state_interpolates(self):
  r=M.state(self.npc(),7,policy=self.policy());self.assertTrue(r["traveling"]);self.assertGreater(r["travel_progress"],0);self.assertLess(r["logical_location"][0],972000)
 def test_representation_by_distance(self):
  near=M.state(self.npc(),1,[0,0,0],policy=self.policy());far=M.state(self.npc(),1,[1000000,0,0],policy=self.policy());self.assertEqual(near["representation"],"physical");self.assertEqual(far["representation"],"logical")
 def test_context_override(self):
  n=self.npc();n["overrides"]=[{"when":{"weather":["Storm"]},"schedule":[{"id":"stay","hour":0,"location":[5,0,0]}]}];r=M.state(n,12,policy=self.policy(),context={"weather":"Storm"});self.assertEqual(r["logical_location"],[5,0,0])
if __name__=="__main__":unittest.main()
