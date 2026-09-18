"""Non-destructive-to-maps PBR build. Owns /Game/SurfaceQuality only.

Regenerates owned master graphs; preserves existing instance edits unless --reset
is explicitly supplied via WTG_SURFACE_RESET=1. Does not delete maps or actors.
"""
import json
import os
from pathlib import Path
import sys
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0, str(ROOT/'Scripts'))
from surface_profiles import PROFILES
from make_surface_assets import generate

DEST = '/Game/SurfaceQuality'
MARKER = ROOT/'Saved/SurfaceQualityReady.ok'
EDIT = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()


def save(asset):
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, False):
        raise RuntimeError('Cannot save '+asset.get_path_name())


def make_asset(name, folder, cls, factory):
    old = unreal.load_asset(folder+'/'+name)
    return old or TOOLS.create_asset(name, folder, cls, factory)


def expression(material, cls, **properties):
    node = EDIT.create_material_expression(material, cls)
    for key, value in properties.items():
        node.set_editor_property(key, value)
    return node


def scalar(mat, name, value):
    return expression(mat, unreal.MaterialExpressionScalarParameter, parameter_name=name, default_value=value)


def vector(mat, name, value):
    return expression(mat, unreal.MaterialExpressionVectorParameter, parameter_name=name, default_value=unreal.LinearColor(*value))


def custom(mat, code, inputs, outputs=()):
    custom_inputs=[]
    custom_outputs=[]
    for name in inputs:
        item=unreal.CustomInput();item.set_editor_property('input_name',name);custom_inputs.append(item)
    for name,kind in outputs:
        item=unreal.CustomOutput();item.set_editor_property('output_name',name);item.set_editor_property('output_type',kind);custom_outputs.append(item)
    node = expression(mat, unreal.MaterialExpressionCustom, code=code,
                      output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3,
                      inputs=custom_inputs, additional_outputs=custom_outputs)
    for name, (source, channel) in inputs.items():
        if not EDIT.connect_material_expressions(source, channel, node, name):
            raise RuntimeError('Custom expression input failed: '+name)
    return node


def output(node, pin, material, prop):
    if not EDIT.connect_material_property(node, pin, prop):
        raise RuntimeError('Material output connection failed: '+str(prop))


def import_texture(path, suffix):
    old=unreal.load_asset(DEST+'/Textures/'+path.stem)
    if old and os.environ.get('WTG_SURFACE_REIMPORT')!='1':
        validate_texture(old,suffix)
        return old
    task = unreal.AssetImportTask()
    for key, value in {'filename':str(path), 'destination_path':DEST+'/Textures',
                       'automated':True, 'replace_existing':True, 'save':False}.items():
        task.set_editor_property(key,value)
    TOOLS.import_asset_tasks([task])
    paths = task.get_editor_property('imported_object_paths')
    if not paths:
        raise RuntimeError('Texture import failed: '+str(path))
    texture = unreal.load_asset(paths[0])
    texture.set_editor_property('srgb', suffix=='BC')
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP if suffix=='N' else
                                (unreal.TextureCompressionSettings.TC_MASKS if suffix=='ARM' else unreal.TextureCompressionSettings.TC_DEFAULT))
    texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_WORLD_NORMAL_MAP if suffix=='N' else unreal.TextureGroup.TEXTUREGROUP_WORLD)
    texture.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
    texture.set_editor_property('never_stream',False)
    texture.set_editor_property('virtual_texture_streaming',False)
    texture.set_editor_property('max_texture_size',2048)
    save(texture)
    validate_texture(texture,suffix)
    return texture


def validate_texture(texture,suffix):
    expected=unreal.TextureCompressionSettings.TC_NORMALMAP if suffix=='N' else (unreal.TextureCompressionSettings.TC_MASKS if suffix=='ARM' else unreal.TextureCompressionSettings.TC_DEFAULT)
    if texture.get_editor_property('srgb') != (suffix=='BC') or texture.get_editor_property('compression_settings')!=expected:
        raise RuntimeError('Incorrect color space/compression: '+texture.get_path_name())
    if texture.get_editor_property('never_stream') or texture.get_editor_property('mip_gen_settings')==unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS:
        raise RuntimeError('Streaming/mips disabled: '+texture.get_path_name())


