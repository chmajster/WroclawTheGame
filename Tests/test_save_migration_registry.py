import importlib.util,json,unittest
from pathlib import Path

R=Path(__file__).resolve().parents[1]
P=R/"Scripts/save/validate_save_migrations.py"
S=importlib.util.spec_from_file_location("migrations",P)
M=importlib.util.module_from_spec(S);S.loader.exec_module(M)

class T(unittest.TestCase):
 def setUp(self):self.d=json.loads((R/"Data/save_migration_registry.json").read_text())

 def test_registry_is_explicit_and_valid(self):
  self.assertEqual(self.d["rules"]["unknown_version"],"reject")
  self.assertTrue(self.d["rules"]["backup_before_migration"])
  self.assertEqual(M.validate_registry(self.d),[])

 def test_composable_migration_paths_match_golden_fixtures(self):
  fixtures=[
   json.loads((R/"Tests/fixtures/save_migration_v2.json").read_text()),
   json.loads((R/"Tests/fixtures/save_migration_v3_blockout.json").read_text())
  ]
  for f in fixtures:
   self.assertEqual(M.build_plan(self.d,f["version"],f["coordinate_space"],f.get("legacy",False)),f["expected_steps"])
  self.assertEqual(M.build_plan(self.d,3,"WroclawGISV1",False),[])
  self.assertIsNone(M.build_plan(self.d,99,"WroclawGISV1",False))

 def test_new_system_payloads_are_defaulted_and_newer_rejected(self):
  p=M.migrate_payloads(self.d,{})
  self.assertEqual(set(p),{"parking","npc_schedule","quests","season"})
  self.assertEqual(p["season"]["schema_version"],1)
  with self.assertRaises(ValueError):M.migrate_payloads(self.d,{"parking":{"schema_version":99,"json":"{}"}})

 def test_stable_id_remap_is_identity_without_registered_rename(self):
  self.assertEqual(M.remap_id(self.d,"poi","poi/123"),"poi/123")
  self.assertEqual(M.backup_slot(self.d,"Przebudzenie_v3"),"Przebudzenie_v3_pre_migration_backup")

 def test_runtime_policy_and_backup_are_used(self):
  mission=(R/"Source/WroclawTheGame/Mission/SliceMission.cpp").read_text()
  policy=(R/"Source/WroclawTheGame/Save/SaveMigrationPolicy.cpp").read_text()
  save=(R/"Source/WroclawTheGame/Save/SliceSave.h").read_text()
  economy=(R/"Source/WroclawTheGame/Systems/WTGEconomySubsystem.h").read_text()
  self.assertIn("BuildCampaignMigrationPlan",mission)
  self.assertIn("_pre_migration_backup",mission)
  self.assertIn("EnsureCurrentSystemPayloads",mission)
  self.assertIn("Economy->ImportSaveState",mission)
  self.assertIn("Economy->ExportSaveState",mission)
  self.assertIn("BuildCampaignMigrationPlan",policy)
  self.assertIn("legacy_history_replay",policy)
  self.assertIn("CampaignMigrationDefinition",policy)
  self.assertIn("FWTGSystemSavePayload",save)
  self.assertIn("FWTGEconomySaveState EconomyState",save)
  self.assertIn("SaveGame, BlueprintReadWrite",economy)

if __name__=="__main__":unittest.main()
