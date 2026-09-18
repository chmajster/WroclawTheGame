"""Explicit, bounded download of six CC0 texture sets. No code/archives are executed.

Downloads are optional during authoring, never implicit in the game/build. Files are
verified against the provider's MD5 and recorded with SHA-256 plus source metadata.
"""
import hashlib
import json
from pathlib import Path
import urllib.request
from urllib.parse import urlparse

ROOT=Path(__file__).resolve().parents[1]
SETS={'Asphalt':'asphalt_02','Brick':'brick_wall_001','Plaster':'plastered_wall_02',
      'Concrete':'concrete_floor_02','Cobble':'cobblestone_floor_03','Wood':'wood_floor'}


def fetch(url):
    if urlparse(url).scheme!='https' or urlparse(url).hostname not in ('api.polyhaven.com','dl.polyhaven.org'):
        raise ValueError('Unexpected source host')
    request=urllib.request.Request(url,headers={'User-Agent':'WroclawTheGame-AssetPrep/1.0'})
    with urllib.request.urlopen(request,timeout=60) as response:
        data=response.read(24*1024*1024+1)
    if len(data)>24*1024*1024:raise ValueError('Unexpectedly large source file')
    return data


def main():
    folder=ROOT/'Saved/SurfaceSource/PolyHaven';folder.mkdir(parents=True,exist_ok=True)
    manifest={'license':'CC0-1.0','license_url':'https://polyhaven.com/license','resolution':2048,'profiles':{}}
    for name,slug in SETS.items():
        info=json.loads(fetch('https://api.polyhaven.com/info/'+slug))
        maps=json.loads(fetch('https://api.polyhaven.com/files/'+slug))
        scale=info.get('scale','')
        dimensions=info.get('dimensions',[])
        if len(dimensions)<2 or dimensions[0]<=0 or abs(dimensions[0]-dimensions[1])>1:
            raise ValueError('Verify non-square or unspecified physical scale: '+slug+' '+scale)
        record={'slug':slug,'page':'https://polyhaven.com/a/'+slug,'authors':info.get('authors',{}),
                'legacy_scale_label':scale,'dimensions_mm':dimensions,'tile_cm':dimensions[0]/10,'files':{}}
        for suffix,channel in [('BC','Diffuse'),('N','nor_dx'),('ARM','arm')]:
            source=maps[channel]['2k']['png']
            path=folder/f'T_{name}_{suffix}_CC0.png'
            data=path.read_bytes() if path.exists() else fetch(source['url'])
            if hashlib.md5(data).hexdigest()!=source['md5']:
                raise ValueError(f"Source checksum mismatch: {slug} {suffix}; received {len(data)} of {source['size']} bytes; magic={data[:8]!r}")
            if not data.startswith(b'\x89PNG\r\n\x1a\n'):raise ValueError('Expected PNG')
            if not path.exists():path.write_bytes(data)
            record['files'][suffix]={'file':path.name,'url':source['url'],'md5':source['md5'],
                                      'sha256':hashlib.sha256(data).hexdigest(),'bytes':len(data)}
        manifest['profiles'][name]=record
        print('Verified',slug,scale,flush=True)
    (ROOT/'Data/surface_sources.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')


if __name__=='__main__':main()
