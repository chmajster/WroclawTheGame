import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1]
P=R/"Scripts/input/build_input_profile.py";S=importlib.util.spec_from_file_location("profile",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def test_commonui_and_enhancedinput_enabled(self):
  p=json.loads((R/"WroclawTheGame.uproject").read_text());plugins={x["Name"]:x["Enabled"] for x in p["Plugins"]};self.assertTrue(plugins["EnhancedInput"]);self.assertTrue(plugins["CommonUI"]);self.assertIn('"CommonUI"',(R/"Source/WroclawTheGame/WroclawTheGame.Build.cs").read_text())
  engine=(R/"Config/DefaultEngine.ini").read_text();self.assertIn("GameViewportClientClassName=/Script/CommonUI.CommonGameViewportClient",engine)
 def test_action_ids_unique(self):
  d=json.loads((R/"Data/input_actions.json").read_text());ids=[a["id"] for c in d["contexts"] for a in c["actions"]];self.assertEqual(len(ids),len(set(ids)));self.assertIn("pause",ids);self.assertIn("exit_vehicle",ids)
 def test_profile_defaults_and_valid_rebind(self):
  actions=json.loads((R/"Data/input_actions.json").read_text());defaults=json.loads((R/"Data/input_profile_defaults.json").read_text());r=M.build_profile(actions,defaults,{"interact":{"keyboard":["F"]}});self.assertEqual(r["status"],"PASS");self.assertEqual(r["rebindings"]["interact"]["keyboard"],["F"])
 def test_conflicting_rebind_rejected(self):
  actions=json.loads((R/"Data/input_actions.json").read_text());defaults=json.loads((R/"Data/input_profile_defaults.json").read_text());r=M.build_profile(actions,defaults,{"interact":{"keyboard":["LeftShift"]}});self.assertEqual(r["status"],"FAIL");self.assertIn("binding_conflict",{x["code"] for x in r["issues"]})
 def test_runtime_profile_subsystem_exists(self):
  h=(R/"Source/WroclawTheGame/UI/WTGInputProfileSubsystem.h").read_text();self.assertIn("UGameInstanceSubsystem",h);self.assertIn("KeyboardOverrides",h);self.assertIn("GamepadDeadzone",h)
if __name__=="__main__":unittest.main()
