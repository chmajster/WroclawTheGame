"""Inspect native texture channels, material bindings and daylight policy."""
import json
import sys
import traceback
from pathlib import Path
import unreal
ROOT=Path(unreal.Paths.project_dir()).resolve();sys.path.insert(0,str(ROOT/'Pipeline/unreal'))
from import_asset import quit_after_editor_startup
try:
    dest='/Game/Wroclaw/StartLocation/Materials';e=unreal.MaterialEditingLibrary
    records=[]
    for name in ['Grass','Stone','Roof','Plaster','Pavement','Asphalt','Cobble','CurbConcrete']:
        mi=unreal.load_asset(dest+'/MI_'+name);textures=[]
        for param,kind in [('BaseColor','base_color'),('Normal','normal'),('ORM','orm')]:
            t=e.get_material_instance_texture_parameter_value(mi,param)
            compression=t.get_editor_property('compression_settings');srgb=t.get_editor_property('srgb')
            expected={'base_color':unreal.TextureCompressionSettings.TC_DEFAULT,'normal':unreal.TextureCompressionSettings.TC_NORMALMAP,'orm':unreal.TextureCompressionSettings.TC_MASKS}[kind]
            if bool(srgb)!=(kind=='base_color') or compression!=expected or t.get_editor_property('max_texture_size')>2048:raise RuntimeError('PBR texture interpretation '+t.get_path_name())
            textures.append(dict(parameter=param,texture=t.get_path_name(),srgb=srgb,compression=str(compression),max_size=t.get_editor_property('max_texture_size')))
        records.append(dict(material=mi.get_path_name(),textures=textures,tint=str(e.get_material_instance_vector_parameter_value(mi,'Tint'))))
    for name in ['Glazing','Paint','CoatedMetal']:
        mat=unreal.load_asset(dest+'/M_Start_'+name)
        node=e.get_material_property_input_node(mat,unreal.MaterialProperty.MP_METALLIC)
        if not isinstance(node,unreal.MaterialExpressionConstant) or node.get_editor_property('r')!=0:raise RuntimeError('Dielectric metallic '+name)
    marker=ROOT/'Saved/StartLocation/pbr.ok';marker.write_text('PASS\n')
    (marker.parent/'pbr-report.json').write_text(json.dumps(dict(status='PASS',materials=records,glass='Opaque reflection approximation; no modelled interiors',daylight='110000 lux sun; sky intensity2.2; cycle remains active. QA13:00 only.'),indent=2),encoding='utf-8')
    unreal.log('WTG_START_PBR_PASS')
except Exception:unreal.log_error(traceback.format_exc());raise
finally:quit_after_editor_startup()
