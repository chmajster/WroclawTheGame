"""Update gameplay placement from generated source without rebaking geometry."""
import json
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir())
marker=root/'Saved/DistrictPlacementReady.ok'
marker.unlink(missing_ok=True)
try:
    plan=json.loads((root/'Saved/PlayableDistrict/district.json').read_text(encoding='utf-8'))
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    levels.load_level('/Game/Maps/Nadodrze_Playable')
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    registry=[a for a in actors if isinstance(a,unreal.WTGDistrictRegistry)]
    if len(registry)!=1:
        raise RuntimeError('Expected one district registry')
    r=registry[0]
    r.set_editor_property('vehicle_yaw',plan['vehicle_yaw'])
    r.set_editor_property('delivery_route',[unreal.Vector(*p) for p in plan['delivery_route']])
    car=r.get_editor_property('delivery_vehicle')
    car.set_actor_rotation(unreal.Rotator(pitch=0,yaw=plan['vehicle_yaw'],roll=0),True)
    for a in actors:
        if isinstance(a,unreal.PlayerStart):
            a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=plan['vehicle_yaw'],roll=0),True)
    if not levels.save_current_level():
        raise RuntimeError('Map save failed')
    marker.write_text(str(plan['vehicle_yaw']),encoding='utf-8')
    unreal.log('WTG_DISTRICT_PLACEMENT_READY yaw='+str(plan['vehicle_yaw']))
finally:
    unreal.SystemLibrary.quit_editor()
