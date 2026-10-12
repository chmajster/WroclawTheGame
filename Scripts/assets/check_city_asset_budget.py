"""Measured source counts; GPU/FPS claims require separate runtime profiling."""
import json
from collections import Counter
from pathlib import Path
import struct

ROOT=Path(__file__).resolve().parents[2]


def evaluate(plan,geometry,policy,quality,textures):
    source={item["id"].replace("-","_"):item["triangles"] for item in geometry["assets"]}
    counts=Counter(item["role"] for item in plan["items"])
    triangles=sum(count*source[policy["meshes"][role].split("SM_wtg_tenement_")[1]] for role,count in counts.items())
    report={"instances":len(plan["items"]),"instanced_lod0_triangles":triangles,
        "unique_source_texture_bytes":sum(t["bytes"] for t in textures),
        "rgba_full_mip_upper_bound_bytes":sum((t["width"]*t["height"]*4*4+2)//3 for t in textures),
        "texture_count":len(textures),"status":"PASS","performance":"NOT_MEASURED"}
    for key,limit in quality["budgets"].items():
        if report[key]>limit: raise ValueError(f"Batch budget exceeded: {key}={report[key]} > {limit}")
    return report


def main():
    def read(path): return json.loads((ROOT/path).read_text(encoding="utf-8"))
    texture_paths=set()
    for path in sorted((ROOT/"Pipeline/assets/tenements").glob("*.asset.json")):
        for source in json.loads(path.read_text(encoding="utf-8"))["textures"].values():
            if source: texture_paths.add(source)
    textures=[]
    for source in sorted(texture_paths):
        data=(ROOT/source).read_bytes()
        if data[:8]!=b"\x89PNG\r\n\x1a\n": raise ValueError("Expected PNG")
        width,height=struct.unpack(">II",data[16:24])
        textures.append({"path":source,"bytes":len(data),"width":width,"height":height})
    report=evaluate(read("Saved/CityDetails/placement.json"),read("SourceAssets/Models/Wroclaw/TenementKit/geometry.json"),read("Data/city_detail_policy.json"),read("Data/city_asset_quality.json"),textures)
    report["textures"]=textures
    (ROOT/"Saved/CityDetails/budget.json").write_text(json.dumps(report,indent=2),encoding="utf-8")
    print(json.dumps({key:value for key,value in report.items() if key!="textures"}))


if __name__=="__main__": main()
