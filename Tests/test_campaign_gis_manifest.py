import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class CampaignGISManifestTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = json.loads((ROOT / "Data" / "campaign_gis.json").read_text(encoding="utf-8"))
        cls.chapter = json.loads((ROOT / "Data" / "chapter1.json").read_text(encoding="utf-8"))
        cls.environment = json.loads((ROOT / "Data" / "environment.json").read_text(encoding="utf-8"))

    def zones_for_x(self, x):
        return [
            zone for zone in self.manifest["zones"]
            if zone["source_min_x"] <= x < zone["source_max_x"]
        ]

    def test_manifest_covers_source_campaign_bounds_without_overlap(self):
        zones = sorted(self.manifest["zones"], key=lambda z: z["source_min_x"])
        self.assertEqual(zones[0]["source_min_x"], 0)
        self.assertGreaterEqual(zones[-1]["source_max_x"], 15800)
        for left, right in zip(zones, zones[1:]):
            self.assertEqual(left["source_max_x"], right["source_min_x"])
        self.assertEqual(len({z["id"] for z in zones}), len(zones))
        self.assertEqual(len({z["anchor_street"] for z in zones}), len(zones))

    def test_every_physical_action_maps_to_exactly_one_zone(self):
        for action in self.chapter["actions"]:
            p = action["position"]
            if p[:2] == [0, 0]:
                continue
            self.assertEqual(len(self.zones_for_x(p[0])), 1, action["id"])

    def test_every_environment_record_maps_to_exactly_one_zone(self):
        for record in self.environment:
            p = record.get("position")
            if not p:
                continue
            self.assertEqual(len(self.zones_for_x(p[0])), 1, record.get("id"))

    def test_migrator_explicitly_preserves_ids_and_reports_blockers(self):
        text = (ROOT / "Scripts" / "gis" / "migrate_campaign.py").read_text(encoding="utf-8")
        self.assertIn('"stable_ids_preserved"', text)
        self.assertIn("blockout_exceeds_real_building_footprint", text)
        self.assertIn("replace_linked_blockout_with_authored_real_interior", text)
        self.assertIn("--require-playable", text)


if __name__ == "__main__":
    unittest.main()
