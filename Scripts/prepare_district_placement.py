"""Update gameplay placement from generated source without rebaking geometry."""
import json
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir())
marker=root/'Saved/DistrictPlacementReady.ok'
marker.unlink(missing_ok=True)
try:
    plan=json.loads((root/'Saved/PlayableDistrict/district.json').read_text(encoding='utf-8'))
    imports_path=root/'Saved/FreeModelImportMap.json'
    imports=json.loads(imports_path.read_text(encoding='utf-8'))
    for key in ('kenney-car-sedan','kenney-car-sedan-sports'):
        meshes=[unreal.load_asset(p) for p in imports[key]['objects']]
        body=next(m for m in meshes if isinstance(m,unreal.StaticMesh) and m.get_name().lower()=='body')
        imports[key]['primary_object']=body.get_path_name()
        unreal.log('WTG_CAR_BODY_SELECTED '+key+' '+str(body.get_bounds()))
    imports_path.write_text(json.dumps(imports,ensure_ascii=False,indent=2),encoding='utf-8')
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    levels.load_level('/Game/Maps/Nadodrze_Playable')
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    registry=[a for a in actors if isinstance(a,unreal.WTGDistrictRegistry)]
    if len(registry)!=1:
        raise RuntimeError('Expected one district registry')
    r=registry[0]
    r.set_editor_property('vehicle_yaw',plan['vehicle_yaw'])
    r.set_editor_property('vehicle_spawn',unreal.Vector(*plan['anchors']['vehicle']))
    r.set_editor_property('delivery_route',[unreal.Vector(*p) for p in plan['delivery_route']])
    car=r.get_editor_property('delivery_vehicle')
    bindings=json.loads((root/'Data/model_bindings.json').read_text(encoding='utf-8'))['systems']
    body=unreal.load_asset(imports[bindings['driveable_vehicle_body']]['primary_object'])
    wheel=unreal.load_asset(imports[bindings['driveable_vehicle_wheel']]['primary_object'])
    car.set_visual_meshes(body,wheel)
    car.set_actor_location(unreal.Vector(*plan['anchors']['vehicle']),False,True)
    car.set_actor_rotation(unreal.Rotator(pitch=0,yaw=plan['vehicle_yaw'],roll=0),True)
    for a in actors:
        if isinstance(a,unreal.PlayerStart):
            a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=plan['vehicle_yaw'],roll=0),True)
    routes=[]
    for route in plan['population_routes']:
        value=unreal.CityPopulationRoute()
        value.set_editor_property('id',route['id']);value.set_editor_property('vehicle',route['vehicle'])
        value.set_editor_property('points',[unreal.Vector(*p) for p in route['points']]);routes.append(value)
    r.set_editor_property('population_routes',routes)
    r.get_editor_property('population').set_editor_property('routes',routes)
    r.get_editor_property('population').set_editor_property('traffic_body_mesh',unreal.load_asset(imports[bindings['ambient_vehicle']]['primary_object']))
    if not levels.save_current_level():
        raise RuntimeError('Map save failed')
    marker.write_text(str(plan['vehicle_yaw']),encoding='utf-8')
    unreal.log('WTG_DISTRICT_PLACEMENT_READY yaw='+str(plan['vehicle_yaw']))
finally:
    unreal.SystemLibrary.quit_editor()
