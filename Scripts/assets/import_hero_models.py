"""Native skeletal stage, dispatched by the existing manifest importer."""
import hashlib
import json
from pathlib import Path
import unreal

ROOT=Path(unreal.Paths.project_dir()).resolve()
DEST='/Game/Wroclaw/Hero'
class CheckedMaterialEditor:
    def __getattr__(self,name):return getattr(unreal.MaterialEditingLibrary,name)
    def connect_material_expressions(self,source,output,destination,input):
        if not unreal.MaterialEditingLibrary.connect_material_expressions(source,output,destination,input):
            raise RuntimeError('Rejected material link: '+source.get_class().get_name()+'.'+output+' -> '+destination.get_class().get_name()+'.'+input)
    def connect_material_property(self,source,output,property):
        if not unreal.MaterialEditingLibrary.connect_material_property(source,output,property):raise RuntimeError('Rejected material property: '+str(property))
EDIT=CheckedMaterialEditor()
TOOLS=unreal.AssetToolsHelpers.get_asset_tools()


def texture(path,name,normal=False):
    task=unreal.AssetImportTask();task.filename=str(ROOT/path);task.destination_path=DEST+'/Textures'
    task.destination_name=name;task.automated=True;task.replace_existing=True;task.save=True
    TOOLS.import_asset_tasks([task]);result=unreal.load_asset(DEST+'/Textures/'+name)
    if not isinstance(result,unreal.Texture2D):raise RuntimeError('Texture import failed: '+name)
    result.set_editor_property('srgb',not normal)
    result.set_editor_property('never_stream',False);result.set_editor_property('max_texture_size',2048)
    if normal:
        result.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP)
        result.set_editor_property('flip_green_channel',True) # MakeHuman's GL tangent normal -> DirectX
    else:result.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_DEFAULT)
    unreal.EditorAssetLibrary.save_loaded_asset(result,False)
    return result


def expr(material,cls):return EDIT.create_material_expression(material,cls)


def vector(material,name,color):
    n=expr(material,unreal.MaterialExpressionVectorParameter);n.set_editor_property('parameter_name',name);n.set_editor_property('default_value',unreal.LinearColor(*color,1));return n


def scalar(material,name,value):
    n=expr(material,unreal.MaterialExpressionScalarParameter);n.set_editor_property('parameter_name',name);n.set_editor_property('default_value',value);return n


def sample(material,tex):
    n=expr(material,unreal.MaterialExpressionTextureSample);n.set_editor_property('texture',tex)
    if tex.get_name().startswith('T_Pack_'):
        uv=expr(material,unreal.MaterialExpressionTextureCoordinate);uv.set_editor_property('u_tiling',8);uv.set_editor_property('v_tiling',8)
        EDIT.connect_material_expressions(uv,'',n,'')
    if tex.get_editor_property('compression_settings')==unreal.TextureCompressionSettings.TC_NORMALMAP:n.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    elif tex.get_editor_property('compression_settings')==unreal.TextureCompressionSettings.TC_MASKS:n.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    elif not tex.get_editor_property('srgb'):n.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    return n


