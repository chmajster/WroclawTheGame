"""Apply source-driven production to the playable map, preserving gameplay actors."""
import hashlib
import json
import math
import sys
import traceback
from pathlib import Path
import unreal

ROOT=Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0,str(ROOT/'Pipeline/unreal'))
from import_asset import quit_after_editor_startup
from city_quality import configure_instances
DEST='/Game/Wroclaw/StartLocation'
marker=ROOT/'Saved/StartLocation/placement.ok';marker.unlink(missing_ok=True)


def prepare():
    plan=json.loads((ROOT/'Saved/StartLocation/placement.json').read_text(encoding='utf-8'))
    for path,sha in plan['source_files'].items():
        if hashlib.sha256((ROOT/path).read_bytes()).hexdigest()!=sha:raise ValueError('Stale placement input '+path)
    policy=json.loads((ROOT/'Data/start_location.json').read_text(encoding='utf-8'))
    source=json.loads((ROOT/'Data/start_location_buildings.json').read_text(encoding='utf-8'))
    metadata=json.loads((ROOT/'SourceAssets/Models/Wroclaw/StartLocation/geometry.json').read_text(encoding='utf-8'))
    records={r['asset']:r for r in metadata['assets']}
    imported=json.loads((ROOT/'Saved/FreeModelImportMap.json').read_text(encoding='utf-8'))
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not levels.load_level(plan['map']):raise RuntimeError('Playable map missing')
    for actor in actors.get_all_level_actors():
        if actor.get_actor_label().startswith(policy['placement_prefix']):actors.destroy_actor(actor)
    tools=unreal.AssetToolsHelpers.get_asset_tools();edit=unreal.MaterialEditingLibrary
    for building in source['buildings']:
        record=records[building['asset']]
        mesh=unreal.load_asset(DEST+'/Buildings/SM_wtg_start_'+building['asset'].replace('-','_'))
        if not isinstance(mesh,unreal.StaticMesh):raise RuntimeError('Production mesh missing '+building['asset'])
        center=record['source_center_m'];origin=building['origin_cm']
        position=[origin[0]+center[0]*100,origin[1]-center[1]*100,origin[2]+center[2]*100]
        actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*position))
        actor.set_actor_label('StartLocation_Building_'+building['asset'])
        component=actor.get_component_by_class(unreal.StaticMeshComponent);component.set_static_mesh(mesh);component.set_collision_profile_name('BlockAll')
        name='MI_Wall_'+building['asset'].replace('-','_');instance=unreal.load_asset(DEST+'/Materials/'+name)
        if not instance:instance=tools.create_asset(name,DEST+'/Materials',unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
        edit.set_material_instance_parent(instance,unreal.load_asset(DEST+'/Materials/MI_Plaster'))
        edit.set_material_instance_vector_parameter_value(instance,'Tint',unreal.LinearColor(*building['tint'],1));edit.update_material_instance(instance)
        unreal.EditorAssetLibrary.save_loaded_asset(instance,False);component.set_material(0,instance)
    role_mesh={r['asset']:unreal.load_asset(DEST+'/'+r['area']+'/SM_wtg_start_'+r['asset'].replace('-','_')) for r in metadata['assets'] if r['area']!='Buildings'}
    for role,slug in [('lamp','street_lamp_01'),('seating','modular_street_seating'),('bin','metal_trash_can')]:role_mesh[role]=unreal.load_asset(imported[slug]['primary_object'])
    role_mesh['paint']=unreal.load_asset('/Engine/BasicShapes/Cube')
    clusters={}
    for item in plan['items']:
        role=item['role'];mesh=role_mesh[role]
        if not isinstance(mesh,unreal.StaticMesh):raise RuntimeError('Missing '+role)
        p=list(item['position_cm']);yaw=item['yaw_deg'];scale=list(item['scale']);bounds=mesh.get_bounds()
        if role in ['lamp','seating','bin']:
            target={'lamp':750,'seating':80,'bin':85}[role];s=target/max(1,bounds.box_extent.z*2);scale=[s,s,s]
            # Centre imported models on their mapped locations and put the base on ground.
            c,ss=math.cos(math.radians(yaw)),math.sin(math.radians(yaw))
            p[0]-=(bounds.origin.x*c-bounds.origin.y*ss)*s;p[1]-=(bounds.origin.x*ss+bounds.origin.y*c)*s
            p[2]-=(bounds.origin.z-bounds.box_extent.z)*s
        key=(math.floor(p[0]/12800),math.floor(p[1]/12800),role)
        if key not in clusters:
            cluster=actors.spawn_actor_from_class(unreal.WTGFacadeInstanceCluster,unreal.Vector(key[0]*12800,key[1]*12800,0))
            cluster.set_actor_label('StartLocation_Instances_'+'_'.join(map(str,key)));clusters[key]=cluster
        transform=unreal.Transform(location=unreal.Vector(*p),rotation=unreal.Rotator(pitch=0,yaw=yaw,roll=0),scale=unreal.Vector(*scale))
        if clusters[key].add_facade_instance(mesh,transform)<0:raise RuntimeError('Cannot instance '+role)
    for key,cluster in clusters.items():
        for component in cluster.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent):
            configure_instances(component)
            component.set_collision_profile_name('BlockAll' if key[2] in ['curb','bike-rack','parked-car','seating','bin','lamp'] else 'NoCollision')
            if key[2]=='paint':component.set_material(0,unreal.load_asset(DEST+'/Materials/MI_Frame'))
    # Surface replacements use source mesh classification, not arbitrary actor positions.
    surfaces={}
    for name in json.loads((ROOT/'Saved/PlayableDistrict/Meshes/meshes.json').read_text()):
        record=json.loads((ROOT/'Saved/PlayableDistrict/Meshes'/name).read_text());kind=record['kind'];surface=record.get('material')
        material_name=None
        if kind=='sidewalk':material_name='Pavement'
        elif kind=='rail':material_name='Cobble'
        elif kind=='road':material_name='Asphalt' if surface=='Asphalt' else ('Cobble' if surface=='Stone' else 'Pavement')
        elif kind=='green':
            mesh=unreal.load_asset('/Game/Generated/GIS/PlayableDistrict/'+Path(name).stem)
            if mesh:mesh.set_material(0,unreal.load_asset('/Game/SurfaceQuality/Instances/MI_Grass'));unreal.EditorAssetLibrary.save_loaded_asset(mesh,False)
        if material_name:
            mesh=unreal.load_asset('/Game/Generated/GIS/PlayableDistrict/'+Path(name).stem)
            if not mesh:continue
            mesh.set_material(0,unreal.load_asset(DEST+'/Materials/MI_'+material_name));unreal.EditorAssetLibrary.save_loaded_asset(mesh,False)
            surfaces[material_name]=surfaces.get(material_name,0)+1
    if plan['rail_strips']:
        vertices=[];triangles=[]
        for strip in plan['rail_strips']:
            base=len(vertices);vertices.extend(strip)
            a,b,c=strip[:3];positive=(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])>0
            triangles.extend([base,base+1,base+2,base,base+2,base+3] if positive else [base,base+2,base+1,base,base+3,base+2])
        mesh=unreal.GeoMeshLibrary.bake_mesh('/Game/Generated/GIS/StartLocation/TramRails',[unreal.Vector(*p) for p in vertices],triangles)
        mesh.set_material(0,unreal.load_asset(DEST+'/Materials/MI_Metal'));unreal.EditorAssetLibrary.save_loaded_asset(mesh,False)
        actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector());actor.set_actor_label('StartLocation_TramRails')
        component=actor.get_component_by_class(unreal.StaticMeshComponent);component.set_static_mesh(mesh);component.set_collision_profile_name('NoCollision')
    for actor in actors.get_all_level_actors():
        if isinstance(actor,unreal.WTGDayNightEnvironment):actor.set_time_of_day(13.5)
        if isinstance(actor,unreal.SurfaceQualityDirector):actor.set_editor_property('follow_weather',True)
    if sum(c.get_facade_instance_count() for c in clusters.values())!=len(plan['items']):raise RuntimeError('Instance count mismatch')
    if not levels.save_current_level():raise RuntimeError('Save failed')
    report=dict(status='PASS',buildings=len(source['buildings']),instances=len(plan['items']),clusters=len(clusters),counts=plan['counts'],surfaces=surfaces,rail_strips=len(plan['rail_strips']),accuracy=plan['placement_accuracy'])
    (marker.parent/'placement-report.json').write_text(json.dumps(report,indent=2),encoding='utf-8');marker.write_text('PASS\n',encoding='utf-8');unreal.log('WTG_START_LOCATION_PLACED '+json.dumps(report))


try:prepare()
except Exception:unreal.log_error(traceback.format_exc());raise
finally:quit_after_editor_startup()
