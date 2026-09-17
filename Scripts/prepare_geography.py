"""Run after prepare_content.py and Scripts/gis/build_meshes.py in Unreal 5.6.
Extends the existing GIS map when WTG_CITY_INPUT is set. Campaign migration remains separate.
"""
import json,traceback,os
from pathlib import Path
import unreal
ROOT=Path(unreal.Paths.project_dir()).resolve();MARKER=ROOT/'Saved/GeographyReady.ok'
def prepare():
    MARKER.unlink(missing_ok=True)
    city_input=os.environ.get('WTG_CITY_INPUT')
    input_dir=Path(city_input) if city_input else ROOT/'Data/processed/wroclaw'
    data=json.loads((input_dir/'sector.json').read_text(encoding='utf-8'))
    mesh_dir=input_dir/'Meshes' if city_input else ROOT/'Saved/GISMeshes'
    meshes=json.loads((mesh_dir/'meshes.json').read_text())
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    map_path='/Game/Maps/Nadodrze_GIS'
    if unreal.EditorAssetLibrary.does_asset_exist(map_path):
        if not levels.new_level('/Game/Maps/WTG_GISScratch'):raise RuntimeError('Cannot change map')
        if not unreal.EditorAssetLibrary.delete_asset(map_path):raise RuntimeError('Cannot replace GIS map')
        for folder in ('/Game/__ExternalActors__/Maps/Nadodrze_GIS','/Game/__ExternalObjects__/Maps/Nadodrze_GIS'):
            if unreal.EditorAssetLibrary.does_directory_exist(folder) and not unreal.EditorAssetLibrary.delete_directory(folder):raise RuntimeError('Cannot clear generated external actors')
    if not levels.new_level(map_path):raise RuntimeError('Cannot create GIS map')
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode',unreal.GeoPreviewGameMode)
    geo=actors.spawn_actor_from_class(unreal.GeoReferencingSystem,unreal.Vector())
    for key,value in {'projected_crs':'EPSG:32633','geographic_crs':'EPSG:4326','planet_shape':unreal.PlanetShape.FLAT_PLANET,
                      'origin_location_in_projected_crs':True,'origin_at_planet_center':False,
                      'origin_projected_coordinates_easting':data['origin_projected_m'][0],
                      'origin_projected_coordinates_northing':data['origin_projected_m'][1],
                      'origin_projected_coordinates_up':data['origin_projected_m'][2],
                      'is_spatially_loaded':False}.items():geo.set_editor_property(key,value)
    geo.apply_settings()
    # Verify the exact UE plugin transform against the offline projection before saving.
    node=data['road_graph']['nodes'][0];lon,lat=node['geographic'];height=data['origin_projected_m'][2]+node['position'][2]/100
    converted=unreal.GeoReferenceLibrary.lat_lon_to_world(geo,lat,lon,height)
    expected=unreal.Vector(*node['position'])
    if (converted-expected).length()>2:raise RuntimeError('Offline/UE geography mismatch > 2 cm')
    materials={'building':'Plaster','terrain':'Concrete','road':'Asphalt','rail':'Metal','green':'Wood','water':'Metal'}
    hlod=unreal.load_asset('/Game/Generated/HLOD_Instancing')
    for filename in meshes:
        record=json.loads((mesh_dir/filename).read_text());name=Path(filename).stem;path='/Game/Generated/GIS/'+name
        if unreal.EditorAssetLibrary.does_asset_exist(path) and not unreal.EditorAssetLibrary.delete_asset(path):raise RuntimeError('Cannot replace mesh '+name)
        mesh=unreal.GeoMeshLibrary.bake_mesh(path,[unreal.Vector(*v) for v in record['vertices']],record['triangles'])
        if not mesh:raise RuntimeError('Mesh bake failed '+name)
        material=unreal.load_asset('/Game/Generated/M_'+(record.get('material') or materials[record['kind']]))
        if material:mesh.set_material(0,material)
        if not unreal.EditorAssetLibrary.save_loaded_asset(mesh,False):raise RuntimeError('Mesh save failed '+name)
        actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*record['origin']))
        actor.set_actor_label('GIS_'+name);actor.get_component_by_class(unreal.StaticMeshComponent).set_static_mesh(mesh)
        if hlod:actor.set_editor_property('hlod_layer',hlod)
    if city_input:
        path='/Game/Generated/CityDefinition';definition=unreal.load_asset(path)
        if not definition:
            factory=unreal.DataAssetFactory();factory.set_editor_property('data_asset_class',unreal.CityDefinition)
            definition=unreal.AssetToolsHelpers.get_asset_tools().create_asset('CityDefinition','/Game/Generated',unreal.CityDefinition,factory)
        if not definition or not definition.import_catalog((input_dir/'city.json').read_text(encoding='utf-8')):
            raise RuntimeError('City catalogue import failed')
        if not unreal.EditorAssetLibrary.save_loaded_asset(definition,False):raise RuntimeError('City catalogue save failed')
        registry=actors.spawn_actor_from_class(unreal.CityRegistry,unreal.Vector())
        registry.set_editor_property('definition',definition)
        registry.set_editor_property('is_spatially_loaded',False)
    nodes={n['id']:n for n in data['road_graph']['nodes']}
    candidates=[e for e in data['road_graph']['edges'] if e['name']=='Ludwika Rydygiera' and e['car_forward'] and e['length_cm']>1500 and e['bridge']=='no']
    if not candidates:raise RuntimeError('No verified street spawn segment')
    edge=max(candidates,key=lambda e:e['length_cm']);a=unreal.Vector(*nodes[edge['from']]['position']);b=unreal.Vector(*nodes[edge['to']]['position'])
    point=(a+b)*.5;rotation=unreal.MathLibrary.find_look_at_rotation(a,b)
    start=actors.spawn_actor_from_class(unreal.PlayerStart,point+unreal.Vector(0,0,150))
    car=actors.spawn_actor_from_class(unreal.DriveableVehicle,point+(b-a)*(650/(b-a).length())+unreal.Vector(0,0,85),rotation)
    # The laboratory car stays loaded so its physics state cannot reset during a short drive.
    car.set_editor_property('is_spatially_loaded',False)
    definitions=[]
    for record in json.loads((ROOT/'Data/processed/wroclaw/races.json').read_text(encoding='utf-8')):
        path='/Game/Generated/Race_'+record['id'];definition=unreal.load_asset(path)
        if not definition:
            factory=unreal.DataAssetFactory();factory.set_editor_property('data_asset_class',unreal.RaceDefinition)
            definition=unreal.AssetToolsHelpers.get_asset_tools().create_asset('Race_'+record['id'],'/Game/Generated',unreal.RaceDefinition,factory)
        if not definition:raise RuntimeError('Cannot create race definition')
        definition.set_editor_property('race_id',record['id']);definition.set_editor_property('title',record['title'])
        definition.set_editor_property('checkpoints',[unreal.Vector(*v) for v in record['checkpoints']])
        definition.set_editor_property('start',unreal.Transform(location=unreal.Vector(*record['start']),rotation=unreal.Rotator(0,record['yaw'],0).quaternion()))
        definition.set_editor_property('time_limit',record['time_limit']);definition.set_editor_property('delivery',record['mode']=='Delivery')
        unreal.EditorAssetLibrary.save_loaded_asset(definition,False);definitions.append(definition)
    car.get_component_by_class(unreal.RaceSession).set_editor_property('definitions',definitions)
    sun=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,10000),unreal.Rotator(-45,35,0))
    sun.get_component_by_class(unreal.DirectionalLightComponent).set_editor_property('intensity',4)
    actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,1000))
    actors.spawn_actor_from_class(unreal.SkyAtmosphere,unreal.Vector())
    if not levels.save_current_level():raise RuntimeError('GIS map save failed')
    MARKER.write_text(('Wave 1 city' if city_input else 'Nadodrze')+' GIS generated; engine playtest pending\n')
try:prepare()
except Exception:unreal.log_error(traceback.format_exc());MARKER.unlink(missing_ok=True)
finally:unreal.SystemLibrary.quit_editor()
