"""Run after prepare_content.py and Scripts/gis/build_meshes.py in Unreal 5.6.
Extends the existing GIS map when WTG_CITY_INPUT is set. Campaign migration remains separate.
"""
import json,traceback,os
from pathlib import Path
import unreal
ROOT=Path(unreal.Paths.project_dir()).resolve();MARKER=ROOT/'Saved/GeographyReady.ok'
FREE_MODEL_IMPORT_MAP=ROOT/'Saved/FreeModelImportMap.json';MODEL_BINDINGS=ROOT/'Data/model_bindings.json'
def prepare():
    MARKER.unlink(missing_ok=True)
    if not FREE_MODEL_IMPORT_MAP.is_file():raise RuntimeError('Free model import map missing')
    imported_models=json.loads(FREE_MODEL_IMPORT_MAP.read_text(encoding='utf-8'))
    model_bindings=json.loads(MODEL_BINDINGS.read_text(encoding='utf-8'))
    def model(model_id,expected):
        metadata=imported_models.get(model_id);object_path=metadata.get('primary_object') if metadata else None
        asset=unreal.load_asset(object_path) if object_path else None
        if not asset or not isinstance(asset,expected):raise RuntimeError('Wrong or missing model '+model_id+' -> '+str(object_path))
        return asset
    systems=model_bindings['systems']
    city_door_mesh=model(systems['city_interior_door'],unreal.StaticMesh)
    city_activity_mesh=model(systems['city_activity_marker'],unreal.StaticMesh)
    pedestrian_mesh=model(systems['ambient_pedestrian'],unreal.SkeletalMesh)
    ambient_vehicle_mesh=model(systems['ambient_vehicle'],unreal.StaticMesh)
    driveable_body_mesh=model(systems['driveable_vehicle_body'],unreal.StaticMesh)
    driveable_wheel_mesh=model(systems['driveable_vehicle_wheel'],unreal.StaticMesh)
    interior_sofa_mesh=model(systems['city_interior_sofa'],unreal.StaticMesh)
    interior_table_mesh=model(systems['city_interior_table'],unreal.StaticMesh)
    interior_chair_mesh=model(systems['city_interior_chair'],unreal.StaticMesh)
    interior_cabinet_mesh=model(systems['city_interior_cabinet'],unreal.StaticMesh)
    interior_shelf_mesh=model(systems['city_interior_shelf'],unreal.StaticMesh)
    interior_lamp_mesh=model(systems['city_interior_lamp'],unreal.StaticMesh)
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
    materials={'building':'Plaster','terrain':'Concrete','road':'Asphalt','rail':'Metal','green':'Soil','water':'Water'}
    hlod=unreal.load_asset('/Game/Generated/HLOD_Instancing')
    for filename in meshes:
        record=json.loads((mesh_dir/filename).read_text());name=Path(filename).stem;path='/Game/Generated/GIS/'+name
        if unreal.EditorAssetLibrary.does_asset_exist(path) and not unreal.EditorAssetLibrary.delete_asset(path):raise RuntimeError('Cannot replace mesh '+name)
        mesh=unreal.GeoMeshLibrary.bake_mesh(path,[unreal.Vector(*v) for v in record['vertices']],record['triangles'])
        if not mesh:raise RuntimeError('Mesh bake failed '+name)
        surface_name=record.get('material') or materials[record['kind']]
        material=unreal.load_asset('/Game/SurfaceQuality/Instances/MI_'+surface_name) or unreal.load_asset('/Game/Generated/M_'+surface_name)
        if material:mesh.set_material(0,material)
        if not unreal.EditorAssetLibrary.save_loaded_asset(mesh,False):raise RuntimeError('Mesh save failed '+name)
        actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*record['origin']))
        actor.set_actor_label('GIS_'+name);component=actor.get_component_by_class(unreal.StaticMeshComponent);component.set_static_mesh(mesh)
        if record['kind']=='water':component.set_collision_profile_name('NoCollision')
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
        content=json.loads((input_dir/'content.json').read_text(encoding='utf-8'))
        cube=unreal.load_asset('/Engine/BasicShapes/Cube.Cube')
        def room_part(center,offset,size):
            part=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*[center[i]+offset[i] for i in range(3)]))
            component=part.get_component_by_class(unreal.StaticMeshComponent);component.set_static_mesh(cube)
            component.set_material(0,unreal.load_asset('/Game/Generated/M_Plaster'))
            part.set_actor_scale3d(unreal.Vector(*[v/100 for v in size]))
        def interior_prop(interior,center,offset,mesh,suffix,scale,yaw=0):
            position=[center[i]+offset[i] for i in range(3)]
            prop=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*position),unreal.Rotator(0,yaw,0))
            if not prop:raise RuntimeError('Cannot place interior prop '+interior['id']+' '+suffix)
            component=prop.get_component_by_class(unreal.StaticMeshComponent);component.set_static_mesh(mesh)
            component.set_collision_profile_name('NoCollision');prop.set_actor_scale3d(unreal.Vector(*scale))
            prop.set_actor_label(interior['id']+'_'+suffix)
            return prop
        for interior in content['interiors']:
            center=interior['center']
            room_part(center,[0,0,-20],[1000,800,40])
            for offset,size in [([-500,0,200],[30,800,440]),([500,0,200],[30,800,440]),([0,-400,200],[1000,30,440]),([0,400,200],[1000,30,440])]:room_part(center,offset,size)
            # Walkable stair flight to the authored upper room; no collision inside the GIS shell is guessed.
            for step in range(10):room_part(center,[-260+step*40,0,(step+1)*10],[40,180,(step+1)*20])
            room_part(center,[275,0,190],[450,750,20])
            interior_prop(interior,center,[250,-230,0],interior_sofa_mesh,'sofa',[.72,.72,.72],180)
            interior_prop(interior,center,[-100,100,0],interior_table_mesh,'table',[.85,.85,.85],0)
            interior_prop(interior,center,[-100,-25,0],interior_chair_mesh,'chair',[.85,.85,.85],15)
            interior_prop(interior,center,[350,250,0],interior_cabinet_mesh,'cabinet',[.72,.72,.72],180)
            interior_prop(interior,center,[-350,250,0],interior_shelf_mesh,'shelf',[.8,.8,.8],0)
            interior_prop(interior,center,[-100,100,82],interior_lamp_mesh,'lamp',[.55,.55,.55],25)
            entry=actors.spawn_actor_from_class(unreal.CityInteriorDoor,unreal.Vector(*interior['entrance']))
            entry.set_editor_property('label','Wejdź: '+interior['title']);entry.set_editor_property('building_id',interior['building'])
            entry.set_editor_property('destination',unreal.Vector(center[0]-350,center[1]-180,center[2]+94))
            entry.get_component_by_class(unreal.StaticMeshComponent).set_static_mesh(city_door_mesh);entry.set_actor_scale3d(unreal.Vector(1,1,1))
            exit_door=actors.spawn_actor_from_class(unreal.CityInteriorDoor,unreal.Vector(center[0]-400,center[1]-220,center[2]+100))
            exit_door.set_editor_property('label','Wyjdź na ulicę');exit_door.set_editor_property('building_id',interior['building'])
            point=interior['entrance'];exit_door.set_editor_property('destination',unreal.Vector(point[0]+130,point[1],point[2]+24))
            exit_door.get_component_by_class(unreal.StaticMeshComponent).set_static_mesh(city_door_mesh);exit_door.set_actor_scale3d(unreal.Vector(1,1,1))
            light=actors.spawn_actor_from_class(unreal.PointLight,unreal.Vector(center[0],center[1],center[2]+350))
            light.get_component_by_class(unreal.PointLightComponent).set_editor_property('intensity',3000)
        for item in content['activities']:
            actor=actors.spawn_actor_from_class(unreal.CityActivity,unreal.Vector(*item['position']))
            if not actor:raise RuntimeError('Cannot place city activity '+item['id'])
            actor.set_editor_property('action_id',item['id'])
            actor.set_editor_property('building_id',item['building'])
            actor.get_component_by_class(unreal.StaticMeshComponent).set_static_mesh(city_activity_mesh);actor.set_actor_scale3d(unreal.Vector(1,1,1))
            actor.set_actor_label(item['id'])
        population=actors.spawn_actor_from_class(unreal.CityPopulation,unreal.Vector())
        definitions=[]
        for item in content['population_routes']:
            definition=unreal.CityPopulationRoute()
            definition.set_editor_property('id',item['id'])
            definition.set_editor_property('vehicle',item['vehicle'])
            definition.set_editor_property('points',[unreal.Vector(*p) for p in item['points']])
            definitions.append(definition)
        population.set_editor_property('routes',definitions)
        population.set_editor_property('vehicle_mesh',ambient_vehicle_mesh);population.set_editor_property('pedestrian_mesh',pedestrian_mesh)
        population.set_editor_property('is_spatially_loaded',False)
        # Recast builds only around existing character NavigationInvoker components.
        for sector in json.loads((input_dir/'city.json').read_text(encoding='utf-8'))['sectors']:
            low,high=sector['min'],sector['max']
            nav=actors.spawn_actor_from_class(unreal.NavMeshBoundsVolume,unreal.Vector((low[0]+high[0])/2,(low[1]+high[1])/2,0))
            if not nav:raise RuntimeError('Cannot create city navigation bounds')
            _,extent=nav.get_actor_bounds(False)
            if min(extent.x,extent.y,extent.z)<=0:raise RuntimeError('Empty city navigation brush')
            nav.set_actor_scale3d(unreal.Vector((high[0]-low[0])/2/extent.x,(high[1]-low[1])/2/extent.y,5000/extent.z))
            nav.set_editor_property('is_spatially_loaded',False)
    nodes={n['id']:n for n in data['road_graph']['nodes']}
    candidates=[e for e in data['road_graph']['edges'] if e['name']=='Ludwika Rydygiera' and e['car_forward'] and e['length_cm']>1500 and e['bridge']=='no']
    if not candidates:raise RuntimeError('No verified street spawn segment')
    edge=max(candidates,key=lambda e:e['length_cm']);a=unreal.Vector(*nodes[edge['from']]['position']);b=unreal.Vector(*nodes[edge['to']]['position'])
    point=(a+b)*.5;rotation=unreal.MathLibrary.find_look_at_rotation(a,b)
    start=actors.spawn_actor_from_class(unreal.PlayerStart,point+unreal.Vector(0,0,150))
    car=actors.spawn_actor_from_class(unreal.DriveableVehicle,point+(b-a)*(650/(b-a).length())+unreal.Vector(0,0,85),rotation)
    car.set_visual_meshes(driveable_body_mesh,driveable_wheel_mesh)
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
