"""Create IK Rig/Retargeter assets and retarget all cross-rig animation bindings.

Runs inside UnrealEditor-Cmd after free models and animations have been imported.
A success marker is written only when auto characterization, FBIK, chain mapping and
every binding marked requires_retarget produce target-compatible animation assets.
"""
from __future__ import annotations

import json
import re
import traceback
from pathlib import Path

import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
MODEL_MAP = ROOT / "Saved/FreeModelImportMap.json"
ANIM_MAP = ROOT / "Saved/FreeAnimationImportMap.json"
BINDINGS = ROOT / "Data/animation_bindings.json"
POLICY = ROOT / "Data/runtime_asset_qa.json"
OUT = ROOT / "Saved/RuntimeAssetQA/retarget.json"
RETARGET_MAP = ROOT / "Saved/RetargetedAnimationMap.json"
MARKER = ROOT / "Saved/AnimationRetargetReady.ok"
DEST = "/Game/RuntimeAssetQA/Retarget"
OUTPUT_DEST = "/Game/FreeAnimations/Retargeted/Quaternius"


def clean(value):
    return re.sub(r"[^A-Za-z0-9_]+", "_", str(value)).strip("_") or "Unknown"


def norm(value):
    return re.sub(r"[^a-z0-9]+", "", str(value).lower())


def recreate_asset(name, asset_class, factory):
    path = f"{DEST}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        if not unreal.EditorAssetLibrary.delete_asset(path):
            raise RuntimeError(f"Cannot replace QA asset: {path}")
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, DEST, asset_class, factory)
    if not asset:
        raise RuntimeError(f"Cannot create asset: {path}")
    return asset


def make_ik_rig(name, mesh, require_fbik, required_chains):
    rig = recreate_asset(name, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    controller = unreal.IKRigController.get_controller(rig)
    if not controller or not controller.set_skeletal_mesh(mesh):
        raise RuntimeError(f"Cannot assign skeletal mesh to {name}")
    if not controller.apply_auto_generated_retarget_definition():
        raise RuntimeError(f"Auto retarget-chain generation failed: {name}")
    if require_fbik and not controller.apply_auto_fbik():
        raise RuntimeError(f"Auto FBIK generation failed: {name}")

    chain_names = []
    for chain in controller.get_retarget_chains():
        try:
            chain_name = chain.get_editor_property("chain_name")
        except Exception:
            chain_name = getattr(chain, "chain_name", "")
        chain_names.append(str(chain_name))
    normalized = {norm(value) for value in chain_names}
    missing = [name for name in required_chains if norm(name) not in normalized]
    if missing:
        raise RuntimeError(f"{name}: missing auto-generated retarget chains {missing}; got {chain_names}")
    if not str(controller.get_retarget_root()):
        raise RuntimeError(f"{name}: retarget root is empty")
    if not unreal.EditorAssetLibrary.save_loaded_asset(rig, False):
        raise RuntimeError(f"Cannot save IK Rig {name}")
    return rig, controller, chain_names


def make_retargeter(name, source_rig, target_rig, source_mesh, target_mesh):
    rtg = recreate_asset(name, unreal.IKRetargeter, unreal.IKRetargetFactory())
    controller = unreal.IKRetargeterController.get_controller(rtg)
    controller.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, source_rig)
    controller.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, target_rig)
    controller.set_preview_mesh(unreal.RetargetSourceOrTarget.SOURCE, source_mesh)
    controller.set_preview_mesh(unreal.RetargetSourceOrTarget.TARGET, target_mesh)

    controller.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
    controller.auto_align_all_bones(unreal.RetargetSourceOrTarget.TARGET)
    settings = controller.get_global_settings()
    settings.set_editor_property("enable_ik", True)
    controller.set_global_settings(settings)

    chain_settings = list(controller.get_all_chain_settings())
    mapped = []
    unmapped = []
    for value in chain_settings:
        source = str(getattr(value, "source_chain", ""))
        target = str(getattr(value, "target_chain", ""))
        if source and source.lower() != "none":
            mapped.append({"source": source, "target": target})
        else:
            unmapped.append(target)
    if not mapped:
        raise RuntimeError(f"{name}: IK Retargeter produced no mapped chains")
    if not unreal.EditorAssetLibrary.save_loaded_asset(rtg, False):
        raise RuntimeError(f"Cannot save retargeter {name}")
    return rtg, mapped, unmapped


