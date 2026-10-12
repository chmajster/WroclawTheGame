import copy
import json
import sys
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"Pipeline/qa"))
from validate_manifest import ManifestError, validate_manifest
from check_automation import validate as validate_automation


class PipelineTests(unittest.TestCase):
    def setUp(self):
        self.asset=json.loads((ROOT/"Pipeline/assets/tenements/ornate-balcony-railing.asset.json").read_text(encoding="utf-8"))

    def test_all_production_manifests_and_new_authored_collision(self):
        for path in (ROOT/"Pipeline/assets").rglob("*.asset.json"):
            validate_manifest(json.loads(path.read_text(encoding="utf-8")))
        self.assertEqual(self.asset["collision"]["mode"],"authored")

    def test_path_escape_id_empty_license_and_unknown_field(self):
        for key,value in [("id","../../unsafe"),("unknown",True)]:
            asset=copy.deepcopy(self.asset); asset[key]=value
            with self.assertRaises(ManifestError): validate_manifest(asset,check_files=False)
        asset=copy.deepcopy(self.asset); asset["source"]["license"]=""
        with self.assertRaises(ManifestError): validate_manifest(asset,check_files=False)

    def test_duplicate_negative_noninteger_or_unscoped_materials(self):
        for bindings in [[{"slot":0,"asset":"/Game/SurfaceQuality/M"}]*2,
                [{"slot":-1,"asset":"/Game/M"}],[{"slot":True,"asset":"/Game/M"}],
                [{"slot":0,"asset":"/Engine/M"}],[{"slot":0,"asset":"/Game/M","typo":1}]]:
            asset=copy.deepcopy(self.asset); asset["materials"]=bindings
            with self.assertRaises(ManifestError): validate_manifest(asset,check_files=False)
        asset=copy.deepcopy(self.asset); asset["materials"]=[{"slot":1,"asset":"/Game/SurfaceQuality/Instances/MI_Stone"}]
        validate_manifest(asset,check_files=False)

    def test_non_decreasing_lods_are_rejected(self):
        asset=copy.deepcopy(self.asset); asset["lod"]["ratios"]=[1,.5,.5]
        with self.assertRaises(ManifestError): validate_manifest(asset,check_files=False)

    def test_empty_failed_or_incomplete_native_reports_are_rejected(self):
        for state in [None,"Fail","InProcess","NotRun"]:
            report={"tests":[{"fullTestPath":"WTG.Assets.TenementKit","state":state}]}
            with self.assertRaises(ValueError): validate_automation(report,"WTG.Assets.TenementKit")
        with self.assertRaises(ValueError): validate_automation({"tests":[]},"WTG.Assets.TenementKit")
        self.assertEqual(validate_automation({"tests":[{"fullTestPath":"WTG.Assets.TenementKit","state":"Success"}]},"WTG.Assets.TenementKit"),1)


if __name__=="__main__": unittest.main()
