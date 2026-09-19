import importlib.util,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/validate_multidistrict_quests.py";S=importlib.util.spec_from_file_location("q",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def test_connected_quest_passes(self):
  city={"sectors":[{"id":"a","district":"A"},{"id":"b","district":"B"}],"links":[{"from":"a","to":"b","foot":True,"car":False}]};quests={"quests":[{"id":"q","stages":[{"district":"A"},{"district":"B"}]}]};self.assertEqual(M.validate(quests,city)["status"],"PASS")
 def test_unreachable_fails(self):
  city={"sectors":[{"id":"a","district":"A"},{"id":"b","district":"B"}],"links":[]};quests={"quests":[{"id":"q","stages":[{"district":"A"},{"district":"B"}]}]};self.assertEqual(M.validate(quests,city)["status"],"FAIL")
if __name__=="__main__":unittest.main()
