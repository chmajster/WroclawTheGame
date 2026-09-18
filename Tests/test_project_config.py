import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class ProjectConfigurationTests(unittest.TestCase):
    def test_unreal_engine_version_is_single_source_of_truth(self):
        project = json.loads((ROOT / "WroclawTheGame.uproject").read_text(encoding="utf-8"))
        self.assertEqual(project.get("EngineAssociation"), "5.8")

        active_text_files = [
            ROOT / "README.md",
            ROOT / "docs" / "ACCEPTANCE.md",
            ROOT / "docs" / "VALIDATION.md",
            ROOT / "docs" / "CITY-WAVE1.md",
            ROOT / "Scripts" / "prepare_content.py",
            ROOT / "Scripts" / "prepare_geography.py",
            ROOT / "Scripts" / "Build-Windows.ps1",
            ROOT / "Scripts" / "Build-Geography.ps1",
        ]
        for path in active_text_files:
            text = path.read_text(encoding="utf-8")
            self.assertNotIn("UE_5.6", text, path)
            self.assertNotIn("UE 5.6", text, path)
            self.assertNotIn("UE5.6", text, path)
            self.assertNotIn("Unreal 5.6", text, path)

    def test_build_entry_points_enforce_engine_version_guard(self):
        for relative in ("Scripts/Build-Windows.ps1", "Scripts/Build-Geography.ps1"):
            text = (ROOT / relative).read_text(encoding="utf-8")
            self.assertIn("Assert-UnrealVersion.ps1", text, relative)
            self.assertIn("ExpectedVersion '5.8'", text, relative)

        guard = (ROOT / "Scripts" / "Assert-UnrealVersion.ps1").read_text(encoding="utf-8")
        self.assertIn("EngineAssociation", guard)
        self.assertIn("Engine\\Build\\Build.version", guard)


if __name__ == "__main__":
    unittest.main()