def master(kind, textures):
    mat = make_asset('M_Master_'+kind, DEST+'/Masters', unreal.Material, unreal.MaterialFactoryNew())
    EDIT.delete_all_material_expressions(mat)
    uv = kind in ('CharacterClothing','Skin')
    mat.set_editor_property('tangent_space_normal', uv)
    if kind in ('Glass','Water'):
        mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
        mat.set_editor_property('translucency_lighting_mode',unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
        mat.set_editor_property('two_sided',True)
        output(scalar(mat,'Opacity',.24 if kind=='Glass' else .7),'',mat,unreal.MaterialProperty.MP_OPACITY)
    if kind=='Skin':
        mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_SUBSURFACE)
        output(vector(mat,'SubsurfaceColor',(.30,.095,.055,1)),'RGB',mat,unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
        output(scalar(mat,'SubsurfaceAmount',.15),'',mat,unreal.MaterialProperty.MP_OPACITY)
    nodes = {
        'Position':(expression(mat,unreal.MaterialExpressionWorldPosition),''),
        'GeometricNormal':(expression(mat,unreal.MaterialExpressionVertexNormalWS),''),
        'CameraPosition':(expression(mat,unreal.MaterialExpressionCameraPositionWS),''),
        'UV':(expression(mat,unreal.MaterialExpressionTextureCoordinate),''),
        'Tint':(vector(mat,'Tint',(1,1,1,1)),'RGB'),
    }
    vertex=expression(mat,unreal.MaterialExpressionVertexColor)
    rgba=expression(mat,unreal.MaterialExpressionAppendVector)
    if not EDIT.connect_material_expressions(vertex,'',rgba,'A') or not EDIT.connect_material_expressions(vertex,'A',rgba,'B'):
        raise RuntimeError('Vertex mask connection failed')
    nodes['VertexMask']=(rgba,'')
    defaults = {'TileSizeCm':200,'UVScale':1,'UseUV':float(uv),'NormalStrength':1,'DetailTiling':12,
                'DetailStrength':.18,'DetailFadeCm':1200,'GroundLevelCm':0,'DirtHeightCm':120,
                'DirtAmount':.18,'RainStreakAmount':.3,'WearAmount':.15,'Wetness':0,
                'WeatherExposure':0,'OxidationAmount':0,'ColorVariation':.055,'RoughnessScale':1,'RoughnessVariation':.045}
    for name,value in defaults.items():
        nodes[name]=(scalar(mat,name,value),'')
    for name,tex in {'Albedo':textures['Concrete']['BC'],'ARM':textures['Concrete']['ARM'],
                     'NormalMap':textures['Concrete']['N'],'DetailNormal':textures['Concrete']['N']}.items():
        nodes[name]=(expression(mat,unreal.MaterialExpressionTextureObjectParameter,parameter_name=name,texture=tex),'')
    f1,f3=unreal.CustomMaterialOutputType.CMOT_FLOAT1,unreal.CustomMaterialOutputType.CMOT_FLOAT3
    node=custom(mat,(ROOT/'Scripts/surface_shader.ush').read_text(),nodes,
                [('RoughnessOut',f1),('MetallicOut',f1),('AOOut',f1),('NormalOut',f3)])
    for pin,prop in [('',unreal.MaterialProperty.MP_BASE_COLOR),('RoughnessOut',unreal.MaterialProperty.MP_ROUGHNESS),
                     ('MetallicOut',unreal.MaterialProperty.MP_METALLIC),('AOOut',unreal.MaterialProperty.MP_AMBIENT_OCCLUSION),
                     ('NormalOut',unreal.MaterialProperty.MP_NORMAL)]:
        output(node,pin,mat,prop)
    EDIT.layout_material_expressions(mat)
    EDIT.recompile_material(mat)
    save(mat)
    return mat


def decal_master():
    mat=make_asset('M_Master_Decal',DEST+'/Masters',unreal.Material,unreal.MaterialFactoryNew())
    EDIT.delete_all_material_expressions(mat)
    mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('material_domain',unreal.MaterialDomain.MD_DEFERRED_DECAL)
    inputs={'UV':(expression(mat,unreal.MaterialExpressionTextureCoordinate),''),
            'Kind':(scalar(mat,'Kind',0),''),'Wear':(scalar(mat,'PaintWearAmount',.25),'')}
    node=custom(mat, '''float2 p=UV;
float edge=saturate(min(min(p.x,1-p.x),min(p.y,1-p.y))*20);
float grit=frac(sin(dot(floor(p*180),float2(127.1,311.7)))*43758.5453);
float crack=abs(p.y-.5-.06*sin(p.x*32)-.022*sin(p.x*93));
float mask=edge;
if(Kind>1.5 && Kind<3.5) mask*=1-smoothstep(.007,.018,crack);
if(Kind>3.5 && Kind<4.5) mask*=saturate((1-p.y)*2)*(.4+.6*pow(abs(sin(p.x*31)),8));
if(Kind>4.5) mask*=step(Wear,grit);
OpacityOut=mask*.8; return float3(1,1,1);''',inputs,[('OpacityOut',unreal.CustomMaterialOutputType.CMOT_FLOAT1)])
    output(node,'OpacityOut',mat,unreal.MaterialProperty.MP_OPACITY)
    output(vector(mat,'DecalColor',(.025,.023,.019,1)),'RGB',mat,unreal.MaterialProperty.MP_BASE_COLOR)
    output(scalar(mat,'Roughness',.8),'',mat,unreal.MaterialProperty.MP_ROUGHNESS)
    EDIT.layout_material_expressions(mat); EDIT.recompile_material(mat); save(mat)
    return mat


def prepare():
    MARKER.unlink(missing_ok=True)
    directory=ROOT/'Saved/SurfaceSource'
    manifest_path=directory/'manifest.json'
    manifest=json.loads(manifest_path.read_text()) if manifest_path.exists() else generate(directory)
    textures={}
    missing=[name for name in PROFILES if name not in manifest['profiles']]
    if missing:
        addition=generate(directory/'Additional',1024,missing)
        for name,record in addition['profiles'].items():
            textures[name]={suffix:import_texture(directory/'Additional'/item['file'],suffix) for suffix,item in record['files'].items()}
            for item in record['files'].values():item['file']='Additional/'+item['file']
            manifest['profiles'][name]=record
        manifest_path.write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    for name in PROFILES:
        if name in textures:continue
        textures[name]={suffix:import_texture(directory/item['file'],suffix)
                        for suffix,item in manifest['profiles'][name]['files'].items()}
    source_path=ROOT/'Data/surface_sources.json'
    sources=json.loads(source_path.read_text())['profiles'] if source_path.exists() else {}
    for name,record in sources.items():
        textures[name]={suffix:import_texture(directory/'PolyHaven'/item['file'],suffix)
                        for suffix,item in record['files'].items()}
    masters={kind:master(kind,textures) for kind in sorted({p[6] for p in PROFILES.values()})}
    report={'profiles':[], 'warnings':['Six CC0 Poly Haven sets plus procedural starter surfaces. Hero geometry and UV QA still required.']}
    reset=os.environ.get('WTG_SURFACE_RESET')=='1'
    for name,profile in PROFILES.items():
        path=DEST+'/Instances/MI_'+name
        existed=unreal.EditorAssetLibrary.does_asset_exist(path)
        instance=make_asset('MI_'+name,DEST+'/Instances',unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
        EDIT.set_material_instance_parent(instance,masters[profile[6]])
        current=EDIT.get_material_instance_texture_parameter_value(instance,'Albedo')
        migrate_cc0=name in sources and current and current.get_name()=='T_'+name+'_BC'
        if not existed or reset or migrate_cc0:
            for param,suffix in [('Albedo','BC'),('NormalMap','N'),('ARM','ARM')]:
                EDIT.set_material_instance_texture_parameter_value(instance,param,textures[name][suffix])
            detail='Fabric' if profile[6]=='CharacterClothing' else ('Skin' if name=='Skin' else 'Concrete')
            EDIT.set_material_instance_texture_parameter_value(instance,'DetailNormal',textures[detail]['N'])
            for param,value in {'TileSizeCm':sources.get(name,{}).get('tile_cm',profile[3]), 'DirtAmount':.18 if profile[6]=='Building' else .05,
                                'WeatherExposure':1 if profile[6] in ('Road','Ground') else 0,
                                'RoughnessVariation':.025 if name=='Glass' else .045}.items():
                EDIT.set_material_instance_scalar_parameter_value(instance,param,value)
        EDIT.update_material_instance(instance);save(instance)
        report['profiles'].append({'name':name,'master':profile[6],'tile_cm':sources.get(name,{}).get('tile_cm',profile[3]),
            'resolution':2048 if name in sources else manifest['profiles'][name]['resolution'],
            'source':sources.get(name,{}).get('page','Original procedural starter')})
    decal=decal_master()
    for kind,(name,color,rough) in enumerate([
        ('RoadPatchFresh',(.018,.019,.020,1),.79),('RoadPatchOld',(.05,.049,.044,1),.91),
        ('RoadCrack',(.008,.007,.006,1),.95),('RoadSeal',(.012,.013,.014,1),.48),
        ('RainStreak',(.04,.035,.025,1),.88),('RoadPaintWorn',(.62,.59,.48,1),.69)]):
        instance=make_asset('MI_'+name,DEST+'/Decals',unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
        EDIT.set_material_instance_parent(instance,decal)
        EDIT.set_material_instance_scalar_parameter_value(instance,'Kind',kind)
        EDIT.set_material_instance_scalar_parameter_value(instance,'Roughness',rough)
        EDIT.set_material_instance_vector_parameter_value(instance,'DecalColor',unreal.LinearColor(*color))
        EDIT.update_material_instance(instance);save(instance)
    errors=unreal.SurfaceQualityLibrary.validate_surface_shaders(list(masters.values())+[decal])
    if errors:
        raise RuntimeError('SM6 shader validation failed:\n'+'\n'.join(errors))
    report['shader_validation']='SM6 compiled, no fallback'
    report['master_statistics']={}
    for kind,material in masters.items():
        stats=EDIT.get_statistics(material)
        report['master_statistics'][kind]={field:stats.get_editor_property(field) for field in
            ('num_pixel_shader_instructions','num_samplers','num_pixel_texture_samples')}
    (ROOT/'Saved/SurfaceQualityReport.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    MARKER.write_text('PBR surfaces imported; SM6 shader validation passed.\n',encoding='utf-8')
    unreal.log('WTG_SURFACE_QUALITY_READY')


if __name__=='__main__':
    prepare()
