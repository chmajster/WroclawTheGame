"""Authored relaxed in-place idle on the actual civilian bind rig (CC0)."""
import hashlib
import json
import math
from pathlib import Path

import bpy
from mathutils import Quaternion, Vector

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'SourceAssets/Characters/WroclawHero/Animations'


def build():
    OUT.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.open_mainfile(filepath=str(ROOT / 'SourceAssets/Characters/WroclawHero/male/hero.blend'))
    rig = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
    rig.animation_data_clear()
    scene = bpy.context.scene
    scene.render.fps = 30
    scene.frame_start, scene.frame_end = 1, 181
    for bone in rig.pose.bones:
        bone.rotation_mode = 'QUATERNION'
        bone.rotation_quaternion = Quaternion()
        bone.location = Vector()

    def global_delta(name, axis, degrees):
        bone = rig.pose.bones.get(name)
        if not bone:
            return
        rest = bone.bone.matrix_local.to_quaternion()
        bone.rotation_quaternion = rest.inverted() @ Quaternion(axis, math.radians(degrees)) @ rest

    for frame in range(1, 182):
        phase = math.tau * (frame - 1) / 180
        for side, sign in [('l', 1), ('r', -1)]:
            global_delta('upperarm_' + side, (0, 1, 0), sign * (76 + math.sin(phase) * .35))
            global_delta('lowerarm_' + side, (0, 0, 1), sign * 7)
            global_delta('hand_' + side, (0, 0, 1), sign * -4)
            # Open, relaxed hands rather than the original combat fists.
            for finger in ['index', 'middle', 'ring', 'pinky']:
                for part, angle in [(1, 8), (2, 12), (3, 8)]:
                    bone = rig.pose.bones.get(f'{finger}_{part:02d}_{side}')
                    if bone:
                        bone.rotation_quaternion = Quaternion((1, 0, 0), math.radians(angle * .15))
        global_delta('spine_02', (1, 0, 0), math.sin(phase) * .45)
        global_delta('spine_03', (0, 1, 0), math.sin(phase) * .35)
        global_delta('head', (0, 0, 1), math.sin(phase) * .7)
        # A complete loop, no translation or scale of the root.
        for bone in rig.pose.bones:
            bone.keyframe_insert('rotation_quaternion', frame=frame)
    scene.frame_set(1)
    blend = OUT / 'civilian-idle.blend'
    bpy.ops.wm.save_as_mainfile(filepath=str(blend))
    bpy.ops.object.select_all(action='DESELECT')
    rig.select_set(True)
    bpy.context.view_layer.objects.active = rig
    fbx = OUT / 'civilian-idle.fbx'
    bpy.ops.export_scene.fbx(filepath=str(fbx), use_selection=True, object_types={'ARMATURE'},
        add_leaf_bones=False, axis_forward='-Y', axis_up='Z', bake_anim=True,
        bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
        bake_anim_step=1, bake_anim_simplify_factor=0)
    metadata = dict(name='A_CivilianIdle', source=str(blend.relative_to(ROOT)).replace('\\', '/'),
        fbx=str(fbx.relative_to(ROOT)).replace('\\', '/'), sha256=hashlib.sha256(fbx.read_bytes()).hexdigest(),
        author='WroclawTheGame contributors', license='CC0-1.0', fps=30, frames=181, duration_seconds=6,
        root_motion=False, skeleton='SK_CivilianSkeleton')
    (OUT / 'animations.json').write_text(json.dumps(metadata, indent=2), encoding='utf-8')
    print('WTG_CIVILIAN_IDLE_PASS', metadata['frames'])


if __name__ == '__main__':
    build()
