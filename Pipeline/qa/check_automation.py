"""Reject empty, failed or incomplete native Automation reports."""
import argparse
import json
from pathlib import Path


def validate(report,filter_name):
    tests=[test for test in report.get("tests",[]) if filter_name in test.get("fullTestPath","")]
    if not tests: raise ValueError("Automation filter selected no reported tests: "+filter_name)
    if any(test.get("state")!="Success" for test in tests): raise ValueError("Automation tests failed or incomplete")
    if report.get("failed",0): raise ValueError("Automation report has failures")
    return len(tests)


if __name__=="__main__":
    parser=argparse.ArgumentParser(); parser.add_argument("directory"); parser.add_argument("filter")
    args=parser.parse_args()
    report=json.loads((Path(args.directory)/"index.json").read_text(encoding="utf-8-sig"))
    print("PASS "+str(validate(report,args.filter))+" actually executed native tests")
