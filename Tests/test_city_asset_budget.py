import copy
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"Scripts/assets"))
from check_city_asset_budget import evaluate


class BudgetTests(unittest.TestCase):
    def setUp(self):
        self.plan={"items":[{"role":"slab"}]*10}
        self.geometry={"assets":[{"id":"balcony-slab","triangles":2000}]}
        self.policy={"meshes":{"slab":"/Game/Test/SM_wtg_tenement_balcony_slab"}}
        self.quality={"budgets":{"instances":10,"instanced_lod0_triangles":20000,"unique_source_texture_bytes":20,"rgba_full_mip_upper_bound_bytes":100}}
    def test_reused_texture_counted_once_and_instance_triangle_cost(self):
        report=evaluate(self.plan,self.geometry,self.policy,self.quality,[{"bytes":20,"width":2,"height":2}])
        self.assertEqual(report["instanced_lod0_triangles"],20000)
        self.assertEqual(report["rgba_full_mip_upper_bound_bytes"],22)
        self.assertEqual(report["performance"],"NOT_MEASURED")
    def test_exceeded_cap_is_not_success(self):
        plan=copy.deepcopy(self.plan); plan["items"].append({"role":"slab"})
        with self.assertRaises(ValueError): evaluate(plan,self.geometry,self.policy,self.quality,[])
