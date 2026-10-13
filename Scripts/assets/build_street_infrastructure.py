"""Assemble the licensed bench; author a metric embedded tram rail profile."""
import json
import hashlib
from pathlib import Path
import bpy
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'SourceAssets/Models/Wroclaw/StartLocation'


def bench():
    source=ROOT/'SourceAssets/Models/PolyHaven/modular_street_seating/modular_street_seating_1k.fbx'
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(source))
    names={'seat','seat_back','crossbar','back_support_l','back_support_r','legs_double','legs_single',
           'arm_rest_01','arm_rest_02','suspended_support_01'}
    selected=[]
    for obj in list(bpy.data.objects):
        if obj.type!='MESH' or obj.name not in names:
            bpy.data.objects.remove(obj,do_unlink=True);continue
        if obj.name in ['legs_single','arm_rest_01']:obj.location.x+=.205
        if obj.name in ['legs_double','arm_rest_02']:obj.location.x-=.205
        selected.append(obj)
    assert {o.name for o in selected}==names
    bpy.ops.object.select_all(action='SELECT');bpy.context.view_layer.objects.active=next(o for o in selected if o.name=='seat')
    bpy.ops.object.join();obj=bpy.context.object;obj.name='street-bench'
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    # Explicit semantic slots, independent of source scene object order.
    roles=[str(obj.data.materials[p.material_index].name) for p in obj.data.polygons]
    mats=[bpy.data.materials[n] for n in ['modular_street_seating_timber','modular_street_seating_armrests','modular_street_seating_supports']]
    obj.data.materials.clear()
    for mat in mats:obj.data.materials.append(mat)
    for poly,role in zip(obj.data.polygons,roles):poly.material_index=next(i for i,m in enumerate(mats) if m.name==role)
    points=[Vector(p) for p in obj.bound_box];center=Vector(((min(p.x for p in points)+max(p.x for p in points))/2,
        (min(p.y for p in points)+max(p.y for p in points))/2,min(p.z for p in points)))
    for vertex in obj.data.vertices:vertex.co-=center
    obj.data.calc_loop_triangles()
    (OUT/'Props').mkdir(parents=True,exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Props/street-bench.blend'))
    return dict(asset='street-bench',source=source.relative_to(ROOT).as_posix(),sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
        author='Stuart Attenborrow',license='CC0-1.0',url='https://polyhaven.com/a/modular_street_seating',
        game_use=True,public_source_redistribution=True,parts=sorted(names),triangles=len(obj.data.loop_triangles),
        use='Complete assembled bench, not a height-scaled crossbar',generator='Scripts/assets/build_street_infrastructure.py')


def rail():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    # X: tangent, Y: cross-section. Top is Z=0 for flush placement at grade.
    profile=[(-.034,0),(.034,0),(.037,-.012),(.022,-.026),(.009,-.035),(.009,-.145),
             (.070,-.165),(.070,-.180),(-.070,-.180),(-.070,-.165),(-.009,-.145),
             (-.009,-.035),(-.022,-.026),(-.037,-.012)]
    vertices=[(x,y,z) for x in [-.5,.5] for y,z in profile];n=len(profile)
    faces=[tuple(reversed(range(n))),tuple(range(n,2*n))]+[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    mesh=bpy.data.meshes.new('EmbeddedRail');mesh.from_pydata(vertices,[],faces);mesh.update()
    obj=bpy.data.objects.new('embedded-tram-rail',mesh);bpy.context.collection.objects.link(obj)
    mat=bpy.data.materials.new('Steel');obj.data.materials.append(mat)
    obj.select_set(True);bpy.context.view_layer.objects.active=obj
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.mesh.normals_make_consistent(inside=False) if hasattr(bpy.ops.mesh,'normals_make_consistent') else None
    bpy.ops.uv.smart_project(island_margin=.02);bpy.ops.object.mode_set(mode='OBJECT')
    (OUT/'Roads').mkdir(exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Roads/embedded-tram-rail.blend'))
    return dict(asset='embedded-tram-rail',author='WroclawTheGame contributors',license='CC0-1.0',
        source='Original rail cross-section, not a surveyed infrastructure replica',game_use=True,public_source_redistribution=True,
        dimensions_m=[1,.14,.18],rail_head_width_cm=6.8,web_height_cm=18,
        generator='Scripts/assets/build_street_infrastructure.py')


if __name__=='__main__':
    metadata=dict(assets=[bench(),rail()])
    (OUT/'infrastructure.json').write_text(json.dumps(metadata,indent=2),encoding='utf-8')
    print('WTG_STREET_INFRASTRUCTURE_PASS')
