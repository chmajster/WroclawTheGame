import json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1]
class T(unittest.TestCase):
 def test_commonui_and_enhancedinput_enabled(self):
  p=json.loads((R/"WroclawTheGame.uproject").read_text());plugins={x["Name"]:x["Enabled"] for x in p["Plugins"]};self.assertTrue(plugins["EnhancedInput"]);self.assertTrue(plugins["CommonUI"]);self.assertIn('"CommonUI"',(R/"Source/WroclawTheGame/WroclawTheGame.Build.cs").read_text())
  engine=(R/"Config/DefaultEngine.ini").read_text();self.assertIn("GameViewportClientClassName=/Script/CommonUI.CommonGameViewportClient",engine)
 def test_action_ids_unique(self):
  d=json.loads((R/"Data/input_actions.json").read_text());ids=[a["id"] for c in d["contexts"] for a in c["actions"]];self.assertEqual(len(ids),len(set(ids)));self.assertIn("pause",ids);self.assertIn("exit_vehicle",ids)
if __name__=="__main__":unittest.main()
