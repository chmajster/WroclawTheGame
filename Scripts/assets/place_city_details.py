"""Apply the generated source plan to existing map; replaces only CityKit_ actors."""
import hashlib
import json
import math
import sys
import traceback
from pathlib import Path
import unreal

ROOT=Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0,str(ROOT/"Pipeline/unreal"))
from import_asset import quit_after_editor_startup
from city_quality import configure_instances

marker=ROOT/"Saved/CityDetails/placement.ok"; marker.parent.mkdir(parents=True,exist_ok=True); marker.unlink(missing_ok=True)
try:
    plan=json.loads((ROOT/"Saved/CityDetails/placement.json").read_text(encoding="utf-8"))
    policy=json.loads((ROOT/"Data/city_detail_policy.json").read_text(encoding="utf-8"))
    for path,sha in plan["source_files"].items():
        if hashlib.sha256((ROOT/path).read_bytes()).hexdigest()!=sha: raise ValueError("Placement source changed: regenerate plan")
    meshes={role:unreal.load_asset(path) for role,path in policy["meshes"].items()}
    if any(not isinstance(mesh,unreal.StaticMesh) for mesh in meshes.values()): raise RuntimeError("Kit mesh missing: import production manifests first")
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.load_level(plan["map"]): raise RuntimeError("Target district map missing")
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in actors.get_all_level_actors():
        if actor.get_actor_label().startswith("CityKit_"): actors.destroy_actor(actor)
    clusters={}
    for item in plan["items"]:
        p=item["position_cm"]; cell=policy["cell_cm"]
        key=(math.floor(p[0]/cell),math.floor(p[1]/cell),item["role"])
        if key not in clusters:
            cluster=actors.spawn_actor_from_class(unreal.WTGFacadeInstanceCluster,unreal.Vector(key[0]*cell,key[1]*cell,0))
            if not cluster: raise RuntimeError("Cannot create HISM cluster")
            cluster.set_actor_label("CityKit_"+"_".join(str(value) for value in key))
            clusters[key]=cluster
        transform=unreal.Transform(location=unreal.Vector(*p),rotation=unreal.Rotator(pitch=0,yaw=item["yaw_deg"],roll=0),scale=unreal.Vector(*item["scale"]))
        if clusters[key].add_facade_instance(meshes[item["role"]],transform)<0: raise RuntimeError("Instance creation failed")
    for key,cluster in clusters.items():
        for component in cluster.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent):
            configure_instances(component)
            if key[2] in {"railing","slab"}: component.set_collision_profile_name("BlockAll")
            else: component.set_collision_profile_name("NoCollision")
    actual=sum(cluster.get_facade_instance_count() for cluster in clusters.values())
    if actual!=len(plan["items"]): raise RuntimeError("Instance count differs from plan")
    if not levels.save_current_level(): raise RuntimeError("Map save failed")
    report={"status":"PASS","map":plan["map"],"counts":plan["counts"],"instances":actual,"clusters":len(clusters),"buildings":plan["building_count"],"fidelity":plan["fidelity"]}
    (marker.parent/"placement-report.json").write_text(json.dumps(report,indent=2),encoding="utf-8")
    marker.write_text("PASS\n",encoding="utf-8"); unreal.log("WTG_CITY_KIT_PLACED "+json.dumps(report))
except Exception:
    unreal.log_error(traceback.format_exc()); raise
finally: quit_after_editor_startup()
