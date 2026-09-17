"""Unreal editor gate: packaging is refused if conversion did not produce a streaming world."""
from pathlib import Path
import unreal
ROOT=Path(unreal.Paths.project_dir()).resolve()
MARKER=ROOT/'Saved/PartitionVerified.ok'
MARKER.unlink(missing_ok=True)
try:
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.load_level('/Game/Maps/Przebudzenie_Source'):raise RuntimeError('Converted map missing')
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    # get_world_partition() is exposed by the native validation library to avoid private Python properties.
    if not unreal.WorldValidationLibrary.validate_partition(world):raise RuntimeError('World Partition / streaming validation failed')
    MARKER.write_text('World Partition enabled\n',encoding='utf-8')
finally:
    unreal.SystemLibrary.quit_editor()
