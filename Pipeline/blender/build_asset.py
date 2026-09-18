"""Headless Blender stage for WroclawTheGame asset manifests.

Run:
blender --background --python Pipeline/blender/build_asset.py -- --manifest <manifest>
"""
from __future__ import annotations

import argparse
import json
import math
import sys
import traceback
from pathlib import Path

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]


def script_args() -> list[str]:
    return sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []


def resolve(value: str) -> Path:
    path = Path(value)
    return path if path.is_absolute() else (ROOT / path).resolve()


def clean_scene() -> None:
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for datablocks in (bpy.data.meshes, bpy.data.curves, bpy.data.materials):
        for block in list(datablocks):
            if block.users == 0:
                datablocks.remove(block)


def import_source(path: Path) -> None:
    suffix = path.suffix.lower()
    if suffix == ".blend":
        bpy.ops.wm.open_mainfile(filepath=str(path))
        return
    clean_scene()
    if suffix == ".fbx":
        bpy.ops.import_scene.fbx(filepath=str(path))
    elif suffix in {".glb", ".gltf"}:
        bpy.ops.import_scene.gltf(filepath=str(path))
    elif suffix == ".obj":
        if hasattr(bpy.ops.wm, "obj_import"):
            bpy.ops.wm.obj_import(filepath=str(path))
        else:
            bpy.ops.import_scene.obj(filepath=str(path))
    else:
        raise RuntimeError(f"Unsupported source format: {suffix}")


def mesh_objects() -> list[bpy.types.Object]:
    return [
        obj for obj in bpy.context.scene.objects
        if obj.type == "MESH" and not obj.name.upper().startswith(("UCX_", "UBX_", "USP_", "UCP_"))
    ]


def select_only(objects: list[bpy.types.Object]) -> None:
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    if objects:
        bpy.context.view_layer.objects.active = objects[0]


def apply_transforms(objects: list[bpy.types.Object]) -> None:
    select_only(objects)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)


def join_copy(objects: list[bpy.types.Object], name: str) -> bpy.types.Object:
    copies = []
    for obj in objects:
        clone = obj.copy()
        clone.data = obj.data.copy()
        bpy.context.collection.objects.link(clone)
        copies.append(clone)
    select_only(copies)
    bpy.context.view_layer.objects.active = copies[0]
    if len(copies) > 1:
        bpy.ops.object.join()
    result = bpy.context.view_layer.objects.active
    result.name = name
    result.data.name = name
    return result


def bounds(objects: list[bpy.types.Object]) -> tuple[Vector, Vector]:
    points = [obj.matrix_world @ Vector(corner) for obj in objects for corner in obj.bound_box]
    if not points:
        raise RuntimeError("Source contains no mesh bounds")
    minimum = Vector((min(p.x for p in points), min(p.y for p in points), min(p.z for p in points)))
    maximum = Vector((max(p.x for p in points), max(p.y for p in points), max(p.z for p in points)))
    return minimum, maximum


def normalize(objects: list[bpy.types.Object], manifest: dict) -> None:
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0

    target_height = manifest.get("dimensions", {}).get("height_m")
    if target_height:
        minimum, maximum = bounds(objects)
        source_height = maximum.z - minimum.z
        if source_height <= 1e-8:
            raise RuntimeError("Cannot scale zero-height mesh")
        factor = float(target_height) / source_height
        for obj in objects:
            obj.location *= factor
            obj.scale *= factor
        apply_transforms(objects)

    minimum, maximum = bounds(objects)
    offset = Vector((-(minimum.x + maximum.x) / 2.0, -(minimum.y + maximum.y) / 2.0, -minimum.z))
    for obj in objects:
        obj.location += offset
    apply_transforms(objects)


def ensure_uv(obj: bpy.types.Object, generate: bool) -> None:
    if len(obj.data.uv_layers) > 0:
        return
    if not generate:
        raise RuntimeError(f"{obj.name} has no UV map")
    select_only([obj])
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(66.0), island_margin=0.02)
    bpy.ops.object.mode_set(mode="OBJECT")


