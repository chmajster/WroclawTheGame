import importlib.util,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/simulate_npc_schedules.py";S=importlib.util.spec_from_file_location("n",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def test_wrap_uses_last_previous_slot(self):
  npc={"id":"n","position":[0,0,0],"schedule":[{"hour":0,"location":[1,0,0]},{"hour":8,"location":[2,0,0]},{"hour":18,"location":[3,0,0]}]}
  self.assertEqual(M.state(npc,12)["logical_location"],[2,0,0]);self.assertEqual(M.state(npc,23)["logical_location"],[3,0,0])
 def test_physicalizes_only_near(self):
  npc={"id":"n","position":[0,0,0],"schedule":[]};self.assertTrue(M.state(npc,1,[0,0,0],10)["physicalize"]);self.assertFalse(M.state(npc,1,[100,0,0],10)["physicalize"])
if __name__=="__main__":unittest.main()
