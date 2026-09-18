import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
FETCH_PATH = ROOT / "Scripts/fetch_free_character_animation_assets.py"
SPEC = importlib.util.spec_from_file_location("free_character_assets", FETCH_PATH)
assets = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(assets)


class FreeCharacterAnimationAssets(unittest.TestCase):
    def test_pinned_sources_are_unique(self):
        targets = [target for _, target, _ in assets.SPECS]
        self.assertEqual(len(targets), len(set(targets)))
        self.assertRegex(assets.MIRROR_COMMIT, r"^[0-9a-f]{40}$")
        for _, _, blob_sha in assets.SPECS:
            self.assertRegex(blob_sha, r"^[0-9a-f]{40}$")

    def test_catalog_paths_match_fetch_targets(self):
        targets = {target for _, target, _ in assets.SPECS}
        for item in assets.character_catalog():
            self.assertIn(item["primary_path"], targets)

    def test_committed_assets_when_present(self):
        catalog = ROOT / "Data/free_animation_catalog.json"
        if not catalog.exists():
            self.skipTest("Binary fetch workflow has not populated animation catalog yet")
        entries = json.loads(catalog.read_text(encoding="utf-8"))
        self.assertEqual(len(entries), 4)
        self.assertTrue(all(item["license"] == "CC0-1.0" for item in entries))
        self.assertTrue(all(item["clip_count"] > 0 for item in entries))
        assets.check_existing()

    def test_semantic_bindings_resolve_to_real_clips(self):
        animation_catalog = ROOT / "Data/free_animation_catalog.json"
        bindings_file = ROOT / "Data/animation_bindings.json"
        if not animation_catalog.exists():
            self.skipTest("Binary fetch workflow has not populated animation catalog yet")
        catalog = json.loads(animation_catalog.read_text(encoding="utf-8"))
        bindings = json.loads(bindings_file.read_text(encoding="utf-8"))["bindings"]
        clips = {item["id"]: set(item["clips"]) for item in catalog}
        self.assertGreaterEqual(len(bindings), 30)
        for semantic, binding in bindings.items():
            self.assertIn(binding["library"], clips, semantic)
            self.assertIn(binding["clip"], clips[binding["library"]], semantic)


if __name__ == "__main__":
    unittest.main()
