import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1]
class T(unittest.TestCase):
 def test_policy_has_garage_capacity(self):
  p=json.loads((R/"Data/parking_policy.json").read_text());self.assertGreater(p["player_garage_capacity"],0);self.assertIn("parking",p["accepted_amenities"])
if __name__=="__main__":unittest.main()
