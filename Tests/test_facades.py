import importlib.util,json,unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
P=ROOT/"Scripts"/"gis"/"generate_facades.py"
S=importlib.util.spec_from_file_location("facades",P)
M=importlib.util.module_from_spec(S);S.loader.exec_module(M)

class FacadeTests(unittest.TestCase):
 def setUp(self):
  self.cfg=json.loads((ROOT/"Data"/"facade_profiles.json").read_text())
  self.bindings=json.loads((ROOT/"Data"/"facade_asset_bindings.json").read_text())

 def feature(self,profile="tenement",levels="4",tags=None):
  base={"building:levels":levels}
  if tags:base.update(tags)
  return {"features":[{"id":"b","kind":"building","architecture_profile":profile,"height_m":16,"rings":[[[0,0,0],[1200,0,0],[1200,800,0],[0,800,0],[0,0,0]]],"tags":base}]}

 def test_levels_bays_and_microdetails(self):
  r=M.generate(self.cfg,self.feature(),bindings=self.bindings);b=r["buildings"][0]
  self.assertEqual(r["schema_version"],2);self.assertEqual(b["floors"],4);self.assertTrue(all(e["bays"]>=1 for e in b["facades"]))
  self.assertGreater(b["counts"].get("window",0),0);self.assertGreater(b["counts"].get("gutter",0),0);self.assertGreater(b["counts"].get("downspout",0),0)
  self.assertGreater(b["counts"].get("door",0),0);self.assertFalse(b["factual_facade"])

 def test_real_entrance_becomes_interactive_door_on_matching_edge(self):
  entrances={"entrances":[{"id":"node/1","building_id":"b","position":[600,0,0],"source":"osm_node","street":"Testowa","housenumber":"12"}]}
  b=M.generate(self.cfg,self.feature(),entrances,self.bindings)["buildings"][0];door=b["doors"][0]
  self.assertEqual(door["entrance_id"],"node/1");self.assertEqual(door["source"],"osm_node");self.assertTrue(door["interactive"]);self.assertEqual(door["edge_index"],0)
  self.assertEqual(door["address"]["housenumber"],"12");self.assertTrue(door["asset_id"].startswith("kenney-door-"))
  ground_same_bay=[x for x in b["openings"] if x.get("kind")=="window" and x.get("floor")==0 and x.get("edge_index")==door["edge_index"] and x.get("bay")==door["bay"]]
  self.assertEqual(ground_same_bay,[]);self.assertIn("address_plaque",{x["kind"] for x in b["details"]})

 def test_windows_bind_existing_models_and_night_emissive(self):
  b=M.generate(self.cfg,self.feature(),bindings=self.bindings)["buildings"][0]
  windows=[x for x in b["openings"] if x["kind"]=="window"];self.assertGreater(len(windows),0)
  self.assertTrue(all(x["asset_id"].startswith("kenney-window-") for x in windows));self.assertTrue(all(x["night_emissive_parameter"]=="WindowEmissive" for x in windows))
  self.assertTrue(all(x["sill_asset_id"]=="procedural:window_sill" for x in windows))

 def test_commercial_ground_floor_gets_storefront_signs_and_optional_awnings(self):
  city=self.feature("mixed","3",{"building":"commercial","shop":"convenience"})
  b=M.generate(self.cfg,city,bindings=self.bindings)["buildings"][0]
  storefronts=[x for x in b["openings"] if x["kind"]=="storefront"];self.assertGreater(len(storefronts),0)
  self.assertTrue(all(x["asset_id"] for x in storefronts));self.assertGreater(b["counts"].get("storefront_sign",0),0)

 def test_balconies_aircon_gutters_and_cornices_have_stable_ids(self):
  a=M.generate(self.cfg,self.feature("estate","8"),bindings=self.bindings);b=M.generate(self.cfg,self.feature("estate","8"),bindings=self.bindings)
  self.assertEqual(a,b);details=a["buildings"][0]["details"];ids=[x["id"] for x in details];self.assertEqual(len(ids),len(set(ids)))
  self.assertIn("balcony",{x["kind"] for x in details});self.assertIn("gutter",{x["kind"] for x in details});self.assertIn("downspout",{x["kind"] for x in details})

 def test_all_bound_nonprocedural_asset_ids_exist_in_repo_catalogs(self):
  ids=set()
  for file in ("free_external_model_catalog.json","free_model_catalog.json"):
   for item in json.loads((ROOT/"Data"/file).read_text()):ids.add(item.get("id") or item.get("slug"))
  referenced=set()
  for values in self.bindings["groups"].values():referenced.update(values)
  self.assertEqual(sorted(referenced-ids),[])

 def test_profiles_differ(self):
  a=M.generate(self.cfg,self.feature("tenement"),bindings=self.bindings)["buildings"][0]
  b=M.generate(self.cfg,self.feature("estate"),bindings=self.bindings)["buildings"][0]
  self.assertNotEqual(a["architecture_profile"],b["architecture_profile"]);self.assertNotEqual(a["doors"][0]["asset_id"],b["doors"][0]["asset_id"])

 def test_city_build_generates_and_bakes_facade_catalog(self):
  build=(ROOT/"Scripts/Build-Geography.ps1").read_text()
  editor=(ROOT/"Scripts/prepare_geography.py").read_text()
  cluster_h=(ROOT/"Source/WroclawTheGame/World/WTGFacadeInstanceCluster.h").read_text()
  cluster_cpp=(ROOT/"Source/WroclawTheGame/World/WTGFacadeInstanceCluster.cpp").read_text()
  self.assertIn("resolve_building_entrances.py",build);self.assertIn("generate_facades.py",build);self.assertIn("facades.json",build)
  self.assertIn("WTGFacadeInstanceCluster",editor);self.assertIn("WROCLAW_FACADE_INSTANCES",editor);self.assertIn("official_replaced",editor)
  self.assertIn("UHierarchicalInstancedStaticMeshComponent",cluster_h);self.assertIn("AddFacadeInstance",cluster_h);self.assertIn("AddInstance(WorldTransform, true)",cluster_cpp)

if __name__=="__main__":unittest.main()
