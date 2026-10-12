"""Bake the bounded source district and pipeline assets inside Unreal 5.8.

Rebuilding replaces only District_ actors in this generated map. It does not
delete and immediately recreate a loaded world package, or touch campaign maps.
"""
import importlib.util
import json
import math
import os
import traceback
from pathlib import Path
import unreal

ROOT=Path(unreal.Paths.project_dir()).resolve()
INPUT=ROOT/'Saved/PlayableDistrict'
MARKER=ROOT/'Saved/PlayableDistrictReady.ok'


def read(path):
    return json.loads(path.read_text(encoding='utf-8'))


def prepare():
    MARKER.unlink(missing_ok=True)
    plan=read(INPUT/'district.json')
    imported=read(ROOT/'Saved/FreeModelImportMap.json')
    bindings=read(ROOT/'Data/model_bindings.json')['systems']
    facade_bindings=read(ROOT/'Data/facade_asset_bindings.json')
    def model(identifier):
        item=imported.get(identifier)
        mesh=unreal.load_asset(item['primary_object']) if item else None
        if not isinstance(mesh,(unreal.StaticMesh,unreal.SkeletalMesh)):
            raise RuntimeError('Required model missing: '+identifier)
        return mesh
    # Use the actual existing production importer and its LOD/collision outputs.
    os.environ['WTG_ASSET_MANIFEST']=str(ROOT/'Pipeline/assets/nadodrze/river-railing.asset.json')
    spec=importlib.util.spec_from_file_location('wtg_district_pipeline_import',ROOT/'Pipeline/unreal/import_asset.py')
    pipeline=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(pipeline)
    pipeline.prepare()
    pipeline_report=read(ROOT/'Saved/Pipeline/import/nadodrze-river-railing.json')
    railing=unreal.load_asset(pipeline_report['mesh'])
    if not isinstance(railing,unreal.StaticMesh):
        raise RuntimeError('Processed railing was not imported')
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    path=plan['map']
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        if not levels.load_level(path):
            raise RuntimeError('Cannot load generated district')
        for actor in actors.get_all_level_actors():
            if actor.get_actor_label().startswith('District_'):
                actors.destroy_actor(actor)
    elif not levels.new_level(path):
        raise RuntimeError('Cannot create generated district')
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode',unreal.GeoPreviewGameMode)
    def spawn(cls,position,label,rotation=(0,0,0)):
        actor=actors.spawn_actor_from_class(cls,unreal.Vector(*position),unreal.Rotator(*rotation))
        if not actor:
            raise RuntimeError('Cannot spawn '+label)
        actor.set_actor_label('District_'+label)
        return actor
    def static(mesh,position,label,rotation=(0,0,0),scale=(1,1,1),collision=True):
        actor=spawn(unreal.StaticMeshActor,position,label,rotation)
        component=actor.get_component_by_class(unreal.StaticMeshComponent)
        component.set_static_mesh(mesh)
        component.set_collision_profile_name('BlockAll' if collision else 'NoCollision')
        actor.set_actor_scale3d(unreal.Vector(*scale))
        return actor
    mesh_count=0
    for filename in read(INPUT/'Meshes/meshes.json'):
        record=read(INPUT/'Meshes'/filename)
        if not record['triangles']:
            continue
        name=Path(filename).stem
        asset_path='/Game/Generated/GIS/PlayableDistrict/'+name
        mesh=unreal.GeoMeshLibrary.bake_mesh(asset_path,[unreal.Vector(*p) for p in record['vertices']],record['triangles'])
        if not mesh:
            raise RuntimeError('Mesh generation failed: '+name)
        surface=record.get('material') or {'water':'Water','terrain':'Soil','rail':'Metal','green':'Soil'}.get(record['kind'],'Concrete')
        material=unreal.load_asset('/Game/SurfaceQuality/Instances/MI_'+surface) or unreal.load_asset('/Game/Generated/M_'+surface)
        if material:
            mesh.set_material(0,material)
        if not unreal.EditorAssetLibrary.save_loaded_asset(mesh,False):
            raise RuntimeError('Cannot save '+asset_path)
        static(mesh,record['origin'],name,collision=record['kind']!='water')
        mesh_count+=1
    # HISM batches keep thousands of facade windows out of the actor/tick budget.
    clusters={}
    def cluster(p):
        key=(math.floor(p[0]/12800),math.floor(p[1]/12800))
        if key not in clusters:
            clusters[key]=spawn(unreal.WTGFacadeInstanceCluster,(key[0]*12800,key[1]*12800,0),'Facades_%s_%s'%key)
        return clusters[key]
    facades=read(INPUT/'facades.json')['buildings']
    route=plan['delivery_route']+[plan['anchors']['meeting']]
    def proximity(building):
        openings=building.get('openings',[])
        if not openings:
            return math.inf
        p=openings[0]['world_position']
        return min(math.hypot(p[0]-q[0],p[1]-q[1]) for q in route)
    facade_count=0
    mesh_cache={}
    for building in sorted(facades,key=proximity):
        if proximity(building)>23000:
            continue
        for item in building.get('openings',[]):
            if facade_count>=plan['budgets']['facade_instances']:
                break
            identifier=item.get('asset_id')
            if not identifier or str(identifier).startswith('procedural:'):
                continue
            if identifier not in mesh_cache:
                mesh_cache[identifier]=model(identifier)
            mesh=mesh_cache[identifier]
            if not isinstance(mesh,unreal.StaticMesh):
                continue
            p=list(item['world_position'])
            yaw=float(item.get('yaw_deg',0))
            p[0]+=math.cos(math.radians(yaw))*18
            p[1]+=math.sin(math.radians(yaw))*18
            scale=facade_bindings.get('asset_scales',{}).get(identifier,[1,1,1])
            yaw+=float(facade_bindings.get('asset_yaw_offsets_deg',{}).get(identifier,0))
            transform=unreal.Transform(location=unreal.Vector(*p),rotation=unreal.Rotator(0,yaw,0),scale=unreal.Vector(*scale))
            cluster(p).add_facade_instance(mesh,transform)
            facade_count+=1
    registry=spawn(unreal.WTGDistrictRegistry,(0,0,0),'Registry')
    for field,key in (('meeting_point','meeting'),('delivery_point','delivery'),('vehicle_spawn','vehicle')):
        registry.set_editor_property(field,unreal.Vector(*plan['anchors'][key]))
    start=list(plan['anchors']['meeting']);start[2]+=94
    registry.set_editor_property('player_start',unreal.Vector(*start))
    registry.set_editor_property('vehicle_yaw',plan['vehicle_yaw'])
    registry.set_editor_property('bounds_min',unreal.Vector(*plan['bounds_min']))
    registry.set_editor_property('bounds_max',unreal.Vector(*plan['bounds_max']))
    registry.set_editor_property('delivery_route',[unreal.Vector(*p) for p in plan['delivery_route']])
    registry.set_editor_property('intersections',[unreal.Vector(*p) for p in plan['intersections']])
    streets=[]
    for s in plan['streets']:
        value=unreal.WTGDistrictStreet()
        value.set_editor_property('name',s['name'])
        value.set_editor_property('points',[unreal.Vector(*p) for p in s['points']])
        value.set_editor_property('walkway',s['walkway'])
        streets.append(value)
    registry.set_editor_property('streets',streets)
    routes=[]
    for r in plan['population_routes']:
        value=unreal.CityPopulationRoute()
        value.set_editor_property('id',r['id'])
        value.set_editor_property('vehicle',r['vehicle'])
        value.set_editor_property('points',[unreal.Vector(*p) for p in r['points']])
        routes.append(value)
    registry.set_editor_property('population_routes',routes)
    registry.set_editor_property('delivery_reward_cents',plan['mission']['reward_cents'])
    registry.set_editor_property('delivery_time_limit',plan['mission']['time_limit_seconds'])
    player_start=spawn(unreal.PlayerStart,start,'PlayerStart',(0,plan['vehicle_yaw'],0))
    vehicle=spawn(unreal.DriveableVehicle,plan['anchors']['vehicle'],'DeliveryVehicle',(0,plan['vehicle_yaw'],0))
    vehicle.set_visual_meshes(model(bindings['driveable_vehicle_body']),model(bindings['driveable_vehicle_wheel']))
    registry.set_editor_property('delivery_vehicle',vehicle)
    population=spawn(unreal.CityPopulation,(0,0,0),'Population')
    population.set_editor_property('routes',routes)
    population.set_editor_property('pedestrian_mesh',model(bindings['ambient_pedestrian']))
    population.set_editor_property('traffic_body_mesh',model(bindings['ambient_vehicle']))
    population.set_editor_property('traffic_wheel_mesh',model(bindings['driveable_vehicle_wheel']))
    population.set_editor_property('max_agents',plan['budgets']['pedestrians'])
    population.set_editor_property('max_traffic_vehicles',plan['budgets']['traffic'])
    registry.set_editor_property('population',population)
    env=spawn(unreal.WTGDayNightEnvironment,(0,0,0),'Sky')
    env.set_editor_property('time_of_day_hours',16.2)
    env.set_editor_property('day_length_minutes',45)
    # Name signs and a recognizable river-island information point, in Polish.
    names=set()
    for index,street in enumerate(plan['streets']):
        if street['name'] in names or street['walkway']:
            continue
        names.add(street['name'])
        p=list(street['points'][len(street['points'])//2]);p[2]+=260
        sign=spawn(unreal.TextRenderActor,p,'StreetName_'+str(index))
        text=sign.get_component_by_class(unreal.TextRenderComponent)
        text.set_text(street['name']);text.set_world_size(20)
    landmarks=read(INPUT/'sector.json')['features']
    railing.set_material(0,unreal.load_asset('/Game/SurfaceQuality/Instances/MI_Metal'))
    rails=0
    for feature in landmarks:
        if feature['kind']!='road' or feature['tags'].get('bridge','no')=='no':
            continue
        for a,b in zip(feature['points'],feature['points'][1:]):
            dx,dy=b[0]-a[0],b[1]-a[1];length=math.hypot(dx,dy)
            if length<120:
                continue
            count=max(1,int(length/245));offset=feature['width_m']*50
            for i in range(count):
                t=(i+.5)/count
                for side in (-1,1):
                    p=[a[0]+dx*t-dy/length*offset*side,a[1]+dy*t+dx/length*offset*side,a[2]+(b[2]-a[2])*t+8]
                    static(railing,p,'BridgeRail_'+str(rails),(0,math.degrees(math.atan2(dy,dx)),0),(length/count/245,1,1))
                    rails+=1
    # Regeneration is successful only after the entire map is saved.
    if not levels.save_current_level():
        raise RuntimeError('Cannot save district map')
    report={'status':'Baked','mesh_cells':mesh_count,'facade_instances':facade_count,'bridge_rails':rails,
            'map':path,'actors':len(actors.get_all_level_actors())}
    (INPUT/'engine-bake-report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    MARKER.write_text(json.dumps(report),encoding='utf-8')
    unreal.log('WTG_PLAYABLE_DISTRICT_READY '+json.dumps(report))


try:
    prepare()
except Exception:
    unreal.log_error(traceback.format_exc())
    MARKER.unlink(missing_ok=True)
    raise
finally:
    unreal.SystemLibrary.quit_editor()
