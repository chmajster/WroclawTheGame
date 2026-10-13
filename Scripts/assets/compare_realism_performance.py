"""Summarize actual native route reports; preserve samples and measurement scope."""
import argparse
import hashlib
import json
import statistics
from pathlib import Path

def summarize(report):
    result={k:report[k] for k in ('status','captured_utc','distance_cm','completed_segments',
        'cpu','gpu','process_ram_bytes','process_peak_ram_bytes','streaming_texture_bytes',
        'nonstreaming_texture_bytes','vsm_warning_frames_including_warmup')}
    result['timings_ms']={}
    for index,name in enumerate(('elapsed','wall_frame','game_thread_active','render_thread_active','rhi_thread_active','gpu_frame')):
        if index==0:continue
        values=sorted(row[index] for row in report['samples'] if row[index]>0)
        result['timings_ms'][name]=dict(mean=statistics.mean(values),p95=values[min(len(values)-1,int(len(values)*.95))],valid_samples=len(values)) if values else None
    components=report['static_mesh_components']
    result['static_components']=len(components)
    result['nanite_components']=sum(x['valid_nanite_data'] for x in components)
    result['non_nanite_shadow_components']=sum(x['cast_shadow'] and not x['valid_nanite_data'] for x in components)
    return result

def main():
    parser=argparse.ArgumentParser();parser.add_argument('before',type=Path);parser.add_argument('after',type=Path);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();a,b=[json.loads(p.read_text(encoding='utf-8-sig')) for p in (args.before,args.after)]
    keys=('cpu','gpu','width','height','view_distance','shadow','gi','reflection','texture','effects','foliage','shading',
          'r.ScreenPercentage','sg.ResolutionQuality','r.DynamicRes.OperationMode','r.VSync','t.MaxFPS',
          'r.Shadow.Virtual.Enable','r.Shadow.Virtual.AllowScreenOverflowMessages','r.Shadow.Virtual.MaxPhysicalPages','route')
    differences={k:[a.get(k),b.get(k)] for k in keys if a.get(k)!=b.get(k)}
    origins=a['route_origin_forward_right'];same_route=all(abs(x-y)<.1 for aa,bb in zip(origins,b['route_origin_forward_right']) for x,y in zip(aa,bb))
    if differences or not same_route:raise ValueError('Comparison mismatch '+str(differences))
    if a['status']!='PASS' or b['status']!='PASS':raise ValueError('Route did not pass')
    result=dict(status='PASS',settings={k:a[k] for k in keys},before=summarize(a),after=summarize(b),
        report_sha256={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in (args.before,args.after)},
        notes=['Same physical six-lap route, 65s sampling, actual D3D12 renderer; 60fps cap and VSync retained.',
               'CPU values exclude idle; wall frame times include pacing and screenshot work. GPU cycle timing is engine RHI timing.',
               'RAM is process working set; texture allocation does not include all GPU buffers/render targets or other apps.',
               'No VSM overflow reproduced in baseline after items 1-7. Zero warnings after is not a guarantee for every view.',
               'HISM component bounds aggregate many small instances; these are not individual caster bounds.',
               'r.ScreenPercentage=0 is automatic. After report also resolves the static engine heuristic; baseline did not record the resolved fraction.'])
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8');print('PASS comparable actual route reports')

if __name__=='__main__':main()
