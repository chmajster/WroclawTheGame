import ast
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class VehicleScenarioGeneratorTests(unittest.TestCase):
    def test_six_distinct_vehicle_scenarios_are_declared(self):
        path = ROOT / "Scripts" / "gis" / "make_routes.py"
        tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
        specs = None
        for node in tree.body:
            if isinstance(node, ast.Assign) and any(
                isinstance(target, ast.Name) and target.id == "SPECS" for target in node.targets
            ):
                specs = ast.literal_eval(node.value)
                break
        self.assertIsNotNone(specs)
        self.assertEqual(len(specs), 6)
        ids = [item[0] for item in specs]
        modes = [item[3] for item in specs]
        self.assertEqual(len(ids), len(set(ids)))
        self.assertIn("Escape", modes)
        self.assertIn("Follow", modes)
        self.assertIn("Navigation", modes)
        self.assertIn("Delivery", modes)
        self.assertGreaterEqual(modes.count("TimeTrial"), 2)

    def test_unreal_import_populates_mode_specific_fields(self):
        text = (ROOT / "Scripts" / "prepare_geography.py").read_text(encoding="utf-8")
        for field in (
            "mode",
            "pursuer_count",
            "min_follow_distance",
            "max_follow_distance",
            "lost_target_time",
            "navigation_hints",
        ):
            self.assertIn(f"set_editor_property('{field}'", text)


if __name__ == "__main__":
    unittest.main()