def materials():
    source='SourceAssets/Characters/MakeHuman/'
    packs=source+'system-assets/'
    skin=texture(packs+'skins/young_caucasian_male/young_lightskinned_male_diffuse.png','T_Skin_BaseColor')
    eye=texture(source+'makehuman/data/eyes/materials/brown_eye.png','T_Eyes_BaseColor')
    cloth=texture(packs+'clothes/male_casualsuit01/male_casualsuit01_diffuse.png','T_Outfit_BaseColor')
    clothnormal=texture(packs+'clothes/male_casualsuit01/male_casualsuit01_normal.png','T_Outfit_Normal',True)
    clothao=texture(packs+'clothes/male_casualsuit01/male_casualsuit01_ao.png','T_Outfit_AO')
    clothao.set_editor_property('srgb',False);clothao.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_MASKS);unreal.EditorAssetLibrary.save_loaded_asset(clothao,False)
    shoe=texture(packs+'clothes/shoes01/shoes01_diffuse.png','T_Shoes_BaseColor')
    shoenormal=texture(packs+'clothes/shoes01/shoes01_normal.png','T_Shoes_Normal',True)
    hair=texture(packs+'hair/short02/short02_diffuse.png','T_Hair_BaseColor')
    hairnormal=texture(packs+'hair/short02/short02_normal.png','T_Hair_Normal',True)
    brows=texture(packs+'eyebrows/eyebrow001/eyebrow001.png','T_Brows_BaseColor')
    fabric=texture('SourceAssets/Characters/WroclawHero/Textures/Pack_BaseColor.png','T_Pack_BaseColor')
    fabricnormal=texture('SourceAssets/Characters/WroclawHero/Textures/Pack_Normal.png','T_Pack_Normal',True)
    fabricorm=texture('SourceAssets/Characters/WroclawHero/Textures/Pack_ORM.png','T_Pack_ORM')
    fabricorm.set_editor_property('srgb',False);fabricorm.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_MASKS);unreal.EditorAssetLibrary.save_loaded_asset(fabricorm,False)
    result={}
    for kind,tex,normal,rough in [('Skin',skin,None,.5),('Eyes',eye,None,.12),('Shirt',cloth,clothnormal,.82),
                                ('Denim',cloth,clothnormal,.87),('Shoe',shoe,shoenormal,.66),
                                ('Hair',hair,hairnormal,.46),('Brows',brows,None,.62),('Pack',fabric,fabricnormal,.86)]:
        path=DEST+'/Materials/M_'+kind
        mat=unreal.load_asset(path) or TOOLS.create_asset('M_'+kind,DEST+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
        EDIT.delete_all_material_expressions(mat)
        mat.set_editor_property('used_with_skeletal_mesh',True)
        mat.set_editor_property('used_with_morph_targets',True)
        mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED if kind in ['Hair','Brows','Eyes'] else unreal.BlendMode.BLEND_OPAQUE)
        if kind in ['Hair','Brows']:mat.set_editor_property('two_sided',True)
        tint=vector(mat,'SkinTone' if kind=='Skin' else ('EyeColor' if kind=='Eyes' else 'BaseColor'),(.58,.36,.24) if kind=='Skin' else (.08,.09,.10))
        color=tint
        if tex:
            s=sample(mat,tex)
            if kind in ['Hair','Brows','Eyes']:EDIT.connect_material_property(s,'A',unreal.MaterialProperty.MP_OPACITY_MASK)
            if kind in ['Skin','Eyes','Hair','Brows']:
                # Preserve source anatomy/strand detail; tint remains a gameplay parameter.
                mult=expr(mat,unreal.MaterialExpressionMultiply);EDIT.connect_material_expressions(s,'RGB',mult,'A');EDIT.connect_material_expressions(tint,'RGB',mult,'B')
                if kind=='Skin':
                    div=expr(mat,unreal.MaterialExpressionDivide);EDIT.connect_material_expressions(mult,'',div,'A');EDIT.connect_material_expressions(vector(mat,'SkinReference',(.58,.36,.24)),'RGB',div,'B');color=div
                elif kind=='Eyes':
                    # Keep white sclera and pupils; recolor only the brown iris.
                    mask=expr(mat,unreal.MaterialExpressionSubtract);EDIT.connect_material_expressions(s,'R',mask,'A');EDIT.connect_material_expressions(s,'B',mask,'B')
                    iris=expr(mat,unreal.MaterialExpressionLinearInterpolate);EDIT.connect_material_expressions(s,'RGB',iris,'A');EDIT.connect_material_expressions(mult,'',iris,'B');EDIT.connect_material_expressions(mask,'',iris,'Alpha');color=iris
                else:
                    div=expr(mat,unreal.MaterialExpressionDivide);EDIT.connect_material_expressions(mult,'',div,'A');EDIT.connect_material_expressions(scalar(mat,'HairReference',.12),'',div,'B');color=div
            else:
                dot=expr(mat,unreal.MaterialExpressionDesaturation);EDIT.connect_material_expressions(s,'RGB',dot,'')
                EDIT.connect_material_expressions(scalar(mat,'Desaturation',1),'',dot,'Fraction')
                mult=expr(mat,unreal.MaterialExpressionMultiply);EDIT.connect_material_expressions(dot,'',mult,'A');EDIT.connect_material_expressions(tint,'RGB',mult,'B');color=mult
                if kind in ['Denim','Pack']:
                    div=expr(mat,unreal.MaterialExpressionDivide);EDIT.connect_material_expressions(mult,'',div,'A');EDIT.connect_material_expressions(scalar(mat,'AlbedoReference',.3 if kind=='Denim' else .38),'',div,'B');color=div
        EDIT.connect_material_property(color,'RGB' if color==tint else '',unreal.MaterialProperty.MP_BASE_COLOR)
        if kind=='Pack':EDIT.connect_material_property(sample(mat,fabricorm),'G',unreal.MaterialProperty.MP_ROUGHNESS)
        else:EDIT.connect_material_property(scalar(mat,'Roughness',rough),'',unreal.MaterialProperty.MP_ROUGHNESS)
        EDIT.connect_material_property(scalar(mat,'Specular',.3 if kind=='Skin' else .4),'',unreal.MaterialProperty.MP_SPECULAR)
        if normal:EDIT.connect_material_property(sample(mat,normal),'RGB',unreal.MaterialProperty.MP_NORMAL)
        if kind=='Skin':
            # Subtle variation follows the licensed anatomical texture. No
            # random surface noise or large invented pore displacement.
            luma=expr(mat,unreal.MaterialExpressionDesaturation);EDIT.connect_material_expressions(sample(mat,skin),'RGB',luma,'')
            EDIT.connect_material_expressions(scalar(mat,'SkinRoughnessLuma',1),'',luma,'Fraction')
            variation=expr(mat,unreal.MaterialExpressionMultiply);variation.set_editor_property('const_b',.10);EDIT.connect_material_expressions(luma,'',variation,'A')
            roughness=expr(mat,unreal.MaterialExpressionAdd);roughness.set_editor_property('const_b',.44);EDIT.connect_material_expressions(variation,'',roughness,'A')
            EDIT.connect_material_property(roughness,'',unreal.MaterialProperty.MP_ROUGHNESS)
            mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_SUBSURFACE)
            EDIT.connect_material_property(vector(mat,'Scattering',(.30,.12,.065)),'RGB',unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
            EDIT.connect_material_property(scalar(mat,'SkinDensity',.86),'',unreal.MaterialProperty.MP_OPACITY)
        if kind in ['Shirt','Denim']:EDIT.connect_material_property(sample(mat,clothao),'R',unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
        if kind=='Hair':
            mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_HAIR)
            cross=expr(mat,unreal.MaterialExpressionCrossProduct)
            EDIT.connect_material_expressions(expr(mat,unreal.MaterialExpressionVertexNormalWS),'',cross,'A')
            EDIT.connect_material_expressions(expr(mat,unreal.MaterialExpressionVertexTangentWS),'',cross,'B')
            EDIT.connect_material_property(cross,'',unreal.MaterialProperty.MP_TANGENT)
        EDIT.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat,False);result[kind]=mat
    return result


