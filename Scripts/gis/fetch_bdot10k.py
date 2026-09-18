#!/usr/bin/env python3
"""Fetch official BDOT10k class files or a Wrocław regional package with provenance."""
from __future__ import annotations

import argparse
import hashlib
import html.parser
import json
import re
import sys
import urllib.parse
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CONFIG = ROOT / "Data" / "bdot10k_sources.json"
DEFAULT_OUTPUT = ROOT / "Saved" / "BDOT10k"
USER_AGENT = "WroclawTheGame/BDOT10k (+https://github.com/chmajster/WroclawTheGame)"


class Links(html.parser.HTMLParser):
    def __init__(self):
        super().__init__()
        self.items = []
    def handle_starttag(self, tag, attrs):
        if tag.casefold() == "a":
            href = dict(attrs).get("href")
            if href:
                self.items.append(href)


def request_bytes(url: str, timeout: int = 300) -> bytes:
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT, "Accept": "*/*"})
    with urllib.request.urlopen(req, timeout=timeout) as response:
        return response.read()


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def class_url(config: dict, class_name: str, fmt: str) -> str:
    if not re.fullmatch(r"OT_[A-Z0-9_]+", class_name):
        raise ValueError(f"Invalid BDOT10k class name: {class_name}")
    template = config["class_base_urls"][fmt]
    return template.format(class_name=class_name)


def regional_links(config: dict) -> list[str]:
    probe = config["wroclaw_probe"]
    # WMS endpoint exposes package download links through GetFeatureInfo.
    params = {
        "SERVICE": "WMS", "VERSION": "1.3.0", "REQUEST": "GetFeatureInfo",
        "LAYERS": "powiaty", "QUERY_LAYERS": "powiaty",
        "CRS": "EPSG:4326",
        "BBOX": f"{probe['latitude']-0.02},{probe['longitude']-0.03},{probe['latitude']+0.02},{probe['longitude']+0.03}",
        "WIDTH": "101", "HEIGHT": "101", "I": "50", "J": "50",
        "FORMAT": "image/png", "INFO_FORMAT": "text/html",
    }
    url = config["package_wms"] + "?" + urllib.parse.urlencode(params)
    payload = request_bytes(url)
    parser = Links(); parser.feed(payload.decode("utf-8", "replace"))
    links = [urllib.parse.urljoin(url, href) for href in parser.items]
    raw = payload.decode("utf-8", "replace")
    links.extend(re.findall(r"https?://[^\"'<>\s]+(?:\.zip|\.gpkg|\.parquet)(?:\?[^\"'<>\s]*)?", raw, re.I))
    return sorted(set(links))


def download(url: str, output: Path, logical_name: str) -> dict:
    output.mkdir(parents=True, exist_ok=True)
    suffix = Path(urllib.parse.urlparse(url).path).suffix or ".bin"
    path = output / (logical_name + suffix)
    if not path.is_file():
        path.write_bytes(request_bytes(url))
    return {"url": url, "path": path.as_posix(), "sha256": sha256(path), "bytes": path.stat().st_size}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", type=Path, default=DEFAULT_CONFIG)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--class", dest="classes", action="append", help="BDOT10k class e.g. OT_BUBD_A")
    parser.add_argument("--format", choices=("geoparquet","gpkg"), default="geoparquet")
    parser.add_argument("--regional-package", action="store_true")
    args = parser.parse_args()
    config = json.loads(args.config.read_text(encoding="utf-8"))
    selected = args.classes or [r["class_name"] for r in config["default_classes"] if r.get("enabled")]
    records = []
    for class_name in selected:
        url = class_url(config, class_name, args.format)
        records.append({"kind":"class","class_name":class_name,**download(url,args.output/class_name,class_name)})
    if args.regional_package:
        links = regional_links(config)
        if not links:
            raise RuntimeError("BDOT10k WMS returned no Wrocław package links")
        for index, url in enumerate(links):
            records.append({"kind":"regional","index":index,**download(url,args.output/"regional",f"wroclaw_{index:02d}")})
    manifest = {
        "schema_version":1,
        "source":config["source"],
        "format":args.format,
        "records":records,
        "next_gate":"Parse selected BDOT10k classes, clip to active Wrocław sectors, map attributes into city semantic overlays, then compare against OSM before replacing any geometry."
    }
    args.output.mkdir(parents=True,exist_ok=True)
    (args.output/"manifest.json").write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
    print(f"Fetched {len(records)} BDOT10k products")
    return 0


if __name__=="__main__":
    try:
        raise SystemExit(main())
    except (OSError,ValueError,RuntimeError) as exc:
        print(f"FAIL {exc}",file=sys.stderr)
        raise SystemExit(1)
