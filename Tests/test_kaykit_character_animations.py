import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
FETCH_PATH = ROOT / "Scripts/fetch_kaykit_character_animations.py"
SPEC = importlib.util.spec_from_file_location("kaykit_assets", FETCH_PATH)
assets = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(assets)


class KayKitCharacterAnimations(unittest.TestCase):
    def test_manifest_is_pinned_and_complete(self):
        self.assertEqual(len(assets.SPECS), 8)
        self.assertRegex(assets.MIRROR_COMMIT, r"^[0-9a-f]{40}$")
        categories = {category for category, _, _ in assets.SPECS}
        self.assertEqual(categories, {
            "General", "MovementBasic", "MovementAdvanced", "CombatMelee",
            "CombatRanged", "Tools", "Simulation", "Special",
        })
        for _, _, blob_sha in assets.SPECS:
            self.assertRegex(blob_sha, r"^[0-9a-f]{40}$")

    def test_catalog_and_required_opening_clips_when_present(self):
        catalog_path = ROOT / "Data/free_kaykit_animation_catalog.json"
        if not catalog_path.exists():
            self.skipTest("KayKit binary materialization has not run yet")
        catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
        self.assertEqual(len(catalog), 8)
        self.assertTrue(all(item["license"] == "CC0-1.0" for item in catalog))
        self.assertTrue(all(item["clip_count"] > 0 for item in catalog))
        simulation = next(item for item in catalog if item["category"] == "Simulation")
        for required in ("Lie_Down", "Lie_Idle", "Lie_StandUp"):
            self.assertIn(required, simulation["clips"])
        assets.check_existing()


if __name__ == "__main__":
    unittest.main()
