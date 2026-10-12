"""Acquire only reviewed, pinned CC0 files. Offline verification is the default.

Powered by Poly Haven. Registry metadata was selected using its public API;
execution uses pinned download URLs, never silently picks a new asset/license.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import urllib.request
from pathlib import Path
from urllib.parse import urlparse

ROOT=Path(__file__).resolve().parents[2]
REGISTRY=ROOT/"Data/city_asset_acquisition.json"
LIMIT=32*1024*1024


def validate_asset(asset):
    if asset.get("provider")!="Poly Haven" or asset.get("license")!="CC0-1.0":
        raise ValueError("Only reviewed Poly Haven CC0 assets are supported")
    if asset.get("license_url")!="https://polyhaven.com/license" or not asset.get("authors") or not asset.get("license_verified_on"):
        raise ValueError("Missing reviewed provenance")
    if set(asset.get("files",{}))!={"base_color","normal","orm"}:
        raise ValueError("Complete BC/DirectX normal/ORM set required")
    paths=set()
    for record in asset["files"].values():
        path=(ROOT/record["path"]).resolve()
        if not path.is_relative_to(ROOT/"SourceAssets/Textures/PolyHaven") or path.suffix!=".png" or path in paths:
            raise ValueError("Invalid or duplicate destination")
        paths.add(path)
        url=urlparse(record["url"])
        if url.scheme!="https" or url.hostname!="dl.polyhaven.org" or not url.path.startswith("/file/ph-assets/Textures/"):
            raise ValueError("Unreviewed download host/path")
        if not 0<record["bytes"]<=LIMIT:
            raise ValueError("File exceeds download budget")


def verify(data,record,resolution):
    if len(data)!=record["bytes"] or hashlib.md5(data).hexdigest()!=record["md5"]:
        raise ValueError("Pinned provider checksum/size mismatch")
    if data[:8]!=b"\x89PNG\r\n\x1a\n" or len(data)<24 or struct.unpack(">II",data[16:24])!=(resolution,resolution):
        raise ValueError("Expected square PNG at the reviewed resolution")
    sha=hashlib.sha256(data).hexdigest()
    if record.get("sha256") and record["sha256"]!=sha:
        raise ValueError("SHA-256 lock mismatch")
    return sha


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--download",action="store_true",help="Download missing pinned files only")
    args=parser.parse_args()
    registry=json.loads(REGISTRY.read_text(encoding="utf-8"))
    print(registry["credit"])
    report=[]
    for asset in registry["assets"]:
        validate_asset(asset)
        for channel,record in asset["files"].items():
            path=ROOT/record["path"]
            reused=path.is_file()
            if reused: data=path.read_bytes()
            elif args.download:
                request=urllib.request.Request(record["url"],headers={"User-Agent":"WroclawTheGame-CityAssets/1.0"})
                with urllib.request.urlopen(request,timeout=60) as response:
                    if urlparse(response.url).hostname!="dl.polyhaven.org": raise ValueError("Unexpected redirect")
                    data=response.read(LIMIT+1)
            else: raise FileNotFoundError(str(path)+"; use --download explicitly")
            sha=verify(data,record,asset["resolution"])
            if not reused:
                path.parent.mkdir(parents=True,exist_ok=True)
                temporary=path.with_suffix(".png.part"); temporary.write_bytes(data); temporary.replace(path)
            record["sha256"]=sha
            report.append({"id":asset["id"],"channel":channel,"path":record["path"],"sha256":sha,"bytes":len(data),"reused":reused})
    # Persist the independently computed SHA-256 locks after every file passed.
    REGISTRY.write_text(json.dumps(registry,indent=2)+"\n",encoding="utf-8")
    output=ROOT/"Saved/Pipeline/acquisition.json"; output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text(json.dumps({"status":"PASS","files":report},indent=2),encoding="utf-8")
    print(f"PASS {len(report)} verified files; {sum(r['reused'] for r in report)} reused")


if __name__=="__main__": main()
