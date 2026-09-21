"""Run after prepare_content.py and Scripts/gis/build_meshes.py in Unreal 5.8.
Extends the GIS map when WTG_CITY_INPUT is set. WTG_CAMPAIGN_GIS_INPUT overlays
the validated campaign migration and switches the map to the campaign game mode.
"""
import json,traceback,os,math
from pathlib import Path
import unreal
ROOT=Path(unreal.Paths.project_dir()).resolve();MARKER=ROOT/'Saved/GeographyReady.ok'
FREE_MODEL_IMPORT_MAP=ROOT/'Saved/FreeModelImportMap.json';MODEL_BINDINGS=ROOT/'Data/model_bindings.json';FACADE_BINDINGS=ROOT/'Data/facade_asset_bindings.json';ROOF_BINDINGS=ROOT/'Data/roof_asset_bindings.json'
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
    def model_any(model_id):
        metadata=imported_models.get(model_id);object_path=metadata.get('primary_object') if metadata else None
        asset=unreal.load_asset(object_path) if object_path else None
        if not asset or not isinstance(asset,(unreal.StaticMesh,unreal.SkeletalMesh)):
            raise RuntimeError('Wrong or missing mesh model '+model_id+' -> '+str(object_path))
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
    campaign_input=os.environ.get('WTG_CAMPAIGN_GIS_INPUT')
    campaign_dir=Path(campaign_input) if campaign_input else None
    official_input=os.environ.get('WTG_OFFICIAL_BUILDINGS_INPUT')
    official_dir=Path(official_input) if official_input else None
    official_replaced=set()
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
    world.get_world_settings().set_editor_property('default_game_mode', unreal.SliceGameMode if campaign_dir else unreal.GeoPreviewGameMode)
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
    if official_dir:
        official_catalog=json.loads((official_dir/'catalog.json').read_text(encoding='utf-8'))
        official_replaced={item.get('replaced_feature_id') for item in official_catalog.get('buildings',[]) if item.get('replaced_feature_id')}
        if official_catalog.get('schema_version')!=1:
            raise RuntimeError('Unsupported official building catalogue')
        expected_origin=[float(v) for v in data['origin_projected_m']]
        actual_origin=[float(v) for v in official_catalog.get('city_origin_projected_m',[])]
        if len(actual_origin)!=3 or any(abs(a-b)>0.001 for a,b in zip(expected_origin,actual_origin)):
            raise RuntimeError('Official building catalogue uses a different GIS origin')
        official_mesh_dir=official_dir/'Meshes'
        for filename in json.loads((official_mesh_dir/'meshes.json').read_text(encoding='utf-8')):
            record=json.loads((official_mesh_dir/filename).read_text(encoding='utf-8'))
            name=Path(filename).stem
            path='/Game/Generated/OfficialBuildings/'+name
            if unreal.EditorAssetLibrary.does_asset_exist(path) and not unreal.EditorAssetLibrary.delete_asset(path):
                raise RuntimeError('Cannot replace official building mesh '+name)
            mesh=unreal.GeoMeshLibrary.bake_mesh(
                path,[unreal.Vector(*v) for v in record['vertices']],record['triangles'])
            if not mesh:raise RuntimeError('Official building mesh bake failed '+name)
            surface_name=record.get('material','Brick')
            material=unreal.load_asset('/Game/SurfaceQuality/Instances/MI_'+surface_name) or unreal.load_asset('/Game/Generated/M_'+surface_name)
            if material:mesh.set_material(0,material)
            if not unreal.EditorAssetLibrary.save_loaded_asset(mesh,False):
                raise RuntimeError('Official building mesh save failed '+name)
            actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*record['origin']))
            if not actor:raise RuntimeError('Cannot spawn official building '+name)
            actor.set_actor_label('OfficialBuilding_'+name)
            component=actor.get_component_by_class(unreal.StaticMeshComponent)
            component.set_static_mesh(mesh)
            component.set_collision_profile_name('BlockAll')
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
        facade_path=input_dir/'facades.json'
        if not facade_path.is_file():raise RuntimeError('Facade catalogue missing: '+str(facade_path))
        facade_catalog=json.loads(facade_path.read_text(encoding='utf-8'))
        if facade_catalog.get('schema_version')!=2:raise RuntimeError('Unsupported facade catalogue')
        roof_path=input_dir/'roof_details.json'
        if not roof_path.is_file():raise RuntimeError('Roof detail catalogue missing: '+str(roof_path))
        roof_catalog=json.loads(roof_path.read_text(encoding='utf-8'))
        if roof_catalog.get('schema_version')!=2:raise RuntimeError('Unsupported roof detail catalogue')
        facade_bindings=json.loads(FACADE_BINDINGS.read_text(encoding='utf-8'))
        roof_bindings=json.loads(ROOF_BINDINGS.read_text(encoding='utf-8'))
        facade_asset_scales=facade_bindings.get('asset_scales',{})
        facade_yaw_offsets=facade_bindings.get('asset_yaw_offsets_deg',{})
        facade_cell_cm=float(facade_bindings.get('instancing',{}).get('cell_size_m',128))*100
        facade_models={}
        for building in facade_catalog.get('buildings',[]):
            if building.get('building_id') in official_replaced:continue
            for item in building.get('openings',[])+building.get('details',[]):
                for field in ('asset_id','sill_asset_id','lintel_asset_id'):
                    model_id=item.get(field)
                    if model_id and not str(model_id).startswith('procedural:'):
                        facade_models.setdefault(model_id,model(model_id,unreal.StaticMesh))
        roof_models={}
        roof_ids=set(roof_bindings.get('shapes',{}).values())|set(roof_bindings.get('details',{}).values())
        for model_id in sorted(roof_ids):
            roof_models[model_id]=model(model_id,unreal.StaticMesh)
        facade_clusters={}
        def facade_cluster(position):
            key=(math.floor(position[0]/facade_cell_cm),math.floor(position[1]/facade_cell_cm))
            cluster=facade_clusters.get(key)
            if cluster:return cluster
            cluster=actors.spawn_actor_from_class(unreal.WTGFacadeInstanceCluster,unreal.Vector())
            if not cluster:raise RuntimeError('Cannot create facade instance cluster')
            cluster.set_actor_label('FacadeCluster_'+str(key[0])+'_'+str(key[1]))
            if hlod:cluster.set_editor_property('hlod_layer',hlod)
            facade_clusters[key]=cluster
            return cluster
        def facade_transform(position,yaw,scale):
            return unreal.Transform(
                location=unreal.Vector(*position),
                rotation=unreal.Rotator(0,float(yaw),0).quaternion(),
                scale=unreal.Vector(*scale))
        def modular_facade_scale(model_id,item):
            base=list(facade_asset_scales.get(model_id,[1,1,1]))
            if model_id in ('wtg-facade-window-sill','wtg-facade-window-lintel'):
                base[0]*=max(.1,float(item.get('width_m',1.0)))
            elif model_id=='wtg-facade-balcony':
                base[0]*=max(.1,float(item.get('width_m',1.0)))
                base[1]*=max(.1,float(item.get('depth_m',1.0)))
                base[2]*=max(.1,float(item.get('railing_height_m',1.0)))
            elif model_id=='wtg-facade-gutter':
                base[0]*=max(.1,float(item.get('length_m',1.0)))
                diameter=max(.04,float(item.get('diameter_m',.12)))
                base[1]*=diameter/.12;base[2]*=diameter/.12
            elif model_id=='wtg-facade-downspout':
                diameter=max(.04,float(item.get('diameter_m',.1)))
                base[0]*=diameter/.12;base[1]*=diameter/.12
                base[2]*=max(.1,float(item.get('height_m',1.0)))
            elif model_id=='wtg-facade-awning':
                base[0]*=max(.1,float(item.get('width_m',1.0)))
                base[1]*=max(.1,float(item.get('depth_m',1.0)))
            elif model_id=='wtg-facade-cornice':
                base[0]*=max(.1,float(item.get('length_m',1.0)))
                base[1]*=max(.1,float(item.get('depth_m',.3))/.3)
            elif model_id=='wtg-facade-door-step':
                base[0]*=max(.1,float(item.get('width_m',1.0)))
                base[1]*=max(.1,float(item.get('depth_m',.6))/.6)
            elif model_id=='wtg-original-sign-board':
                base[0]*=max(.25,float(item.get('width_m',1.2))/1.2)
                base[2]*=.32
            elif model_id=='wtg-original-wall-plaque':
                base[0]*=.8;base[2]*=.72
            return base
        def add_facade_mesh(item,model_id=None,position=None):
            model_id=model_id or item.get('asset_id')
            if not model_id or str(model_id).startswith('procedural:'):return
            mesh=facade_models.get(model_id)
            if not mesh:raise RuntimeError('Facade model missing: '+str(model_id))
            scale=modular_facade_scale(model_id,item)
            yaw=float(item.get('yaw_deg',0))+float(facade_yaw_offsets.get(model_id,0))
            target_position=position or item['world_position']
            facade_cluster(target_position).add_facade_instance(
                mesh,facade_transform(target_position,yaw,scale))
        def add_opening_trim(item):
            height=float(item.get('height_m',1.0))
            center=list(item['world_position'])
            sill_id=item.get('sill_asset_id')
            if sill_id:
                sill=list(center);sill[2]-=height*50
                add_facade_mesh(item,sill_id,sill)
            lintel_id=item.get('lintel_asset_id')
            if lintel_id:
                lintel=list(center);lintel[2]+=height*50
                add_facade_mesh(item,lintel_id,lintel)
        def add_facade_cube(position,yaw,scale):
            facade_cluster(position).add_facade_instance(cube,facade_transform(position,yaw,scale))
        def add_procedural_facade_detail(item):
            kind=item.get('kind');p=item.get('world_position')
            if not p:return
            yaw=float(item.get('yaw_deg',0))
            if kind=='balcony':
                width=float(item.get('width_m',1.5));depth=float(item.get('depth_m',1));rail=float(item.get('railing_height_m',1))
                add_facade_cube(p,yaw+90,[width,depth,.12])
                rad=math.radians(yaw)
                front=[p[0]+math.cos(rad)*depth*50,p[1]+math.sin(rad)*depth*50,p[2]+rail*50]
                add_facade_cube(front,yaw+90,[width,.06,rail])
            elif kind=='gutter':
                d=float(item.get('diameter_m',.12));add_facade_cube(p,yaw,[float(item.get('length_m',1)),d,d])
            elif kind=='downspout':
                d=float(item.get('diameter_m',.1));add_facade_cube(p,yaw,[d,d,float(item.get('height_m',1))])
            elif kind=='cornice':
                add_facade_cube(p,yaw,[float(item.get('length_m',1)),float(item.get('depth_m',.2)),.18])
            elif kind=='door_step':
                add_facade_cube(p,yaw+90,[float(item.get('width_m',1.4)),float(item.get('depth_m',.5)),.12])
            elif kind=='awning':
                add_facade_cube(p,yaw+90,[float(item.get('width_m',2)),float(item.get('depth_m',1)),.12])
            elif kind=='storefront_sign':
                add_facade_cube(p,yaw+90,[float(item.get('width_m',1.5)),.08,.45])
            elif kind=='address_plaque':
                add_facade_cube(p,yaw+90,[.28,.04,.18])
        for building in facade_catalog.get('buildings',[]):
            if building.get('building_id') in official_replaced:continue
            for item in building.get('openings',[]):
                add_facade_mesh(item)
                add_opening_trim(item)
            for item in building.get('details',[]):
                if str(item.get('asset_id','')).startswith('procedural:'):add_procedural_facade_detail(item)
                else:add_facade_mesh(item)
        def roof_scale(roof):
            shape=roof.get('shape','flat')
            width=max(.5,float(roof.get('width_m',1)))
            depth=max(.5,float(roof.get('depth_m',1)))
            height=max(.1,float(roof.get('height_m',.5)))
            base_height={'flat':.1,'gabled':.5,'hipped':.5,'pyramidal':.5,'mansard':.5,'dome':.5,'onion':.78}.get(shape,.5)
            z=2.0 if shape=='flat' else height/base_height
            return [width,depth,z]
        roof_instances=0
        for building in roof_catalog.get('buildings',[]):
            if building.get('building_id') in official_replaced:continue
            roof=building.get('roof',{})
            model_id=roof.get('asset_id')
            mesh=roof_models.get(model_id)
            if not mesh:raise RuntimeError('Roof model missing: '+str(model_id))
            yaw=float(roof.get('ridge_yaw_deg',0))+float(roof_bindings.get('yaw_offsets_deg',{}).get(model_id,0))
            position=roof.get('position')
            if not position:raise RuntimeError('Roof position missing: '+building.get('building_id','?'))
            facade_cluster(position).add_facade_instance(mesh,facade_transform(position,yaw,roof_scale(roof)));roof_instances+=1
            details=building.get('details',{})
            for key in ('chimney_instances','dormer_instances'):
                for item in details.get(key,[]):
                    detail_id=item.get('asset_id')
                    detail_mesh=roof_models.get(detail_id)
                    if not detail_mesh:raise RuntimeError('Roof detail model missing: '+str(detail_id))
                    detail_position=item.get('position')
                    scale=item.get('scale',[1,1,1])
                    detail_yaw=float(item.get('yaw_deg',0))+float(roof_bindings.get('yaw_offsets_deg',{}).get(detail_id,0))
                    facade_cluster(detail_position).add_facade_instance(detail_mesh,facade_transform(detail_position,detail_yaw,scale));roof_instances+=1
        unreal.log('WROCLAW_FACADE_INSTANCES '+str(sum(cluster.get_facade_instance_count() for cluster in facade_clusters.values())))
        unreal.log('WROCLAW_ROOF_INSTANCES '+str(roof_instances))
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
        population.set_editor_property('traffic_body_mesh',driveable_body_mesh);population.set_editor_property('traffic_wheel_mesh',driveable_wheel_mesh)
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
        required={item['model'] for item in campaign_opening}
        required.update(item['model'] for item in bindings)
        required.update(model_bindings['actions'].values())
        required.update(model_bindings['systems'].values())
        free_models={model_id:model_any(model_id) for model_id in sorted(required)}

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
            if actor.get_root_component()==component:actor.set_actor_scale3d(unreal.Vector(*scale))
            else:component.set_relative_scale3d(unreal.Vector(*scale))
            component.set_editor_property('cast_shadow',True)
            return component
        def hide_static_visuals(actor):
            for component in actor.get_components_by_class(unreal.StaticMeshComponent):
                component.set_visibility(False,True);component.set_editor_property('cast_shadow',False)
        def apply_skeletal(actor,mesh,scale=(1,1,1),rotation=None):
            component=actor.get_component_by_class(unreal.SkeletalMeshComponent)
            if not component:raise RuntimeError('Missing campaign SkeletalMeshComponent')
            component.set_skeletal_mesh_asset(mesh);component.set_collision_profile_name('NoCollision')
            component.set_visibility(True,True);component.set_editor_property('cast_shadow',True)
            component.set_relative_scale3d(unreal.Vector(*scale))
            capsule=actor.get_component_by_class(unreal.CapsuleComponent)
            if capsule:component.set_relative_location(unreal.Vector(0,0,-capsule.get_unscaled_capsule_half_height()))
            component.set_relative_rotation(unreal.Rotator(*(rotation or [0,-90,0])))
            hide_static_visuals(actor)
            return component
        def spawn_skeletal_visual(mesh,position,label,rotation=None,scale=(1,1,1)):
            visual=campaign_spawn(unreal.SkeletalMeshActor,position,label,rotation or [0,0,0])
            component=visual.get_component_by_class(unreal.SkeletalMeshComponent)
            if not component:raise RuntimeError('Missing campaign skeletal visual component')
            component.set_skeletal_mesh_asset(mesh);component.set_collision_profile_name('NoCollision')
            component.set_editor_property('cast_shadow',True);visual.set_actor_scale3d(unreal.Vector(*scale))
            return visual

        sign_backing_mesh=free_models[systems['environment_sign_backing']]
        street_lamp_mesh=free_models[systems['street_lamp']]
        street_lamp_ids={'environment_253','environment_255','environment_257','environment_259','environment_261','environment_263','environment_265'}
        guard_mesh=free_models[systems['enemy_guard']]
        resident_mesh=free_models[systems['resident_npc']]
        camera_mesh=free_models[systems['surveillance_camera']]
        monitor_mesh=free_models[systems['cctv_monitor']]
        hide_models={
            'container':systems['hide_container'],
            'park_hiding':systems['hide_park'],
            'garage_hiding':systems['hide_shelf'],
        }
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
                if record['id'] in street_lamp_ids:
                    position=[record['position'][0],record['position'][1],record['position'][2]-360]
                    lamp=campaign_spawn(unreal.StaticMeshActor,position,'environment_lamp_'+record['id'],rotation)
                    apply_mesh(lamp,street_lamp_mesh,[1,1,1]).set_collision_profile_name('NoCollision')
            elif kind=='sign':
                scale_x=max(.8,min(2.2,len(record['text'])/16.0))
                position=[record['position'][0],record['position'][1],record['position'][2]-110]
                backing=campaign_spawn(unreal.StaticMeshActor,position,'environment_signback_'+record['id'],rotation)
                apply_mesh(backing,sign_backing_mesh,[scale_x,1,1]).set_collision_profile_name('NoCollision')
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
            authored=binding_by_action.get(action['id'],{})
            model_id=authored.get('model') or model_bindings['actions'].get(action['id'])
            if not model_id:raise RuntimeError('Migrated physical action has no model binding: '+action['id'])
            position=list(action['position']);rotation=authored.get('rotation',[0,0,0])
            offset=authored.get('offset',[0,0,0]);position=[position[i]+offset[i] for i in range(3)]
            scale=authored.get('scale',[1,1,1])
            actor=campaign_spawn(unreal.SliceProp,position,'action_'+action['id'],rotation)
            actor.set_editor_property('action_id',action['id'])
            mesh=free_models[model_id]
            if isinstance(mesh,unreal.StaticMesh):apply_mesh(actor,mesh,scale)
            else:
                hide_static_visuals(actor)
                spawn_skeletal_visual(mesh,position,'action_visual_'+action['id'],rotation,scale)

        for guard in campaign_world['guards']:
            actor=campaign_spawn(unreal.SliceEnemy,guard['position'],'guard_'+guard['id'])
            actor.set_editor_property('guard_id',guard['id'])
            if isinstance(guard_mesh,unreal.SkeletalMesh):apply_skeletal(actor,guard_mesh)
        for hiding in campaign_world['hides']:
            actor=campaign_spawn(unreal.WorldInteraction,hiding['position'],'hide_'+hiding['id'])
            actor.set_editor_property('definition_id',hiding['id']);actor.set_editor_property('kind','Hide')
            model_id=hide_models.get(hiding['id'])
            if not model_id:raise RuntimeError('Migrated hiding spot has no model binding: '+hiding['id'])
            mesh=free_models[model_id]
            if isinstance(mesh,unreal.StaticMesh):apply_mesh(actor,mesh,[1,1,1])
        for camera in campaign_world['cameras']:
            actor=campaign_spawn(unreal.SurveillanceCamera,camera['position'],'camera_'+camera['id'],camera['rotation'])
            actor.set_editor_property('definition_id',camera['id'])
            if isinstance(camera_mesh,unreal.StaticMesh):apply_mesh(actor,camera_mesh,[1,1,1])
            monitor=campaign_spawn(unreal.WorldInteraction,[camera['position'][0]-180,camera['position'][1],camera['position'][2]-240],'monitor_'+camera['id'])
            monitor.set_editor_property('definition_id',camera['id']);monitor.set_editor_property('kind','CCTV')
            if isinstance(monitor_mesh,unreal.StaticMesh):apply_mesh(monitor,monitor_mesh,[.55,.55,.55])
        for npc in campaign_world['npc']:
            actor=campaign_spawn(unreal.ResidentNPC,npc['position'],'npc_'+npc['id'])
            actor.set_editor_property('definition_id',npc['id'])
            if isinstance(resident_mesh,unreal.SkeletalMesh):apply_skeletal(actor,resident_mesh)

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
