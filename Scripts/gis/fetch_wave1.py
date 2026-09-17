"""Explicit source acquisition. Builds remain offline; never run this from packaging.
OSM API tiles are merged by source identity, not geometric proximity.
"""
import argparse
import csv
from datetime import datetime, timezone
import gzip
import hashlib
import io
import json
from pathlib import Path
import struct
import urllib.request
import urllib.error
import time
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
BBOX = [16.995, 51.065, 17.060, 51.126]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def download(url):
    request = urllib.request.Request(url, headers={'User-Agent': 'WroclawTheGame-GIS/0.4 (source import)'})
    cache = ROOT/'Saved/SourceDownloads'/datetime.now(timezone.utc).date().isoformat()
    cache.mkdir(parents=True, exist_ok=True)
    path = cache/hashlib.sha256(url.encode()).hexdigest()
    if path.exists(): return path.read_bytes()
    for attempt in range(3):
        try:
            with urllib.request.urlopen(request, timeout=90) as response:
                data = response.read()
            path.write_bytes(data)
            return data
        except urllib.error.HTTPError as error:
            if error.code < 500 and error.code != 429: raise
            if attempt == 2: raise
        except (urllib.error.URLError, TimeoutError):
            if attempt == 2: raise
        time.sleep(2 ** attempt)


def fetch(destination):
    date = datetime.now(timezone.utc).date().isoformat()
    elements = {}; tiles = []
    west, south, east, north = BBOX
    def tile(bbox, depth=0):
        url = 'https://www.openstreetmap.org/api/0.6/map?bbox=' + ','.join(f'{v:.9f}' for v in bbox)
        try:
            raw = download(url)
        except urllib.error.HTTPError as error:
            reason = error.read().decode(errors='replace')
            if error.code != 400 or depth >= 4 or 'too many nodes' not in reason.lower():
                raise ValueError(f'OSM response {error.code}: {reason}') from error
            w,s,e,n = bbox; x = (w+e)/2; y = (s+n)/2
            for child in ([w,s,x,y], [x,s,e,y], [w,y,x,n], [x,y,e,n]): tile(child, depth+1)
            return
        document = ET.fromstring(raw)
        if document.tag != 'osm' or document.find('error') is not None:
            raise ValueError('Invalid OSM response')
        tiles.append({'SourceURL': url, 'SHA256': digest(raw)})
        for item in document:
            if item.tag not in ('node', 'way', 'relation'): continue
            key = item.tag, item.attrib['id']; old = elements.get(key)
            if old is None or int(item.get('version', '0')) > int(old.get('version', '0')):
                elements[key] = item
        print(f'Fetched OSM source tile {len(tiles)}', flush=True)
    # Keep each public API request small and subdivide dense cells when necessary.
    for row in range(7):
        for column in range(2):
            tile([west+(east-west)*column/2, south+(north-south)*row/7,
                  west+(east-west)*(column+1)/2, south+(north-south)*(row+1)/7])
    root = ET.Element('osm', version='0.6', generator='WroclawTheGame fetch_wave1.py')
    for key in sorted(elements):
        root.append(elements[key])
    xml = ET.tostring(root, encoding='utf-8', xml_declaration=True)
    compressed = gzip.compress(xml, mtime=0)
    osm = destination/'osm'; osm.mkdir(parents=True, exist_ok=True)
    parts = []
    for index, start in enumerate(range(0, len(compressed), 7000002), 1):
        name = f'wave1.osm.gz.part{index:03d}'; raw = compressed[start:start+7000002]
        (osm/name).write_bytes(raw)
        parts.append({'File': name, 'SHA256': digest(raw)})
    info = {'SourceName': 'OpenStreetMap contributors', 'SourceURL': 'https://www.openstreetmap.org',
            'SourceDate': date, 'SourceLicense': 'ODbL-1.0', 'LicenseURL': 'https://www.openstreetmap.org/copyright',
            'ImportDate': date, 'CRS': 'EPSG:4326', 'ProcessingVersion': '2', 'SHA256': digest(compressed),
            'BoundingBox': BBOX, 'UncompressedSHA256': digest(xml), 'StorageFormat': 'gzip-compressed OSM XML',
            'SourceParts': parts, 'SourceTiles': tiles, 'MergePolicy': 'Highest source version per OSM type/id; single retrieval session'}
    (osm/'wave1.metadata.json').write_text(json.dumps(info, indent=2)+'\n')
    url = 'https://s3.amazonaws.com/elevation-tiles-prod/skadi/N51/N51E017.hgt.gz'
    # Western strip requires its adjacent source tile as well.
    archives = {}
    for longitude in (16, 17):
        tile_url = f'https://s3.amazonaws.com/elevation-tiles-prod/skadi/N51/N51E{longitude:03d}.hgt.gz'
        archive = download(tile_url); data = gzip.decompress(archive)
        if len(data) != 3601*3601*2:
            raise ValueError('Unexpected Skadi grid size')
        archives[longitude] = data
        tilespec = {'SourceURL': tile_url, 'SHA256': digest(archive)}
        if longitude == 16: terrain_sources = []
        terrain_sources.append(tilespec)
    # One-arc-second grid, padded by one sample for edge interpolation.
    import math
    stream = io.StringIO(newline=''); writer = csv.writer(stream, lineterminator='\n')
    writer.writerow(['longitude','latitude','elevation'])
    for lat_index in range(math.floor(south*3600)-1, math.ceil(north*3600)+2):
        latitude = lat_index/3600
        row = 3600-(lat_index-51*3600)
        for lon_index in range(math.floor(west*3600)-1, math.ceil(east*3600)+2):
            longitude = lon_index/3600; tile_lon = math.floor(longitude)
            column = lon_index-tile_lon*3600
            height = struct.unpack_from('>h', archives[tile_lon], (row*3601+column)*2)[0]
            if height == -32768:
                raise ValueError('Terrain void requires explicit repair')
            writer.writerow([f'{longitude:.10f}',f'{latitude:.10f}',height])
    raw = stream.getvalue().encode()
    terrain = destination/'terrain'; terrain.mkdir(parents=True, exist_ok=True)
    (terrain/'wave1.csv').write_bytes(raw)
    info = {'SourceName': 'Mapzen Terrain Tiles (Skadi; Europe EU-DEM / USGS sources)', 'SourceURL': url,
            'SourceDate': date, 'SourceLicense': 'Terrain-Tiles-provider-terms',
            'LicenseURL': 'https://github.com/tilezen/joerd/blob/master/docs/attribution.md',
            'ImportDate': date, 'CRS': 'EPSG:4326', 'VerticalCRS': 'EGM96 orthometric metres',
            'ProcessingVersion': '2', 'SHA256': digest(raw), 'SourceArchives': terrain_sources,
            'GridSize': 3601, 'Attribution': 'Mapzen; Copernicus EU-DEM; USGS SRTM/GMTED2010'}
    (terrain/'wave1.metadata.json').write_text(json.dumps(info, indent=2)+'\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--download', action='store_true', required=True)
    parser.add_argument('--destination', type=Path, default=ROOT/'Data/source/wroclaw')
    args = parser.parse_args(); fetch(args.destination)
