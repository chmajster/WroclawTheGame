import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/validate_world.py";S=importlib.util.spec_from_file_location("v",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def setUp(self):self.p=json.loads((R/"Data/world_validation_policy.json").read_text())
 def base(self):return {"features":[{"id":"b","kind":"building","rings":[[[0,0,0],[100,0,0],[100,100,0],[0,0,0]]]},{"id":"road","kind":"road","points":[[0,0,0],[100,0,0]]}],"road_graph":{"nodes":[{"id":"a"},{"id":"z"}],"edges":[{"id":"e","from":"a","to":"z","length_cm":100}]}}
 def city(self):return {"sectors":[{"id":"s","district":"d"}]}
 def test_valid_minimal_world_passes(self):self.assertEqual(M.validate(self.p,self.base(),self.city())["status"],"PASS")
 def test_orphan_entrance_and_unresolved_hero_fail(self):
  r=M.validate(self.p,self.base(),self.city(),{"entrances":[{"id":"x","building_id":"missing"}]},landmarks={"records":[{"id":"h","quality":"hero","status":"unresolved"}]});self.assertEqual(r["status"],"FAIL");self.assertIn("orphan_entrance",{x["code"] for x in r["issues"]});self.assertIn("unresolved_hero_landmark",{x["code"] for x in r["issues"]})
 def test_parking_smart_crowd_traffic_hlod_and_layers_pass(self):
  parking={"parking":[{"id":"p","capacity":1,"slots":[{"id":"p/slot/0","position":[0,0,0]}]}]}
  smart={"objects":[{"id":"smart:road","source_feature":"road","capacity":1,"slots":[{"id":"smart:road/slot/0","position":[0,0,0]}]}]}
  crowd={"corridors":[{"id":"foot:e","edge":"e","capacity":2}]}
  traffic={"lanes":[{"id":"e:f","edge":"e","speed_limit_kph":30,"capacity":2}]}
  layers={"status":"PASS","layers":[{"id":"DL_BaseGeometry"},{"id":"DL_Buildings"}],"assignments":[{"id":"b","layer":"DL_Buildings"}],"activation_sets":{"default":["DL_BaseGeometry","DL_Buildings"]}}
  hlod={"assignments":[{"building_id":"b","data_layer":"DL_Buildings","memory_budget_mb":48,"quality":"hero","silhouette_protected":True}]}
  r=M.validate(self.p,self.base(),self.city(),parking=parking,smart_objects=smart,crowd=crowd,traffic=traffic,hlod=hlod,data_layers=layers);self.assertEqual(r["status"],"PASS");self.assertEqual(r["counts"]["traffic_lanes"],1)
 def test_cross_system_reference_errors_fail(self):
  parking={"parking":[{"id":"p","capacity":2,"slots":[{"id":"slot","position":[0,0,0]}]}]}
  smart={"objects":[{"id":"x","source_feature":"missing","capacity":1,"slots":[{"id":"x/0","position":[0,0,0]}]}]}
  crowd={"corridors":[{"id":"c","edge":"missing","capacity":0}]};traffic={"lanes":[{"id":"l","edge":"missing","speed_limit_kph":0,"capacity":0}]}
  r=M.validate(self.p,self.base(),self.city(),parking=parking,smart_objects=smart,crowd=crowd,traffic=traffic);codes={x["code"] for x in r["issues"]};self.assertEqual(r["status"],"FAIL");self.assertTrue({"parking_capacity_mismatch","smart_object_unknown_source","crowd_unknown_edge","traffic_unknown_edge"}<=codes)
 def test_floating_service_benchmark_and_migrations(self):
  migrations=json.loads((R/"Data/save_migration_registry.json").read_text());r=M.validate(self.p,self.base(),self.city(),terrain_checks=[{"id":"prop","object_z_cm":500,"terrain_z_cm":0}],service_probes=[{"id":"police","critical":True,"status":"unavailable"}],benchmark_report={"status":"FAIL"},migration_registry=migrations);codes={x["code"] for x in r["issues"]};self.assertTrue({"floating_object","critical_service_unroutable","benchmark_failed"}<=codes)
 def test_compiled_quest_route_is_checked(self):
  quests={"compiled_quests":[{"id":"q","stages":[{"district":"d"}],"route_legs":[{"from_stage":"a","to_stage":"b","district_path":None}]}]};r=M.validate(self.p,self.base(),self.city(),quests=quests);self.assertIn("unroutable_quest_leg",{x["code"] for x in r["issues"]})
if __name__=="__main__":unittest.main()
