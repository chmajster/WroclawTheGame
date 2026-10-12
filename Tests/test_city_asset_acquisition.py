import base64
import copy
import hashlib
import json
import sys
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"Scripts/assets"))
from acquire_city_assets import validate_asset, verify
from audit_city_assets import audit


class AcquisitionTests(unittest.TestCase):
    def setUp(self):
        self.asset=json.loads((ROOT/"Data/city_asset_acquisition.json").read_text(encoding="utf-8"))["assets"][0]

    def test_real_registry_and_demand_are_complete(self):
        validate_asset(self.asset)
        self.assertEqual(audit()["status"],"PASS")
        for record in self.asset["files"].values():
            self.assertEqual(verify((ROOT/record["path"]).read_bytes(),record,2048),record["sha256"])

    def test_unclear_or_incompatible_license_is_rejected(self):
        for license in [None,"unknown","CC-BY-NC-4.0","CC-BY-4.0"]:
            asset=copy.deepcopy(self.asset); asset["license"]=license
            with self.assertRaises(ValueError): validate_asset(asset)

    def test_untrusted_host_or_redirect_path_is_rejected(self):
        for url in ["http://dl.polyhaven.org/file/ph-assets/Textures/x.png","https://example.com/model.png","https://dl.polyhaven.org/script.py"]:
            asset=copy.deepcopy(self.asset); asset["files"]["normal"]["url"]=url
            with self.assertRaises(ValueError): validate_asset(asset)

    def test_escape_duplicate_or_missing_map_is_rejected(self):
        asset=copy.deepcopy(self.asset); asset["files"]["normal"]["path"]="../../outside.png"
        with self.assertRaises(ValueError): validate_asset(asset)
        asset=copy.deepcopy(self.asset); asset["files"]["normal"]["path"]=asset["files"]["base_color"]["path"]
        with self.assertRaises(ValueError): validate_asset(asset)
        asset=copy.deepcopy(self.asset); del asset["files"]["orm"]
        with self.assertRaises(ValueError): validate_asset(asset)

    def test_corruption_and_wrong_resolution_are_rejected(self):
        record=self.asset["files"]["normal"]; data=(ROOT/record["path"]).read_bytes()
        with self.assertRaises(ValueError): verify(data[:-1],record,2048)
        with self.assertRaises(ValueError): verify(data,record,1024)
        record=copy.deepcopy(record); record["sha256"]="0"*64
        with self.assertRaises(ValueError): verify(data,record,2048)


if __name__=="__main__": unittest.main()
