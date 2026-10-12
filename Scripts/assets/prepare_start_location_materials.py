"""Import reviewed shared PBR once, create stable interfaces before mesh import."""
import json
import sys
import traceback
from pathlib import Path
import unreal

ROOT=Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0,str(ROOT/'Pipeline/unreal'))
from import_asset import import_texture,create_material,texture_sample,quit_after_editor_startup
DEST='/Game/Wroclaw/StartLocation/Materials'
marker=ROOT/'Saved/StartLocation/materials.ok';marker.parent.mkdir(parents=True,exist_ok=True);marker.unlink(missing_ok=True)


def prepare():
    edit=unreal.MaterialEditingLibrary;tools=unreal.AssetToolsHelpers.get_asset_tools()
    registry=json.loads((ROOT/'Data/city_asset_acquisition.json').read_text(encoding='utf-8'))
    by_slug={a['slug']:a for a in registry['assets']}
    surfaces={'Plaster':'painted_plaster_wall','Stone':'sandstone_cracks','Roof':'roof_tiles_14',
              'Asphalt':'asphalt_03','Pavement':'concrete_pavement_03','Cobble':'cobblestone_floor_08','Grass':'grass_ground'}
    masters={}
    for name,slug in surfaces.items():
        textures={role:import_texture(ROOT/f['path'],DEST+'/Textures',role) for role,f in by_slug[slug]['files'].items()}
        material=create_material('Start_'+name,DEST,textures)
        # Tint is an instance parameter, shared texture memory remains unchanged.
        base=edit.create_material_expression(material,unreal.MaterialExpressionTextureSampleParameter2D,-600,-200)
        base.set_editor_property('texture',textures['base_color']);base.set_editor_property('parameter_name','BaseColor')
        tint=edit.create_material_expression(material,unreal.MaterialExpressionVectorParameter,-600,-400)
        tint.set_editor_property('parameter_name','Tint');tint.set_editor_property('default_value',unreal.LinearColor(1,1,1,1))
        multiply=edit.create_material_expression(material,unreal.MaterialExpressionMultiply,-300,-200)
        edit.connect_material_expressions(base,'RGB',multiply,'A');edit.connect_material_expressions(tint,'',multiply,'B')
        edit.connect_material_property(multiply,'',unreal.MaterialProperty.MP_BASE_COLOR)
        normal=texture_sample(edit,material,textures['normal'],-600,80,'Normal')
        orm=texture_sample(edit,material,textures['orm'],-600,300,'ORM')
        edit.connect_material_property(normal,'RGB',unreal.MaterialProperty.MP_NORMAL)
        for pin,prop in [('R',unreal.MaterialProperty.MP_AMBIENT_OCCLUSION),('G',unreal.MaterialProperty.MP_ROUGHNESS),('B',unreal.MaterialProperty.MP_METALLIC)]:edit.connect_material_property(orm,pin,prop)
        if name in ['Asphalt','Pavement','Cobble','Grass']:
            # GIS ground has arbitrary/degenerate UVs; use centimetre world XY.
            world=edit.create_material_expression(material,unreal.MaterialExpressionWorldPosition,-1100,0)
            divide=edit.create_material_expression(material,unreal.MaterialExpressionDivide,-900,0);divide.set_editor_property('const_b',float(by_slug[slug]['dimensions_mm'][0])/10)
            mask=edit.create_material_expression(material,unreal.MaterialExpressionComponentMask,-750,0)
            mask.set_editor_property('r',True);mask.set_editor_property('g',True)
            edit.connect_material_expressions(world,'',divide,'A');edit.connect_material_expressions(divide,'',mask,'Input')
            edit.connect_material_expressions(mask,'',base,'Coordinates')
            for node in (normal,orm):edit.connect_material_expressions(mask,'',node,'Coordinates')
        for usage in [unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,unreal.MaterialUsage.MATUSAGE_NANITE]:edit.set_material_usage(material,usage)
        edit.recompile_material(material);unreal.EditorAssetLibrary.save_loaded_asset(material,False);masters[name]=material
    constants={'Frame':((.75,.73,.67),.65,0),'Glazing':((.025,.055,.075),.18,.25),
               'Metal':((.07,.075,.08),.4,.85),'Wood':((.15,.065,.025),.76,0),
               'Leaf':((.055,.16,.022),.76,0),'Rubber':((.018,.018,.018),.9,0),
               'Paint':((.13,.2,.26),.25,.3),'Light':((.8,.8,.65),.25,0)}
    for name,(color,roughness,metallic) in constants.items():
        path=DEST+'/M_Start_'+name;mat=unreal.load_asset(path)
        if not mat:mat=tools.create_asset('M_Start_'+name,DEST,unreal.Material,unreal.MaterialFactoryNew())
        edit.delete_all_material_expressions(mat)
        c=edit.create_material_expression(mat,unreal.MaterialExpressionVectorParameter,0,0);c.set_editor_property('parameter_name','Tint');c.set_editor_property('default_value',unreal.LinearColor(*color,1))
        edit.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
        for prop,value in [(unreal.MaterialProperty.MP_ROUGHNESS,roughness),(unreal.MaterialProperty.MP_METALLIC,metallic)]:
            node=edit.create_material_expression(mat,unreal.MaterialExpressionConstant,0,120);node.set_editor_property('r',value);edit.connect_material_property(node,'',prop)
        if name=='Leaf':mat.set_editor_property('two_sided',True)
        for usage in [unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,unreal.MaterialUsage.MATUSAGE_NANITE]:edit.set_material_usage(mat,usage)
        edit.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat,False);masters[name]=mat
    for name,material in masters.items():
        path=DEST+'/MI_'+name;instance=unreal.load_asset(path)
        if not instance:instance=tools.create_asset('MI_'+name,DEST,unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
        edit.set_material_instance_parent(instance,material)
        for usage in [unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,unreal.MaterialUsage.MATUSAGE_NANITE]:edit.set_material_usage_override(instance,usage,True,True)
        edit.update_material_instance(instance)
        unreal.EditorAssetLibrary.save_loaded_asset(instance,False)
    for path in ['/Game/SurfaceQuality/Instances/MI_PaintedMetal','/Game/Wroclaw/Nadodrze/MI_DistrictWindowGlazing']:
        material=unreal.load_asset(path)
        if isinstance(material,unreal.MaterialInstanceConstant):
            edit.set_material_usage_override(material,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,True,True)
            edit.update_material_instance(material);unreal.EditorAssetLibrary.save_loaded_asset(material,False)
    marker.write_text('PASS\n',encoding='utf-8');unreal.log('WTG_START_MATERIALS_PASS '+str(len(masters)))


try:prepare()
except Exception:unreal.log_error(traceback.format_exc());raise
finally:quit_after_editor_startup()