def triangle_count(obj: bpy.types.Object) -> int:
    obj.data.calc_loop_triangles()
    return len(obj.data.loop_triangles)


def decimated_copy(source: bpy.types.Object, ratio: float, name: str) -> bpy.types.Object:
    obj = source.copy()
    obj.data = source.data.copy()
    bpy.context.collection.objects.link(obj)
    obj.name = name
    obj.data.name = name
    if ratio < 0.999999:
        select_only([obj])
        modifier = obj.modifiers.new(name="WTG_LOD_DECIMATE", type="DECIMATE")
        modifier.ratio = float(ratio)
        modifier.use_collapse_triangulate = True
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    return obj


def convex_collision(source: bpy.types.Object, mesh_name: str) -> bpy.types.Object:
    obj = source.copy()
    obj.data = source.data.copy()
    bpy.context.collection.objects.link(obj)
    obj.name = f"UCX_{mesh_name}_00"
    obj.data.name = obj.name
    obj.data.materials.clear()
    select_only([obj])
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.convex_hull()
    bpy.ops.object.mode_set(mode="OBJECT")
    return obj


def export_fbx(path: Path, objects: list[bpy.types.Object]) -> None:
    select_only(objects)
    bpy.ops.export_scene.fbx(
        filepath=str(path),
        use_selection=True,
        object_types={"MESH"},
        apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_UNITS",
        axis_forward="-Z",
        axis_up="Y",
        bake_anim=False,
        add_leaf_bones=False,
        use_mesh_modifiers=True,
        path_mode="AUTO",
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", required=True)
    args = parser.parse_args(script_args())

    manifest_path = resolve(args.manifest)
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    asset_id = manifest["id"]
    safe_name = "SM_" + "".join(ch if ch.isalnum() or ch == "_" else "_" for ch in asset_id)
    out = ROOT / "Saved" / "Pipeline" / "generated" / asset_id
    out.mkdir(parents=True, exist_ok=True)

    source = resolve(manifest["source"]["model"])
    if not source.is_file():
        raise RuntimeError(f"Missing source model: {source}")

    import_source(source)
    meshes = mesh_objects()
    if not meshes:
        raise RuntimeError("Source contains no mesh objects")

    normalize(meshes, manifest)
    base = join_copy(meshes, safe_name)
    ensure_uv(base, bool(manifest["mesh"]["generate_uv_if_missing"]))
    apply_transforms([base])

    lod0_triangles = triangle_count(base)
    max_triangles = int(manifest["mesh"]["max_triangles"])
    if lod0_triangles > max_triangles:
        raise RuntimeError(f"LOD0 triangle budget exceeded: {lod0_triangles} > {max_triangles}")

    ratios = [float(value) for value in manifest["lod"]["ratios"]]
    lod_objects = []
    lod_reports = []
    for index, ratio in enumerate(ratios):
        obj = decimated_copy(base, ratio, f"{safe_name}_LOD{index}")
        count = triangle_count(obj)
        lod_objects.append(obj)
        lod_reports.append({"lod": index, "ratio": ratio, "triangles": count})

    collision = None
    if manifest["collision"]["mode"] == "simple":
        collision = convex_collision(base, safe_name)

    for index, obj in enumerate(lod_objects):
        export_objects = [obj]
        if index == 0 and collision is not None:
            export_objects.append(collision)
        export_fbx(out / f"{safe_name}_LOD{index}.fbx", export_objects)

    report = {
        "asset_id": asset_id,
        "source": str(source),
        "mesh_name": safe_name,
        "lod0_triangles": lod0_triangles,
        "max_triangles": max_triangles,
        "lods": lod_reports,
        "collision": manifest["collision"]["mode"],
        "exports": [f"{safe_name}_LOD{i}.fbx" for i in range(len(lod_objects))],
        "status": "PASS",
    }
    (out / "blender-report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    print("WTG_BLENDER_READY " + json.dumps(report))


if __name__ == "__main__":
    try:
        main()
    except Exception:
        traceback.print_exc()
        raise