def prepare(manifest):
    unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
    out=ROOT/'Saved/Pipeline/import';out.mkdir(parents=True,exist_ok=True)
    marker=out/(manifest['id']+'.ok');marker.unlink(missing_ok=True)
    for record in json.loads((ROOT/manifest['skeletal']['sources']).read_text())['assets']:
        if hashlib.sha256((ROOT/record['path']).read_bytes()).hexdigest()!=record['sha256']:
            raise RuntimeError('Source hash mismatch: '+record['path'])
    mats=materials();metadata=json.loads((ROOT/manifest['skeletal']['geometry']).read_text());meshes={};reports=[]
    editor=unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
    skeleton=unreal.load_asset(DEST+'/SK_CivilianSkeleton')
    if not skeleton:
        existing_body=unreal.load_asset(DEST+'/Meshes/SK_Civilian_Body')
        if existing_body:
            skeleton=unreal.LocomotionAssetLibrary.create_civilian_skeleton(existing_body)
            if not skeleton:raise RuntimeError('Civilian skeleton creation failed')
    for record in metadata['assets']:
        source=ROOT/record['fbx']
        if hashlib.sha256(source.read_bytes()).hexdigest()!=record['fbx_sha256']:raise RuntimeError('Stale export '+record['id'])
        name='SK_Civilian_'+record['id'].removeprefix('male_');path=DEST+'/Meshes/'+name
        existing=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
        options=unreal.FbxImportUI();options.import_mesh=True;options.import_as_skeletal=True
        options.import_materials=False;options.import_textures=False;options.import_animations=False
        options.mesh_type_to_import=unreal.FBXImportType.FBXIT_SKELETAL_MESH;options.automated_import_should_detect_type=False
        options.create_physics_asset=record['kind']=='body'
        data=options.get_editor_property('skeletal_mesh_import_data');data.set_editor_property('import_morph_targets',True);data.set_editor_property('morph_threshold_position',.0001)
        data.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
        data.set_editor_property('import_content_type',unreal.FBXImportContentType.FBXICT_ALL)
        options.set_editor_property('original_import_type',unreal.FBXImportType.FBXIT_SKELETAL_MESH)
        if skeleton:options.skeleton=skeleton
        if existing:
            if not unreal.LocomotionAssetLibrary.configure_civilian_lod_settings(existing,skeleton):raise RuntimeError('Existing rig setup failed '+name)
            stored=existing.get_editor_property('asset_import_data');stored.set_editor_property('import_morph_targets',True);stored.set_editor_property('morph_threshold_position',.0001)
            stored.set_editor_property('import_content_type',unreal.FBXImportContentType.FBXICT_ALL)
            if editor.import_lod(existing,0,str(source))!=0:raise RuntimeError('Reimport failed '+name)
            mesh=existing
        else:
            task=unreal.AssetImportTask();task.filename=str(source);task.destination_path=DEST+'/Meshes';task.destination_name=name
            task.automated=True;task.replace_existing=True;task.save=True;task.options=options;task.factory=unreal.FbxFactory()
            TOOLS.import_asset_tasks([task]);mesh=unreal.load_asset(path)
        if not isinstance(mesh,unreal.SkeletalMesh):raise RuntimeError('Missing native mesh '+name)
        if not skeleton:skeleton=mesh.get_editor_property('skeleton')
        if not skeleton:
            skeleton=unreal.LocomotionAssetLibrary.create_civilian_skeleton(mesh)
        if not unreal.LocomotionAssetLibrary.configure_civilian_lod_settings(mesh,skeleton):raise RuntimeError('Rig/LOD settings failed '+name)
        if mesh.get_editor_property('skeleton')!=skeleton:raise RuntimeError('Skeleton mismatch '+name)
        slots=list(mesh.get_editor_property('materials'))
        for slot in slots:
            role=str(slot.get_editor_property('material_slot_name'));slot.set_editor_property('material_interface',mats[role if role in mats else 'Skin'])
        mesh.set_editor_property('materials',slots)
        if not editor.regenerate_lod(mesh,3,True,False):raise RuntimeError('LOD generation failed '+name)
        unreal.EditorAssetLibrary.save_loaded_asset(mesh,False)
        names=[m.get_name() for m in mesh.get_editor_property('morph_targets')]
        unreal.log('WTG_HERO_IMPORTED_MORPHS '+name+' '+json.dumps(names))
        for morph in record['morphs']:
            if not any(n==morph['name'] or n.endswith('_'+morph['name']) for n in names):raise RuntimeError('Missing morph '+morph['name'])
        meshes[record['id']]=mesh;reports.append(dict(id=record['id'],mesh=mesh.get_path_name(),lods=editor.get_lod_count(mesh),morphs=names))
    catalog=unreal.load_asset(DEST+'/DA_CivilianAppearance')
    unreal.EditorAssetLibrary.save_loaded_asset(skeleton,False)
    if not catalog:catalog=unreal.EditorAssetLibrary.duplicate_asset('/Game/CharacterCreator/DA_CharacterAppearanceCatalog',DEST+'/DA_CivilianAppearance')
    bodies=list(catalog.bodies);body=next(b for b in bodies if str(b.id)=='Male.Standard');hero=meshes['male_Body']
    physics=hero.get_editor_property('physics_asset')
    if not physics:
        physics=unreal.LocomotionAssetLibrary.create_civilian_physics(hero)
        if not physics:raise RuntimeError('Native body physics asset creation failed')
        hero.set_editor_property('physics_asset',physics)
        unreal.EditorAssetLibrary.save_loaded_asset(physics,False);unreal.EditorAssetLibrary.save_loaded_asset(hero,False)
    original=next(b for b in unreal.load_asset('/Game/CharacterCreator/DA_CharacterAppearanceCatalog').bodies if str(b.id)=='Male.Standard')
    animation_cache={}
    def animation(source):
        if not source:return None
        key=source.get_path_name()
        if key in animation_cache:return animation_cache[key]
        path=DEST+'/Animations/'+source.get_name()
        result=unreal.load_asset(path) or unreal.EditorAssetLibrary.duplicate_asset(key,path)
        if isinstance(result,unreal.BlendSpace):
            # Always rebuild from original samples. Reusing the previous output
            # feeds already converted clips back into their own retarget.
            samples=list(source.get_editor_property('sample_data'))
            for s in samples:s.set_editor_property('animation',animation(s.get_editor_property('animation')))
            result.set_editor_property('sample_data',samples)
        if not unreal.LocomotionAssetLibrary.retarget_civilian_animation(result,hero,source):raise RuntimeError('Retarget failed '+key)
        unreal.EditorAssetLibrary.save_loaded_asset(result,False);animation_cache[key]=result;return result
    body.mesh=hero;body.face_mesh=None;body.skin_material=mats['Skin'];body.reference_height=180;body.placeholder=False
    for field in ['creator_idle','walk','jog','crouch','locomotion','jump_loop']:body.set_editor_property(field,animation(original.get_editor_property(field)))
    idle_metadata=json.loads((ROOT/'SourceAssets/Characters/WroclawHero/Animations/animations.json').read_text())
    idle_source=ROOT/idle_metadata['fbx']
    if hashlib.sha256(idle_source.read_bytes()).hexdigest()!=idle_metadata['sha256']:raise RuntimeError('Stale civilian idle export')
    options=unreal.FbxImportUI();options.set_editor_property('import_mesh',False)
    options.set_editor_property('import_animations',True);options.set_editor_property('skeleton',skeleton)
    options.set_editor_property('mesh_type_to_import',unreal.FBXImportType.FBXIT_ANIMATION)
    options.set_editor_property('original_import_type',unreal.FBXImportType.FBXIT_ANIMATION)
    options.set_editor_property('automated_import_should_detect_type',False)
    task=unreal.AssetImportTask();task.filename=str(idle_source);task.destination_path=DEST+'/Animations'
    task.destination_name=idle_metadata['name']+'_Source';task.automated=True;task.replace_existing=True;task.save=True
    task.options=options;task.factory=unreal.FbxFactory();TOOLS.import_asset_tasks([task])
    idle_source_asset=unreal.load_asset(DEST+'/Animations/'+idle_metadata['name']+'_Source')
    if not isinstance(idle_source_asset,unreal.AnimSequence):raise RuntimeError('Authored civilian idle import failed: '+str(task.imported_object_paths))
    idle=unreal.load_asset(DEST+'/Animations/'+idle_metadata['name']) or unreal.EditorAssetLibrary.duplicate_asset(idle_source_asset.get_path_name(),DEST+'/Animations/'+idle_metadata['name'])
    if not unreal.LocomotionAssetLibrary.retarget_civilian_animation(idle,hero,idle_source_asset):raise RuntimeError('Civilian idle bind normalization failed')
    unreal.EditorAssetLibrary.save_loaded_asset(idle,False)
    body.creator_idle=idle
    samples=list(body.locomotion.get_editor_property('sample_data'))
    for sample_data in samples:
        if sample_data.get_editor_property('sample_value').x<1:sample_data.set_editor_property('animation',idle)
        else:
            clip=sample_data.get_editor_property('animation')
            measured=unreal.LocomotionAssetLibrary.measure_civilian_stride(clip,hero)
            if measured<=20:raise RuntimeError('Cannot measure stance stride '+clip.get_name())
            rate=sample_data.get_editor_property('sample_value').x/measured
            if not .25<rate<3:
                # The source sprint has an incompatible stance cadence on this
                # anatomy. Use the measured run cycle at the sprint speed.
                if sample_data.get_editor_property('sample_value').x<500:raise RuntimeError('Implausible stride correction '+clip.get_name()+' '+str(rate))
                if not unreal.LocomotionAssetLibrary.retarget_civilian_animation(clip,hero,original.jog):raise RuntimeError('Sprint run-cycle conversion failed')
                measured=unreal.LocomotionAssetLibrary.measure_civilian_stride(clip,hero)
                rate=sample_data.get_editor_property('sample_value').x/measured
                if not .25<rate<3:raise RuntimeError('Sprint cadence still invalid')
            clip.set_editor_property('rate_scale',rate);unreal.EditorAssetLibrary.save_loaded_asset(clip,False)
            unreal.log('WTG_CIVILIAN_STRIDE '+clip.get_name()+' source_cm_s='+str(measured)+' rate='+str(rate))
    body.locomotion.set_editor_property('sample_data',samples)
    unreal.EditorAssetLibrary.save_loaded_asset(body.locomotion,False)
    body.animation_class=unreal.load_class(None,'/Script/WroclawTheGame.CivilianAnimInstance')
    if not body.animation_class:raise RuntimeError('Civilian animation class is not compiled')
    animation_cache['authored_idle']=idle
    catalog.bodies=bodies
    morphs=list(catalog.morphs)
    for m in morphs:
        suffix=str(m.morph_id).replace('.','_');m.morph_target_name=next((n for n in reports[0]['morphs'] if n==suffix or n.endswith('_'+suffix)),suffix)
    catalog.morphs=morphs
    mapping={'Jacket01':'Shirt','Jacket02':'Shirt','TShirt01':'Shirt','TShirt02':'Shirt','Jeans01':'Trousers','Jeans02':'Trousers','Sneakers01':'Shoes','Sneakers02':'Shoes','Backpack01':'Backpack'}
    for field,parts_map in [('hair_style_definitions',{'Short':'HairShort'}),('eyebrow_style_definitions',{'Natural':'BrowsNatural'}),('clothing_definitions',mapping)]:
        parts=list(catalog.get_editor_property(field))
        for p in parts:
            if str(p.id) not in parts_map:continue
            name=parts_map[str(p.id)];p.set_editor_property('body_meshes',{'Male.Standard':meshes['male_'+name]});p.material=None
            p.offset=unreal.Transform()
            if field=='clothing_definitions':p.hidden_body_regions=['Torso','Arms'] if name=='Shirt' else (['Legs'] if name=='Trousers' else (['Feet'] if name=='Shoes' else []))
            if name=='Trousers':
                variants=list(p.variants);variants[1].color=unreal.LinearColor(.28,.28,.28,1);p.variants=variants
        catalog.set_editor_property(field,parts)
    errors=unreal.CharacterCreatorValidator.validate_catalog(catalog,True)
    if errors:raise RuntimeError('\n'.join(errors))
    if not unreal.EditorAssetLibrary.save_loaded_asset(catalog,False):raise RuntimeError('Catalog save failed')
    report=dict(asset_id=manifest['id'],status='PASS',type='skeletal_mesh',meshes=reports,mesh=hero.get_path_name(),
                skeleton=skeleton.get_path_name(),physics_asset=physics.get_path_name(),catalog=catalog.get_path_name(),animations=[a.get_path_name() for a in animation_cache.values()])
    (out/(manifest['id']+'.json')).write_text(json.dumps(report,indent=2));marker.write_text('PASS\n')
    unreal.log('WTG_HERO_NATIVE_PASS '+str(len(reports)))
    return report
