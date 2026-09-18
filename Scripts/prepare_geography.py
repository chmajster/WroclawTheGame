"""Run after prepare_content.py and Scripts/gis/build_meshes.py in Unreal 5.8.
Extends the GIS map when WTG_CITY_INPUT is set. WTG_CAMPAIGN_GIS_INPUT overlays
the validated campaign migration and switches the map to the campaign game mode.
"""
import json,traceback,os
from pathlib import Path
import unreal
ROOT=Path(unreal.Paths.project_dir()).resolve();MARKER=ROOT/'Saved/GeographyReady.ok'
def prepare():
    MARKER.unlink(missing_ok=True)
    city_input=os.environ.get('WTG_CITY_INPUT')
    campaign_input=os.environ.get('WTG_CAMPAIGN_GIS_INPUT')
    campaign_dir=Path(campaign_input) if campaign_input else None
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
    world.get_world_settings().set_editor_property(
        'default_game_mode', unreal.SliceGameMode if campaign_dir else unreal.GeoPreviewGameMode)
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
        for interior in content['interiors']:
            center=interior['center']
            room_part(center,[0,0,-20],[1000,800,40])
            for offset,size in [([-500,0,200],[30,800,440]),([500,0,200],[30,800,440]),([0,-400,200],[1000,30,440]),([0,400,200],[1000,30,440])]:room_part(center,offset,size)
            # Walkable stair flight to the authored upper room; no collision inside the GIS shell is guessed.
            for step in range(10):room_part(center,[-260+step*40,0,(step+1)*10],[40,180,(step+1)*20])
            room_part(center,[275,0,190],[450,750,20])
            entry=actors.spawn_actor_from_class(unreal.CityInteriorDoor,unreal.Vector(*interior['entrance']))
            entry.set_editor_property('label','Wejdź: '+interior['title']);entry.set_editor_property('building_id',interior['building'])
            entry.set_editor_property('destination',unreal.Vector(center[0]-350,center[1]-180,center[2]+94))
            exit_door=actors.spawn_actor_from_class(unreal.CityInteriorDoor,unreal.Vector(center[0]-400,center[1]-220,center[2]+100))
            exit_door.set_editor_property('label','Wyjdź na ulicę');exit_door.set_editor_property('building_id',interior['building'])
            point=interior['entrance'];exit_door.set_editor_property('destination',unreal.Vector(point[0]+130,point[1],point[2]+24))
            light=actors.spawn_actor_from_class(unreal.PointLight,unreal.Vector(center[0],center[1],center[2]+350))
            light.get_component_by_class(unreal.PointLightComponent).set_editor_property('intensity',3000)
        for item in content['activities']:
            actor=actors.spawn_actor_from_class(unreal.CityActivity,unreal.Vector(*item['position']))
            if not actor:raise RuntimeError('Cannot place city activity '+item['id'])
            actor.set_editor_property('action_id',item['id'])
            actor.set_editor_property('building_id',item['building'])
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
    if campaign_dir:
        migration_text=(campaign_dir/'campaign_migration.json').read_text(encoding='utf-8')
        migration=json.loads(migration_text)
        if not migration.get('stable_ids_preserved') or not migration.get('runtime_data_generated'):
            raise RuntimeError('Campaign GIS migration report is invalid')
        migration_path='/Game/Generated/CampaignMigrationDefinition'
        migration_definition=unreal.load_asset(migration_path)
        if not migration_definition:
            factory=unreal.DataAssetFactory()
            factory.set_editor_property('data_asset_class',unreal.CampaignMigrationDefinition)
            migration_definition=unreal.AssetToolsHelpers.get_asset_tools().create_asset(
                'CampaignMigrationDefinition','/Game/Generated',unreal.CampaignMigrationDefinition,factory)
        if not migration_definition or not migration_definition.import_json(migration_text):
            raise RuntimeError('Campaign migration Data Asset import failed')
        if not unreal.EditorAssetLibrary.save_loaded_asset(migration_definition,False):
            raise RuntimeError('Campaign migration Data Asset save failed')
        migration_registry=actors.spawn_actor_from_class(unreal.CampaignMigrationRegistry,unreal.Vector())
        if not migration_registry:raise RuntimeError('Campaign migration registry spawn failed')
        migration_registry.set_editor_property('definition',migration_definition)
        migration_registry.set_editor_property('is_spatially_loaded',False)
        campaign_environment=json.loads((campaign_dir/'environment.json').read_text(encoding='utf-8'))
        campaign_chapter=json.loads((campaign_dir/'chapter1.json').read_text(encoding='utf-8'))
        campaign_world=json.loads((campaign_dir/'openworld.json').read_text(encoding='utf-8'))
        campaign_opening=json.loads((campaign_dir/'opening_scene_models.json').read_text(encoding='utf-8'))
        bindings=json.loads((ROOT/'Data/prop_model_bindings.json').read_text(encoding='utf-8'))
        binding_by_action={item['action']:item for item in bindings}
        imported=json.loads((ROOT/'Saved/FreeModelImportMap.json').read_text(encoding='utf-8'))
        required={item['model'] for item in campaign_opening}
        required.update(item['model'] for item in bindings)
        free_models={}
        for model_id in sorted(required):
            metadata=imported.get(model_id)
            object_path=metadata.get('primary_object') if metadata else None
            mesh=unreal.load_asset(object_path) if object_path else None
            if not mesh or not isinstance(mesh,unreal.StaticMesh):
                raise RuntimeError('Campaign free model unavailable: '+model_id)
            free_models[model_id]=mesh

        def campaign_spawn(cls,position,label,rotation=None):
            actor=actors.spawn_actor_from_class(
                cls,unreal.Vector(*position),unreal.Rotator(*(rotation or [0,0,0])))
            if not actor:raise RuntimeError('Cannot spawn campaign actor '+label)
            actor.set_actor_label('CampaignGIS_'+label)
            return actor

        def apply_mesh(actor,mesh,scale):
            component=actor.get_component_by_class(unreal.StaticMeshComponent)
            if not component:raise RuntimeError('Missing campaign StaticMeshComponent')
            component.set_static_mesh(mesh)
            actor.set_actor_scale3d(unreal.Vector(*scale))
            component.set_editor_property('cast_shadow',True)
            return component

        for record in campaign_environment:
            kind=record['type'];rotation=record.get('rotation',[0,0,0])
            if kind=='box':
                actor=campaign_spawn(unreal.StaticMeshActor,record['position'],record['id'],rotation)
                component=actor.get_component_by_class(unreal.StaticMeshComponent);component.set_static_mesh(cube)
                surface=unreal.load_asset('/Game/SurfaceQuality/Instances/MI_'+record['material']) or unreal.load_asset('/Game/Generated/M_'+record['material'])
                if surface:component.set_material(0,surface)
                actor.set_actor_scale3d(unreal.Vector(*[v/100 for v in record['size']]))
                if hlod:actor.set_editor_property('hlod_layer',hlod)
            elif kind=='light':
                actor=campaign_spawn(unreal.PointLight,record['position'],record['id'],rotation)
                component=actor.get_component_by_class(unreal.PointLightComponent)
                component.set_editor_property('intensity',record['intensity'])
                component.set_editor_property('attenuation_radius',record['radius'])
                component.set_editor_property('cast_shadows',record.get('cast_shadows',True))
                component.set_editor_property('source_radius',record.get('source_radius',4.0))
                component.set_editor_property('soft_source_radius',record.get('soft_source_radius',12.0))
                component.set_editor_property('light_color',unreal.Color(*[int(v*255) for v in record['color']],255))
            elif kind=='sign':
                actor=campaign_spawn(unreal.TextRenderActor,record['position'],record['id'],rotation)
                component=actor.get_component_by_class(unreal.TextRenderComponent)
                component.set_text(record['text']);component.set_world_size(record['size'])
            else:
                raise RuntimeError('Unsupported migrated campaign environment type: '+kind)

        for record in campaign_opening:
            actor=campaign_spawn(unreal.StaticMeshActor,record['position'],record['id'],record.get('rotation',[0,0,0]))
            apply_mesh(actor,free_models[record['model']],record.get('scale',[1,1,1]))
            if hlod:actor.set_editor_property('hlod_layer',hlod)

        for action in campaign_chapter['actions']:
            if action['kind'] in ('virtual','zone') or action['position'][:2]==[0,0]:
                continue
            binding=binding_by_action.get(action['id']);position=list(action['position']);rotation=[0,0,0]
            if binding:
                offset=binding.get('offset',[0,0,0])
                position=[position[i]+offset[i] for i in range(3)]
                rotation=binding.get('rotation',[0,0,0])
            actor=campaign_spawn(unreal.SliceProp,position,'action_'+action['id'],rotation)
            actor.set_editor_property('action_id',action['id'])
            if binding:apply_mesh(actor,free_models[binding['model']],binding.get('scale',[1,1,1]))
            else:actor.set_actor_scale3d(unreal.Vector(*[v/100 for v in action['size']]))

        for guard in campaign_world['guards']:
            actor=campaign_spawn(unreal.SliceEnemy,guard['position'],'guard_'+guard['id'])
            actor.set_editor_property('guard_id',guard['id'])
        for hiding in campaign_world['hides']:
            actor=campaign_spawn(unreal.WorldInteraction,hiding['position'],'hide_'+hiding['id'])
            actor.set_editor_property('definition_id',hiding['id']);actor.set_editor_property('kind','Hide')
        for camera in campaign_world['cameras']:
            actor=campaign_spawn(unreal.SurveillanceCamera,camera['position'],'camera_'+camera['id'],camera['rotation'])
            actor.set_editor_property('definition_id',camera['id'])
            monitor=campaign_spawn(unreal.WorldInteraction,[camera['position'][0]-180,camera['position'][1],camera['position'][2]-240],'monitor_'+camera['id'])
            monitor.set_editor_property('definition_id',camera['id']);monitor.set_editor_property('kind','CCTV')
        for npc in campaign_world['npc']:
            actor=campaign_spawn(unreal.ResidentNPC,npc['position'],'npc_'+npc['id'])
            actor.set_editor_property('definition_id',npc['id'])

        definition=unreal.load_asset('/Game/Generated/ChapterDefinition')
        if not definition:raise RuntimeError('Campaign ChapterDefinition missing after content preparation')
        definition.import_generated_catalog()
        if not unreal.EditorAssetLibrary.save_loaded_asset(definition,False):
            raise RuntimeError('Campaign ChapterDefinition save failed on GIS map')

    nodes={n['id']:n for n in data['road_graph']['nodes']}
    candidates=[e for e in data['road_graph']['edges'] if e['name']=='Ludwika Rydygiera' and e['car_forward'] and e['length_cm']>1500 and e['bridge']=='no']
    if not candidates:raise RuntimeError('No verified street spawn segment')
    edge=max(candidates,key=lambda e:e['length_cm']);a=unreal.Vector(*nodes[edge['from']]['position']);b=unreal.Vector(*nodes[edge['to']]['position'])
    point=(a+b)*.5;rotation=unreal.MathLibrary.find_look_at_rotation(a,b)
    if campaign_dir:
        awake=next((item for item in campaign_chapter['actions'] if item['id']=='awake'),None)
        if not awake:raise RuntimeError('Migrated campaign has no awake action')
        start_location=unreal.Vector(*awake['position'])+unreal.Vector(0,0,150)
    else:
        start_location=point+unreal.Vector(0,0,150)
    start=actors.spawn_actor_from_class(unreal.PlayerStart,start_location)
    if not start:raise RuntimeError('GIS PlayerStart creation failed')
    start.set_actor_label('CampaignGIS_PlayerStart' if campaign_dir else 'GIS_PlayerStart')
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
        definition.set_editor_property('time_limit',record['time_limit'])
        definition.set_editor_property('mode',record.get('mode','TimeTrial'))
        definition.set_editor_property('delivery',record.get('mode')=='Delivery')
        definition.set_editor_property('pursuer_count',record.get('pursuer_count',2))
        definition.set_editor_property('min_follow_distance',record.get('min_follow_distance',600))
        definition.set_editor_property('max_follow_distance',record.get('max_follow_distance',2500))
        definition.set_editor_property('lost_target_time',record.get('lost_target_time',5))
        definition.set_editor_property('navigation_hints',record.get('navigation_hints',[]))
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
