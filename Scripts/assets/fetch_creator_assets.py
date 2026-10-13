"""Acquire only reviewed CC0 graphical assets; never download upstream program code."""
from pathlib import Path
import argparse,json,hashlib,urllib.request
ROOT=Path(__file__).resolve().parents[2]
def verify(data,item):
    if len(data)!=item['bytes'] or hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()!=item['git_blob_sha1']:
        raise ValueError('Pinned asset hash/length mismatch: '+item['path'])
def main():
    p=argparse.ArgumentParser();p.add_argument('--check',action='store_true');a=p.parse_args();records=[]
    for item in json.loads((ROOT/'Data/creator_source_catalog.json').read_text())['assets']:
        target=ROOT/item['path']
        if not target.is_file():
            if a.check:raise FileNotFoundError(target)
            req=urllib.request.Request(item['url'],headers={'User-Agent':'WTG-creator-production'})
            data=urllib.request.urlopen(req,timeout=60).read();verify(data,item)
            target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
        data=target.read_bytes();verify(data,item);records.append({'path':item['path'],'sha256':hashlib.sha256(data).hexdigest(),'bytes':len(data)})
    out=ROOT/'Saved/CreatorRebuild';out.mkdir(exist_ok=True,parents=True)
    (out/'source-checksums.json').write_text(json.dumps(records,indent=2))
    print('PASS: '+str(len(records))+' reviewed graphical assets/notices')
if __name__=='__main__':main()
