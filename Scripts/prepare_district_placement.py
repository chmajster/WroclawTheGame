"""Update gameplay placement from generated source without rebaking geometry."""
import json
import math
import os
import sys
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir())
sys.path.insert(0,str(root/'Scripts'))
from district_facades import opening_pose,wall_material
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
    if os.environ.get('WTG_REBAKE_DISTRICT_BUILDINGS')=='1':
        editor_actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        for a in actors:
            if a.get_actor_label().startswith('District_building_'):
                a.get_component_by_class(unreal.StaticMeshComponent).set_static_mesh(None)
                editor_actors.destroy_actor(a)
        files=json.loads((root/'Saved/PlayableDistrict/Meshes/meshes.json').read_text(encoding='utf-8'))
        baked=0
        for filename in files:
            if not filename.startswith('building_'):continue
            record=json.loads((root/'Saved/PlayableDistrict/Meshes'/filename).read_text(encoding='utf-8'))
            name=Path(filename).stem
            mesh=unreal.GeoMeshLibrary.bake_mesh('/Game/Generated/GIS/PlayableDistrict/'+name,[unreal.Vector(*p) for p in record['vertices']],record['triangles'])
            material=unreal.load_asset('/Game/SurfaceQuality/Instances/MI_'+record['material'])
            mesh.set_material(0,material)
            if not unreal.EditorAssetLibrary.save_loaded_asset(mesh,False):raise RuntimeError('Building save failed: '+name)
            a=editor_actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*record['origin']),unreal.Rotator())
            a.set_actor_label('District_'+name)
            component=a.get_component_by_class(unreal.StaticMeshComponent)
            component.set_static_mesh(mesh);component.set_collision_profile_name('BlockAll');baked+=1
        unreal.log('WTG_DISTRICT_BUILDINGS_REBAKED count='+str(baked))
        actors=editor_actors.get_all_level_actors()
    registry=[a for a in actors if isinstance(a,unreal.WTGDistrictRegistry)]
    if len(registry)!=1:
        raise RuntimeError('Expected one district registry')
    r=registry[0]
    wall_cache={}
    for a in actors:
        label=a.get_actor_label()
        if label.startswith('District_building_'):
            surface=label.rsplit('_',1)[1]
            if surface not in wall_cache:wall_cache[surface]=wall_material(unreal,surface)
            a.get_component_by_class(unreal.StaticMeshComponent).set_material(0,wall_cache[surface])
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
    # Refit opening pivots without rebaking source geometry or pipeline assets.
    for a in actors:
        if a.get_actor_label().startswith('District_Facades_'):
            unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(a)
    window_path=json.loads((root/'Saved/Pipeline/import/nadodrze-window-frame.json').read_text(encoding='utf-8'))['mesh']
    window=unreal.load_asset(window_path);clusters={};cache={};count=0
    facades=json.loads((root/'Saved/PlayableDistrict/facades.json').read_text(encoding='utf-8'))['buildings']
    route=plan['delivery_route']+[plan['anchors']['meeting']]
    def proximity(b):
        points=b.get('openings',[])
        return min((math.dist(points[0]['world_position'][:2],q[:2]) for q in route),default=math.inf) if points else math.inf
    editor_actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for building in sorted(facades,key=proximity):
        if proximity(building)>23000:continue
        for item in building.get('openings',[]):
            if count>=plan['budgets']['facade_instances']:break
            identifier=item.get('asset_id')
            if not identifier or identifier.startswith('procedural:'):continue
            if identifier not in cache:
                cache[identifier]=window if identifier.startswith('kenney-window-') else unreal.load_asset(imports[identifier]['primary_object'])
                unreal.log('WTG_FACADE_BOUNDS '+identifier+' '+str(cache[identifier].get_bounds()))
            mesh=cache[identifier];bounds=mesh.get_bounds()
            if not isinstance(mesh,unreal.StaticMesh):continue
            p,yaw,scale=opening_pose(item,[bounds.origin.x,bounds.origin.y,bounds.origin.z],[bounds.box_extent.x,bounds.box_extent.y,bounds.box_extent.z])
            key=(math.floor(p[0]/12800),math.floor(p[1]/12800))
            if key not in clusters:
                a=editor_actors.spawn_actor_from_class(unreal.WTGFacadeInstanceCluster,unreal.Vector(key[0]*12800,key[1]*12800,0),unreal.Rotator())
                a.set_actor_label('District_Facades_%s_%s'%key);clusters[key]=a
            clusters[key].add_facade_instance(mesh,unreal.Transform(location=unreal.Vector(*p),rotation=unreal.Rotator(pitch=0,yaw=yaw,roll=0),scale=unreal.Vector(*scale)))
            count+=1
    placed=sum(a.get_facade_instance_count() for a in clusters.values())
    unreal.log('WTG_FACADE_REFIT attempted='+str(count)+' placed='+str(placed))
    if not levels.save_current_level():
        raise RuntimeError('Map save failed')
    marker.write_text(str(plan['vehicle_yaw']),encoding='utf-8')
    unreal.log('WTG_DISTRICT_PLACEMENT_READY yaw='+str(plan['vehicle_yaw']))
finally:
    unreal.SystemLibrary.quit_editor()
