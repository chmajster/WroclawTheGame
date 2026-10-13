"""Persistent native mesh policy shared by import and standalone QA."""
import json
from pathlib import Path
import unreal

ROOT=Path(unreal.Paths.project_dir()).resolve()


def configure_mesh(mesh,manifest):
    area=manifest.get("area")
    if area not in ["tenements","start_location"]: return
    if area=="start_location":
        policy=json.loads((ROOT/"Data/start_location.json").read_text(encoding="utf-8"))["quality"]
        category="building" if manifest['unreal']['destination'].endswith('/Buildings') else ("vegetation" if manifest["display_name"]=="young-tree" else "prop")
        screen_sizes=policy["lod_screen_sizes"][category]
    else:
        policy=json.loads((ROOT/"Data/city_asset_quality.json").read_text(encoding="utf-8"))
        screen_sizes=policy["lod_screen_sizes"]
    editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    nanite=editor.get_nanite_settings(mesh)
    if nanite.get_editor_property("enabled"):
        if area=="start_location" and category=="building":
            nanite.set_editor_property("explicit_tangents",True)
        nanite.set_editor_property("fallback_target",unreal.NaniteFallbackTarget.PERCENT_TRIANGLES)
        nanite.set_editor_property("fallback_percent_triangles",policy["nanite_fallback_percent_triangles"])
        nanite.set_editor_property("fallback_relative_error",0.)
        editor.set_nanite_settings(mesh,nanite,True)
    if not editor.set_lod_screen_sizes(mesh,screen_sizes): raise RuntimeError("Cannot set LOD screen sizes")
    usage=unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES
    for slot in mesh.get_editor_property("static_materials"):
        material=slot.get_editor_property("material_interface")
        if material is None: raise RuntimeError("Unbound material")
        required=[usage]+([unreal.MaterialUsage.MATUSAGE_NANITE] if nanite.get_editor_property("enabled") else [])
        missing=[flag for flag in required if not unreal.MaterialEditingLibrary.has_material_usage(material,flag)]
        if missing and isinstance(material,unreal.Material):
            for flag in missing:unreal.MaterialEditingLibrary.set_material_usage(material,flag)
            unreal.MaterialEditingLibrary.recompile_material(material)
        elif missing and isinstance(material,unreal.MaterialInstanceConstant):
            for flag in missing:unreal.MaterialEditingLibrary.set_material_usage_override(material,flag,True,True)
            unreal.MaterialEditingLibrary.update_material_instance(material)
        if not unreal.MaterialEditingLibrary.has_material_usage(material,usage): raise RuntimeError("HISM usage not retained")
        if any(not unreal.MaterialEditingLibrary.has_material_usage(material,flag) for flag in required):raise RuntimeError("Material usage not retained")
        # Shared materials recur across many meshes. Re-saving an unchanged
        # package repeatedly can race UE's async package reads (Windows code 32).
        if missing and not unreal.EditorAssetLibrary.save_loaded_asset(material,False): raise RuntimeError("Material save failed")
    if not unreal.EditorAssetLibrary.save_loaded_asset(mesh,False): raise RuntimeError("Mesh quality save failed")


def configure_instances(component,policy=None):
    if policy is None:
        policy=json.loads((ROOT/"Data/city_asset_quality.json").read_text(encoding="utf-8"))
        cull=policy["instance_cull_cm"]
    else:cull={"start":policy["instance_cull_start_cm"],"end":policy["instance_cull_end_cm"]}
    component.set_cull_distances(cull["start"],cull["end"])
