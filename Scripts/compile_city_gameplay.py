"""Compile stable city activity definitions shared by native runtime and portable tests."""
import argparse
import json
import math
import re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

def validate(data):
    if data.get('schema_version')!=1:raise ValueError('Unsupported city gameplay schema')
    rows=data['actions'];ids={r['id'] for r in rows}
    if len(ids)!=len(rows):raise ValueError('Duplicate city action')
    for r in rows:
        if not re.fullmatch(r'[a-z0-9._-]{1,96}',r['id']):raise ValueError('Unsafe persistent ID')
        if not set(r['requires'])<=ids or r['id'] in r['requires']:raise ValueError('Unknown/circular prerequisite')
        if r['kind'] not in ('clue','finish','secret','event'):raise ValueError('Invalid city activity')
        if not math.isfinite(r['seconds']) or not 0<=r['seconds']<=600:raise ValueError('Invalid duration')
        if not r['node'] or not r['building']:raise ValueError('Unanchored city content')
    by_id={r['id']:r for r in rows}
    def visit(key,trail):
        if key in trail:raise ValueError('Prerequisite cycle')
        for dep in by_id[key]['requires']:visit(dep,trail|{key})
    for key in ids:visit(key,set())

def generate(data):
    validate(data)
    def val(v):
        if isinstance(v,str):return json.dumps(v,ensure_ascii=False)
        if isinstance(v,bool):return str(v).lower()
        if isinstance(v,list):return '{'+','.join(map(val,v))+'}'
        return str(v)
    fields=['id','sector','title','body','kind','node','building','requires','seconds','vehicle','crouch']
    lines=['// Generated from Data/city_gameplay.json; do not edit.','#pragma once','#include <string>','#include <vector>','namespace Wroclaw {',
    'struct CityActionDef { std::string id,sector,title,body,kind,node,building; std::vector<std::string> prerequisites; double seconds; bool vehicle,crouch; };',
    'inline const std::vector<CityActionDef>& CityActions(){static const std::vector<CityActionDef> data={']
    lines+=['{'+','.join(val(r[f]) for f in fields)+'},' for r in data['actions']]
    return '\n'.join(lines+['};return data;}','}'])+'\n'
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--check',action='store_true');args=p.parse_args()
    result=generate(json.loads((ROOT/'Data/city_gameplay.json').read_text(encoding='utf-8')))
    path=ROOT/'Source/WroclawTheGame/Content/CityGameplayCatalog.h'
    if args.check:
        if path.read_text(encoding='utf-8')!=result:raise SystemExit('Stale CityGameplayCatalog.h')
    else:path.write_text(result,encoding='utf-8')
