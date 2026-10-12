"""Persistent native mesh policy shared by import and standalone QA."""
import json
from pathlib import Path
import unreal

ROOT=Path(unreal.Paths.project_dir()).resolve()


def configure_mesh(mesh,manifest):
    if manifest.get("area")!="tenements": return
    policy=json.loads((ROOT/"Data/city_asset_quality.json").read_text(encoding="utf-8"))
    editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    nanite=editor.get_nanite_settings(mesh)
    if nanite.get_editor_property("enabled"):
        nanite.set_editor_property("fallback_target",unreal.NaniteFallbackTarget.PERCENT_TRIANGLES)
        nanite.set_editor_property("fallback_percent_triangles",policy["nanite_fallback_percent_triangles"])
        nanite.set_editor_property("fallback_relative_error",0.)
        editor.set_nanite_settings(mesh,nanite,True)
    if not editor.set_lod_screen_sizes(mesh,policy["lod_screen_sizes"]): raise RuntimeError("Cannot set LOD screen sizes")
    usage=unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES
    for slot in mesh.get_editor_property("static_materials"):
        material=slot.get_editor_property("material_interface")
        if material is None: raise RuntimeError("Unbound material")
        if isinstance(material,unreal.Material):
            unreal.MaterialEditingLibrary.set_material_usage(material,usage)
            if nanite.get_editor_property("enabled"):
                unreal.MaterialEditingLibrary.set_material_usage(material,unreal.MaterialUsage.MATUSAGE_NANITE)
            unreal.MaterialEditingLibrary.recompile_material(material)
        elif isinstance(material,unreal.MaterialInstanceConstant):
            # Persist usage on this instance without changing its shared master.
            unreal.MaterialEditingLibrary.set_material_usage_override(material,usage,True,True)
            unreal.MaterialEditingLibrary.update_material_instance(material)
        if not unreal.MaterialEditingLibrary.has_material_usage(material,usage): raise RuntimeError("HISM usage not retained")
        if not unreal.EditorAssetLibrary.save_loaded_asset(material,False): raise RuntimeError("Material save failed")
    if not unreal.EditorAssetLibrary.save_loaded_asset(mesh,False): raise RuntimeError("Mesh quality save failed")


def configure_instances(component):
    policy=json.loads((ROOT/"Data/city_asset_quality.json").read_text(encoding="utf-8"))
    cull=policy["instance_cull_cm"]
    component.set_cull_distances(cull["start"],cull["end"])
