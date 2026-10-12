"""Explicit review record after inspecting PNGs; never automatically approves art."""
import argparse
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--notes",required=True)
    parser.add_argument("--reviewer",required=True)
    parser.add_argument("--status",choices=["PASS_WITH_LIMITATIONS","NEEDS_FIXES"],required=True)
    parser.add_argument("--output",default="Saved/CityDetails/visual-review.json")
    args=parser.parse_args()
    views=json.loads((ROOT/"Data/city_asset_qa_views.json").read_text(encoding="utf-8"))["views"]
    screenshots={}; captures={}; native={}
    for view in views:
        report_path=ROOT/"Saved/Pipeline/screenshots"/view["id"]/"capture.json"
        report=json.loads(report_path.read_text(encoding="utf-8"))
        if report.get("status")!="PASS": raise ValueError("Capture failed")
        if len(report["screenshots"])!=len(view["qa"]["cameras"]): raise ValueError("Incomplete capture")
        for file in report["screenshots"]:
            path=ROOT/file
            sha=hashlib.sha256(path.read_bytes()).hexdigest()
            if sha!=report["sha256"][path.name]: raise ValueError("Screenshot changed after capture")
            screenshots[str(path.relative_to(ROOT)).replace('\\','/')]=sha
        if not report.get("native_files_sha256"): raise ValueError("Missing native input fingerprint; recapture")
        for file,sha in report["native_files_sha256"].items():
            if hashlib.sha256((ROOT/file).read_bytes()).hexdigest()!=sha: raise ValueError("Native asset/map changed; recapture: "+file)
            native[file]=sha
        captures[view["id"]]=report["captured_utc"]
    output=ROOT/args.output; output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text(json.dumps({"status":args.status,"reviewer":args.reviewer,"reviewed_utc":datetime.now(timezone.utc).isoformat(),
        "notes":args.notes,"captures":captures,"screenshots_sha256":screenshots,"native_files_sha256":native},indent=2),encoding="utf-8")
    production=ROOT/"Saved/CityDetails/production.json"
    if production.is_file():
        state=json.loads(production.read_text(encoding="utf-8-sig"))
        state["visual_review"]=args.status; state["visual_review_record"]=str(output.relative_to(ROOT)).replace('\\','/')
        production.write_text(json.dumps(state,indent=2),encoding="utf-8")
    print("Recorded explicit visual review: "+args.status)


if __name__=="__main__": main()
