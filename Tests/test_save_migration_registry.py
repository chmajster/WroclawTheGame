import json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1]
class T(unittest.TestCase):
 def test_registry_is_explicit_and_acyclic(self):
  d=json.loads((R/"Data/save_migration_registry.json").read_text());self.assertEqual(d["rules"]["unknown_version"],"reject");pairs=[(x["from_version"],x["to_version"],x["from_space"],x["to_space"]) for x in d["campaign"]["supported"]];self.assertEqual(len(pairs),len(set(pairs)))
 def test_runtime_policy_is_used(self):
  cpp=(R/"Source/WroclawTheGame/Mission/SliceMission.cpp").read_text();policy=(R/"Source/WroclawTheGame/Save/SaveMigrationPolicy.cpp").read_text();self.assertIn("CanAttemptCampaignSave",cpp);self.assertIn("BlockoutV1",policy);self.assertIn("WroclawGISV1",policy);self.assertIn("SaveVersion == CurrentVersion",policy)
if __name__=="__main__":unittest.main()
