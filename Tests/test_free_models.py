import ast
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class FreeModelCoverage(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.chapter = json.loads((ROOT / "Data/chapter1.json").read_text(encoding="utf-8"))
        cls.poly = json.loads((ROOT / "Data/free_model_catalog.json").read_text(encoding="utf-8"))
        cls.external = json.loads((ROOT / "Data/free_external_model_catalog.json").read_text(encoding="utf-8"))
        cls.characters = json.loads((ROOT / "Data/free_character_catalog.json").read_text(encoding="utf-8"))
        cls.bindings = json.loads((ROOT / "Data/model_bindings.json").read_text(encoding="utf-8"))

    def test_catalog_ids_are_unique_and_cc0(self):
        poly_ids = [x["slug"] for x in self.poly]
        external_ids = [x["id"] for x in self.external]
        character_ids = [x["id"] for x in self.characters]
        self.assertEqual(len(poly_ids), len(set(poly_ids)))
        self.assertEqual(len(external_ids), len(set(external_ids)))
        self.assertEqual(len(character_ids), len(set(character_ids)))
        self.assertFalse(set(poly_ids) & set(external_ids))
        self.assertFalse((set(poly_ids) | set(external_ids)) & set(character_ids))
        for item in self.poly + self.external + self.characters:
            self.assertEqual(item["license"], "CC0-1.0")

    def test_primary_model_sources_exist(self):
        for item in self.poly + self.external + self.characters:
            source = ROOT / item["primary_path"]
            self.assertTrue(source.is_file(), f"Missing model source: {source}")
            self.assertGreater(source.stat().st_size, 0, f"Empty model source: {source}")

    def test_every_inventory_item_has_model(self):
        item_ids = {x["id"] for x in self.chapter["items"]}
        self.assertEqual(item_ids, set(self.bindings["items"]))

    def test_every_physical_chapter_action_has_model(self):
        physical = {
            a["id"]
            for a in self.chapter["actions"]
            if a["kind"] not in ("virtual", "zone")
            and a.get("position", [0, 0])[:2] != [0, 0]
        }
        self.assertEqual(physical, set(self.bindings["actions"]))

    def test_all_bindings_reference_catalogued_models(self):
        available = ({x["slug"] for x in self.poly} | {x["id"] for x in self.external} | {x["id"] for x in self.characters})
        referenced = (
            set(self.bindings["items"].values())
            | set(self.bindings["actions"].values())
            | set(self.bindings["systems"].values())
        )
        self.assertFalse(referenced - available, f"Unknown bound models: {sorted(referenced - available)}")

    def test_required_system_placeholders_have_replacements(self):
        required = {
            "player_fallback",
            "player_fallback_female",
            "resident_npc",
            "enemy_guard",
            "ambient_pedestrian",
            "ambient_vehicle",
            "driveable_vehicle_body",
            "driveable_vehicle_wheel",
            "city_interior_door",
            "surveillance_camera",
            "cctv_monitor",
            "hide_container",
            "hide_shelf",
            "street_lamp",
            "city_interior_sofa",
            "city_interior_table",
            "city_interior_chair",
            "city_interior_cabinet",
            "city_interior_shelf",
            "city_interior_lamp",
            "city_activity_marker",
        }
        self.assertEqual(required, set(self.bindings["systems"]))


    def test_runtime_gameplay_classes_do_not_render_engine_cubes(self):
        sources = [
            "Source/WroclawTheGame/Interaction/SliceProp.cpp",
            "Source/WroclawTheGame/World/CityActivity.cpp",
            "Source/WroclawTheGame/World/CityInteriorDoor.cpp",
            "Source/WroclawTheGame/World/WorldInteraction.cpp",
            "Source/WroclawTheGame/World/ResidentNPC.cpp",
            "Source/WroclawTheGame/World/CityPopulation.cpp",
            "Source/WroclawTheGame/AI/SliceEnemy.cpp",
            "Source/WroclawTheGame/Vehicles/DriveableVehicle.cpp",
            "Source/WroclawTheGame/Character/SliceCharacter.cpp",
        ]
        for source in sources:
            text = (ROOT / source).read_text(encoding="utf-8")
            self.assertNotIn("/Engine/BasicShapes/Cube", text, source)

    def test_humanoid_bindings_use_rigged_character_catalog(self):
        character_ids = {x["id"] for x in self.characters if x.get("category") == "body"}
        systems = self.bindings["systems"]
        for key in ("player_fallback", "player_fallback_female", "resident_npc", "enemy_guard", "ambient_pedestrian"):
            self.assertIn(systems[key], character_ids, key)
        self.assertIn(self.bindings["actions"]["neighbor_help"], character_ids)

    def test_model_generation_scripts_parse(self):
        for script in ("Scripts/prepare_content.py", "Scripts/prepare_geography.py", "Scripts/prepare_character_creator.py"):
            source = (ROOT / script).read_text(encoding="utf-8")
            ast.parse(source, filename=script)


if __name__ == "__main__":
    unittest.main()
