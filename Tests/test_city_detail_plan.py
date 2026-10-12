import copy
import json
import math
import sys
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"Scripts/assets"))
from build_city_detail_plan import build


class DetailPlanTests(unittest.TestCase):
    def setUp(self):
        self.policy=json.loads((ROOT/"Data/city_detail_policy.json").read_text(encoding="utf-8"))
        self.district={"delivery_route":[[0,0,0]],"anchors":{"meeting":[0,0,0]}}
        def item(id,kind,**fields): return dict(id=id,kind=kind,world_position=[500,200,1000],yaw_deg=90,**fields)
        self.facades={"buildings":[{"building_id":"way/42","details":[
            item("balcony","balcony",width_m=1.8,depth_m=1.1,railing_height_m=1.05),
            item("cornice","cornice",length_m=7,depth_m=.25)],
            "openings":[item("door","door",width_m=1.4,height_m=2.35)]}]}
        self.facades["buildings"][0]["details"][0]["world_position"][2]=1500

    def test_deterministic_complete_balcony_and_clear_door(self):
        a=build(self.facades,self.district,self.policy)
        self.assertEqual(a,build(self.facades,self.district,self.policy))
        by_role={item["role"]:item for item in a["items"]}
        railing=by_role["railing"]; slab=by_role["slab"]; entrance=by_role["entrance"]
        self.assertAlmostEqual(slab["position_cm"][2]+60,railing["position_cm"][2])
        self.assertAlmostEqual(entrance["scale"][0]*180,140,places=3)
        self.assertAlmostEqual(entrance["scale"][2]*230,235,places=3)
        self.assertEqual(by_role["door_leaf"]["scale"],entrance["scale"][:1]+[1.]+entrance["scale"][2:])
        self.assertEqual(len({item["id"] for item in a["items"]}),len(a["items"]))

    def test_modules_preserve_detail_density_and_limits(self):
        a=build(self.facades,self.district,self.policy)
        cornices=[item for item in a["items"] if item["role"]=="cornice"]
        self.assertEqual(len(cornices),3)
        self.assertTrue(all(0<item["scale"][0]<=1 for item in cornices))
        self.assertAlmostEqual(sum(item["scale"][0]*300 for item in cornices),670,places=3)
        policy=copy.deepcopy(self.policy); policy["limits"]["cornice_segments"]=2
        self.assertNotIn("cornice",build(self.facades,self.district,policy)["counts"])

    def test_outside_route_and_nonfinite_transforms_rejected(self):
        district={"delivery_route":[[1e6,1e6,0]],"anchors":{"meeting":[1e6,1e6,0]}}
        with self.assertRaises(ValueError): build(self.facades,district,self.policy)
        facades=copy.deepcopy(self.facades); facades["buildings"][0]["details"][0]["world_position"][2]=math.nan
        with self.assertRaises(ValueError): build(facades,self.district,self.policy)

    def test_portal_crown_not_buried_in_balcony(self):
        facades=copy.deepcopy(self.facades)
        facades["buildings"][0]["details"][0]["world_position"][2]=1200
        roles=build(facades,self.district,self.policy)["counts"]
        self.assertNotIn("entrance",roles)
        self.assertNotIn("door_leaf",roles)


if __name__=="__main__": unittest.main()
