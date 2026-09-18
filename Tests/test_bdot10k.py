import importlib.util
import json
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
PATH=ROOT/"Scripts"/"gis"/"fetch_bdot10k.py"
SPEC=importlib.util.spec_from_file_location("fetch_bdot10k",PATH)
MODULE=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(MODULE)

class BDOT10kTests(unittest.TestCase):
    def setUp(self):
        self.config=json.loads((ROOT/"Data"/"bdot10k_sources.json").read_text(encoding="utf-8"))
    def test_building_class_is_configured(self):
        names={x["class_name"] for x in self.config["default_classes"]}
        self.assertIn("OT_BUBD_A",names)
    def test_geoparquet_url_matches_official_pattern(self):
        url=MODULE.class_url(self.config,"OT_BUBD_A","geoparquet")
        self.assertEqual(url,"https://opendata.geoportal.gov.pl/bdot10k/schemat2021/GeoParquet/OT_BUBD_A.parquet")
    def test_rejects_invalid_class_name(self):
        with self.assertRaises(ValueError):
            MODULE.class_url(self.config,"../../secret","geoparquet")
    def test_all_official_thematic_groups_are_declared(self):
        self.assertEqual(set(self.config["thematic_groups"]),{"SW","SK","SU","PT","BU","KU","AD","TC","OI","RT"})

if __name__=="__main__": unittest.main()
