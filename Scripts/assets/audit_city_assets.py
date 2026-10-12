"""Check demand against existing production manifests and downloaded catalogs."""
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]


def audit():
    needs=json.loads((ROOT/"Data/city_asset_needs.json").read_text(encoding="utf-8"))
    catalog=json.loads((ROOT/"Data/free_model_catalog.json").read_text(encoding="utf-8"))
    models={item["slug"]:item for item in catalog}
    registry=json.loads((ROOT/"Data/city_asset_acquisition.json").read_text(encoding="utf-8"))
    acquired={item["id"]:item for item in registry["assets"]}
    results=[]
    for role in needs["roles"]:
        if role.get("existing_slug"):
            item=models[role["existing_slug"]]; paths=[item["primary_path"]]
        elif role.get("manifest"):
            manifest=ROOT/role["manifest"]
            item=json.loads(manifest.read_text(encoding="utf-8")); paths=[item["source"]["model"]]
        else:
            item=acquired[role["acquisition_id"]]; paths=[r["path"] for r in item["files"].values()]
        if not all((ROOT/path).is_file() for path in paths): raise FileNotFoundError(role["role"]+": missing source")
        results.append({"role":role["role"],"decision":role["status"],"source_paths":paths})
    return {"status":"PASS","roles":results,"priority_areas":needs["priority_areas"],"existing_polyhaven_models":len(models)}


if __name__=="__main__":
    report=audit(); output=ROOT/"Saved/Pipeline/needs-audit.json"
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text(json.dumps(report,indent=2),encoding="utf-8")
    print(f"PASS {len(report['roles'])} roles; reuse library of {report['existing_polyhaven_models']} models; hero gaps remain explicit")