def first_skeletal_mesh(paths, label):
    for path in paths:
        asset = unreal.load_asset(path)
        if isinstance(asset, unreal.SkeletalMesh):
            return asset
    raise RuntimeError(f"No SkeletalMesh imported for {label}")


def animation_asset_for_clip(paths, clip):
    wanted = norm(clip)
    candidates = []
    for path in paths:
        asset = unreal.load_asset(path)
        if not isinstance(asset, unreal.AnimationAsset):
            continue
        name = path.rsplit("/", 1)[-1].split(".")[-1]
        normalized = norm(name)
        if normalized == wanted:
            return path, asset
        if wanted in normalized:
            candidates.append((len(normalized), path, asset))
    if not candidates:
        raise RuntimeError(f"Cannot resolve imported animation clip {clip}")
    candidates.sort(key=lambda item: item[0])
    return candidates[0][1], candidates[0][2]


def retarget_one(source_path, source_mesh, target_mesh, retargeter, library_id, clip):
    asset_subsystem = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    package_path = source_path.split(".")[0]
    asset_data = asset_subsystem.find_asset_data(package_path)
    if not asset_data or not asset_data.is_valid():
        raise RuntimeError(f"Cannot resolve AssetData for {source_path}")

    destination = f"{OUTPUT_DEST}/{clean(library_id)}"
    inputs = unreal.IKRetargetBatchOperationInputs()
    inputs.set_editor_property("assets_to_retarget", [asset_data])
    inputs.set_editor_property("source_mesh", source_mesh)
    inputs.set_editor_property("target_mesh", target_mesh)
    inputs.set_editor_property("ik_retarget_asset", retargeter)
    inputs.set_editor_property("override_set_names", [])
    inputs.set_editor_property("search", "")
    inputs.set_editor_property("replace", "")
    inputs.set_editor_property("prefix", "RTG_")
    inputs.set_editor_property("suffix", "")
    inputs.set_editor_property("target_path", destination)
    inputs.set_editor_property("use_source_path", False)
    inputs.set_editor_property("include_referenced_assets", False)
    inputs.set_editor_property("overwrite_existing_files", True)
    inputs.set_editor_property("retain_additive_flags", True)

    output = list(unreal.IKRetargetBatchOperation.run_batch_retarget(inputs))
    if not output:
        raise RuntimeError(f"Batch retarget produced no asset: {library_id}:{clip}")
    asset = output[0].get_asset()
    if not isinstance(asset, unreal.AnimationAsset):
        raise RuntimeError(f"Retarget output is not AnimationAsset: {library_id}:{clip}")
    return asset.get_path_name()


