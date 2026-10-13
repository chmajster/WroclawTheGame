"""Canonical native actor/instance/material snapshot across pipeline regeneration."""
import hashlib
import json
import os
import sys
import traceback
from pathlib import Path
import unreal
ROOT=Path(unreal.Paths.project_dir()).resolve();sys.path.insert(0,str(ROOT/'Pipeline/unreal'))
from import_asset import quit_after_editor_startup
out=ROOT/'Saved/StartLocation';marker=out/'repeatability.ok';marker.unlink(missing_ok=True)
try:
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.load_level('/Game/Maps/Nadodrze_Playable'):raise RuntimeError('Map missing')
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    rows=[];labels=set();instances=0
    for actor in actors:
        label=actor.get_actor_label()
        if not label.startswith(('StartLocation_','District_','CityKit_')):continue
        if label in labels:raise RuntimeError('Duplicate label '+label)
        labels.add(label)
        meshes=[]
        for c in actor.get_components_by_class(unreal.StaticMeshComponent):
            mesh=c.get_editor_property('static_mesh')
            if not mesh:continue
            materials=[c.get_material(i).get_path_name() if c.get_material(i) else None for i in range(c.get_num_materials())]
            count=c.get_instance_count() if isinstance(c,unreal.InstancedStaticMeshComponent) else 1
            instances+=count
            poses=[]
            if isinstance(c,unreal.InstancedStaticMeshComponent):
                for i in range(count):
                    t=c.get_instance_transform(i,True);p=t.translation;s=t.scale3d;q=t.rotation
                    poses.append([round(x,3) for x in [p.x,p.y,p.z,s.x,s.y,s.z,q.x,q.y,q.z,q.w]])
            meshes.append(dict(mesh=mesh.get_path_name(),materials=materials,count=count,poses=poses))
        p=actor.get_actor_location();rows.append(dict(label=label,actor_class=actor.get_class().get_path_name(),
            position=[round(x,3) for x in [p.x,p.y,p.z]],meshes=sorted(meshes,key=lambda x:x['mesh'])))
    rows.sort(key=lambda x:x['label'])
    catalog=unreal.load_asset('/Game/Wroclaw/Hero/DA_CivilianAppearance')
    errors=unreal.CharacterCreatorValidator.validate_catalog(catalog,True)
    if errors:raise RuntimeError('\n'.join(errors))
    body=next(b for b in catalog.bodies if str(b.id)=='Male.Standard')
    if not body.mesh.get_path_name().startswith('/Game/Wroclaw/Hero/'):raise RuntimeError('Hero placeholder regression')
    digest=hashlib.sha256(json.dumps(rows,sort_keys=True,separators=(',',':')).encode()).hexdigest()
    snapshot=dict(status='PASS',actor_count=len(rows),instance_count=instances,canonical_sha256=digest,actors=rows,
        hero_mesh=body.mesh.get_path_name(),hero_animation_class=body.animation_class.get_path_name(),
        catalog_validation='PASS',scope='Editor actor labels, positions, mesh bindings and HISM transforms; runtime gameplay checked separately.')
    tag=os.environ.get('WTG_REPEATABILITY_TAG','snapshot')
    target=out/('repeatability-'+tag+'.json');target.write_text(json.dumps(snapshot,indent=2),encoding='utf-8')
    marker.write_text('PASS\n');unreal.log('WTG_START_REPEATABILITY_PASS '+digest+' actors='+str(len(rows))+' instances='+str(instances))
except Exception:unreal.log_error(traceback.format_exc());raise
finally:quit_after_editor_startup()
