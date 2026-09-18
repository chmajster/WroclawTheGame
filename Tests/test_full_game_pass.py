import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class FullGamePassIntegrationTests(unittest.TestCase):
    def read(self, relative):
        return (ROOT / relative).read_text(encoding="utf-8")

    def test_full_game_build_enables_city_and_campaign(self):
        text = self.read("Scripts/Build-FullGame.ps1")
        self.assertIn("City=$true", text)
        self.assertIn("Campaign=$true", text)
        self.assertIn("Build-Geography.ps1", text)

    def test_windows_build_keeps_version_and_asset_pipeline_gates(self):
        text = self.read("Scripts/Build-Windows.ps1")
        self.assertIn("Assert-UnrealVersion.ps1", text)
        self.assertIn("ExpectedVersion '5.8'", text)
        self.assertIn("Pipeline\\qa\\validate_manifest.py", text)
        self.assertIn("import_free_animations.py", text)

    def test_phone_map_and_vehicle_security_are_wired(self):
        map_cpp = self.read("Source/WroclawTheGame/Systems/WroclawMapSubsystem.cpp")
        city_cpp = self.read("Source/WroclawTheGame/Systems/CityGameplaySubsystem.cpp")
        self.assertIn("PhoneMapText", map_cpp)
        self.assertIn("CycleWaypoint", map_cpp)
        self.assertIn("UVehiclePursuitSubsystem", city_cpp)
        self.assertIn("URoadBlockSubsystem", city_cpp)

    def test_campaign_migration_and_six_scenarios_remain_present(self):
        migration = self.read("Scripts/gis/migrate_campaign.py")
        routes = self.read("Scripts/gis/make_routes.py")
        self.assertIn("--require-playable", migration)
        for mode in ("Escape", "Follow", "Navigation", "Delivery"):
            self.assertIn(mode, routes)


if __name__ == "__main__":
    unittest.main()