def run():
    MARKER.unlink(missing_ok=True)
    OUT.unlink(missing_ok=True)
    RETARGET_MAP.unlink(missing_ok=True)

    model_map = json.loads(MODEL_MAP.read_text(encoding="utf-8"))
    anim_map = json.loads(ANIM_MAP.read_text(encoding="utf-8"))
    bindings = json.loads(BINDINGS.read_text(encoding="utf-8"))["bindings"]
    policy = json.loads(POLICY.read_text(encoding="utf-8"))["character"]
    required_chains = list(policy["required_ik_chains"])
    require_fbik = bool(policy["require_fbik"])

    target_meta = model_map.get("quaternius-ubc-superhero-male")
    if not target_meta:
        raise RuntimeError("Quaternius target character is not imported")
    target_mesh = unreal.load_asset(target_meta.get("primary_object"))
    if not isinstance(target_mesh, unreal.SkeletalMesh):
        raise RuntimeError("Quaternius target character is not a SkeletalMesh")

    target_rig, _, target_chains = make_ik_rig(
        "IKR_Quaternius_UBC", target_mesh, require_fbik, required_chains
    )

    requested = {
        key: value
        for key, value in bindings.items()
        if bool(value.get("requires_retarget"))
    }
    if bool(policy["require_retarget_for_flagged_bindings"]) and not requested:
        raise RuntimeError("No bindings are marked requires_retarget")

    libraries = sorted({value["library"] for value in requested.values()})
    rigs = {}
    retargeters = {}
    report_libraries = {}
    for library_id in libraries:
        metadata = anim_map.get(library_id)
        if not metadata:
            raise RuntimeError(f"Animation library not imported: {library_id}")
        source_mesh = first_skeletal_mesh(metadata.get("meshes", []), library_id)
        source_rig, source_controller, source_chains = make_ik_rig(
            "IKR_" + clean(library_id), source_mesh, require_fbik, required_chains
        )
        if not source_controller.is_skeletal_mesh_compatible(source_mesh):
            raise RuntimeError(f"Source IK Rig rejects its own mesh: {library_id}")
        rtg, mapped, unmapped = make_retargeter(
            "RTG_" + clean(library_id) + "_To_Quaternius",
            source_rig,
            target_rig,
            source_mesh,
            target_mesh,
        )
        rigs[library_id] = (source_mesh, source_rig)
        retargeters[library_id] = rtg
        report_libraries[library_id] = {
            "source_mesh": source_mesh.get_path_name(),
            "source_rig": source_rig.get_path_name(),
            "source_chains": source_chains,
            "target_chains": target_chains,
            "retargeter": rtg.get_path_name(),
            "mapped_chains": mapped,
            "unmapped_chains": unmapped,
        }

    cache = {}
    semantic_map = {}
    errors = []
    for semantic, binding in sorted(requested.items()):
        library_id = binding["library"]
        clip = binding["clip"]
        key = (library_id, clip)
        metadata = anim_map[library_id]
        source_path, source_asset = animation_asset_for_clip(metadata.get("animations", []), clip)
        source_mesh, _ = rigs[library_id]
        if key not in cache:
            try:
                cache[key] = retarget_one(
                    source_path,
                    source_mesh,
                    target_mesh,
                    retargeters[library_id],
                    library_id,
                    clip,
                )
            except Exception as exc:
                errors.append(f"{semantic}: {exc}")
                continue
        semantic_map[semantic] = {
            "library": library_id,
            "clip": clip,
            "source": source_asset.get_path_name(),
            "retargeted": cache[key],
        }

    missing_semantics = sorted(set(requested) - set(semantic_map))
    if missing_semantics:
        errors.append("missing retargeted semantic bindings: " + ", ".join(missing_semantics))

    for semantic, record in semantic_map.items():
        asset = unreal.load_asset(record["retargeted"])
        if not isinstance(asset, unreal.AnimationAsset):
            errors.append(f"{semantic}: retargeted object cannot be loaded as AnimationAsset")
            continue
        skeleton = asset.get_editor_property("skeleton")
        if skeleton != target_mesh.get_editor_property("skeleton"):
            errors.append(f"{semantic}: retargeted animation skeleton differs from target skeleton")

    report = {
        "status": "PASS" if not errors else "FAIL",
        "target_mesh": target_mesh.get_path_name(),
        "target_rig": target_rig.get_path_name(),
        "libraries": report_libraries,
        "retargeted_bindings": semantic_map,
        "errors": errors,
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    RETARGET_MAP.write_text(json.dumps(semantic_map, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    if errors:
        raise RuntimeError("Animation retarget QA failed:\n" + "\n".join(errors))
    MARKER.write_text(f"PASS {len(semantic_map)} retargeted bindings\n", encoding="utf-8")
    unreal.log(f"WTG_ANIMATION_RETARGET_PASS {len(semantic_map)}")


try:
    run()
except Exception:
    unreal.log_error(traceback.format_exc())
    MARKER.unlink(missing_ok=True)
finally:
    unreal.SystemLibrary.quit_editor()
