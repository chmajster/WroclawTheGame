import ast
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class RuntimeAssetQASourceTests(unittest.TestCase):
    def test_policy_covers_all_requested_gates(self):
        policy = json.loads((ROOT / "Data/runtime_asset_qa.json").read_text(encoding="utf-8"))
        self.assertTrue(policy["static_mesh"]["require_simple_collision"])
        self.assertTrue(policy["static_mesh"]["require_material"])
        self.assertGreaterEqual(len(policy["static_mesh"]["lod_rules"]), 3)
        self.assertGreaterEqual(policy["skeletal_mesh"]["required_lods"], 2)
        self.assertIn("body", policy["skeletal_mesh"]["require_physics_asset_for_categories"])
        self.assertTrue(policy["character"]["require_ik_retargeter"])
        self.assertTrue(policy["character"]["require_fbik"])
        self.assertTrue(policy["character"]["visual_review_required"])
        self.assertIn("opening.stand_up", policy["character"]["visual_qa_poses"])
        self.assertIn("character.dodge_forward", policy["character"]["visual_qa_poses"])

    def test_ikrig_plugin_is_enabled(self):
        project = json.loads((ROOT / "WroclawTheGame.uproject").read_text(encoding="utf-8"))
        plugins = {item["Name"]: item.get("Enabled", False) for item in project["Plugins"]}
        self.assertTrue(plugins.get("IKRig"))

    def test_all_runtime_qa_scripts_parse(self):
        scripts = (
            "Scripts/prepare_runtime_asset_quality.py",
            "Scripts/prepare_animation_retargeting.py",
            "Scripts/validate_runtime_asset_scene.py",
            "Scripts/validate_geography_asset_scene.py",
            "Scripts/capture_runtime_character_qa.py",
            "Scripts/record_runtime_asset_visual_review.py",
            "Scripts/build_runtime_asset_qa_report.py",
        )
        for script in scripts:
            ast.parse((ROOT / script).read_text(encoding="utf-8"), filename=script)

    def test_windows_build_orders_asset_gates_before_packaging(self):
        text = (ROOT / "Scripts/Build-Windows.ps1").read_text(encoding="utf-8")
        ordered = [
            "import_free_models.py",
            "prepare_runtime_asset_quality.py",
            "import_free_animations.py",
            "prepare_animation_retargeting.py",
            "prepare_character_creator.py",
            "prepare_content.py",
            "validate_runtime_asset_scene.py",
            "capture_runtime_character_qa.py",
            "build_runtime_asset_qa_report.py",
            "BuildCookRun",
        ]
        positions = [text.index(token) for token in ordered]
        self.assertEqual(positions, sorted(positions))
        self.assertIn("visual_review.json", text)
        self.assertIn("RuntimeAssetQAPass.ok", text)

    def test_geography_package_requires_gis_and_visual_qa(self):
        text = (ROOT / "Scripts/Build-Geography.ps1").read_text(encoding="utf-8")
        self.assertLess(text.index("prepare_geography.py"), text.index("validate_geography_asset_scene.py"))
        self.assertLess(text.index("validate_geography_asset_scene.py"), text.index("BuildCookRun"))
        self.assertIn("capture_runtime_character_qa.py", text)
        self.assertIn("--require-geography", text)
        self.assertIn("RuntimeAssetQAPass.ok", text)

    def test_character_visual_poses_resolve_to_animation_bindings(self):
        policy = json.loads((ROOT / "Data/runtime_asset_qa.json").read_text(encoding="utf-8"))
        bindings = json.loads((ROOT / "Data/animation_bindings.json").read_text(encoding="utf-8"))["bindings"]
        for semantic in policy["character"]["visual_qa_poses"]:
            self.assertIn(semantic, bindings)

    def test_cross_rig_bindings_are_explicit(self):
        bindings = json.loads((ROOT / "Data/animation_bindings.json").read_text(encoding="utf-8"))["bindings"]
        flagged = {name for name, value in bindings.items() if value.get("requires_retarget")}
        self.assertIn("opening.stand_up", flagged)
        self.assertIn("character.dodge_forward", flagged)
        self.assertGreaterEqual(len(flagged), 10)

    def test_final_report_requires_fresh_visual_review(self):
        text = (ROOT / "Scripts/build_runtime_asset_qa_report.py").read_text(encoding="utf-8")
        self.assertIn('"visual_review"', text)
        self.assertIn("capture_fingerprint", text)
        self.assertIn("visual review is stale", text)


if __name__ == "__main__":
    unittest.main()
