"""Native masked foliage/rough bark/soil interfaces, stable paths."""
import sys
import traceback
from pathlib import Path
import unreal
ROOT=Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0,str(ROOT/'Pipeline/unreal'))
from import_asset import import_texture,texture_sample,quit_after_editor_startup
DEST='/Game/Wroclaw/StartLocation/Materials'


def prepare():
    edit=unreal.MaterialEditingLibrary;tools=unreal.AssetToolsHelpers.get_asset_tools()
    for name,color,rough in [('Leaf',(.10,.19,.035),.72),('Bark',(.16,.12,.08),.9),('Soil',(.06,.043,.025),.96)]:
        mat=unreal.load_asset(DEST+'/M_Start_'+name)
        if not mat:mat=tools.create_asset('M_Start_'+name,DEST,unreal.Material,unreal.MaterialFactoryNew())
        edit.delete_all_material_expressions(mat)
        if name!='Soil':
            stem='leaf' if name=='Leaf' else 'bark'
            textures={role:import_texture(ROOT/f'SourceAssets/Textures/Wroclaw/Foliage/{stem}_{role}.png',DEST+'/Textures',role) for role in ['base_color','normal']}
            base=texture_sample(edit,mat,textures['base_color'],-400,-200,'BaseColor')
            normal=texture_sample(edit,mat,textures['normal'],-400,0,'Normal')
            edit.connect_material_property(base,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
            edit.connect_material_property(normal,'RGB',unreal.MaterialProperty.MP_NORMAL)
        else:
            base=edit.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,-400,-200)
            base.set_editor_property('constant',unreal.LinearColor(*color,1));edit.connect_material_property(base,'',unreal.MaterialProperty.MP_BASE_COLOR)
        for prop,value in [(unreal.MaterialProperty.MP_ROUGHNESS,rough),(unreal.MaterialProperty.MP_METALLIC,0)]:
            node=edit.create_material_expression(mat,unreal.MaterialExpressionConstant,-200,180);node.set_editor_property('r',value);edit.connect_material_property(node,'',prop)
        if name=='Leaf':
            mat.set_editor_property('two_sided',True);mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED)
            mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
            mat.set_editor_property('opacity_mask_clip_value',.35)
            edit.connect_material_property(base,'A',unreal.MaterialProperty.MP_OPACITY_MASK)
            subsurface=edit.create_material_expression(mat,unreal.MaterialExpressionMultiply,-200,-300)
            subsurface.set_editor_property('const_b',.45);edit.connect_material_expressions(base,'RGB',subsurface,'A')
            edit.connect_material_property(subsurface,'',unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
        for flag in [unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,unreal.MaterialUsage.MATUSAGE_NANITE]:edit.set_material_usage(mat,flag)
        edit.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat,False)
        instance=unreal.load_asset(DEST+'/MI_'+name)
        if not instance:instance=tools.create_asset('MI_'+name,DEST,unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
        edit.set_material_instance_parent(instance,mat);edit.update_material_instance(instance);unreal.EditorAssetLibrary.save_loaded_asset(instance,False)
    marker=ROOT/'Saved/StartLocation/foliage.ok';marker.write_text('PASS\n',encoding='utf-8');unreal.log('WTG_FOLIAGE_MATERIALS_PASS')


try:prepare()
except Exception:unreal.log_error(traceback.format_exc());raise
finally:quit_after_editor_startup()
