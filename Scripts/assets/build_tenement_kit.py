"""Author original metre-scale facade details with Blender; no downloaded geometry.

blender --background --python Scripts/assets/build_tenement_kit.py -- [--render]
Run make_kit_textures.py first. Architectural enrichment, not a surveyed facade.
"""
from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "SourceAssets/Models/Wroclaw/TenementKit"
TEXTURES = OUT / "Textures"


def select(objects):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]


def box(name, center, size, bevel=0.008):
    bpy.ops.mesh.primitive_cube_add(size=1, location=center)
    obj = bpy.context.object
    obj.name = name
    obj.scale = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel:
        mod = obj.modifiers.new("Cast edge radius", "BEVEL")
        mod.width = bevel
        mod.segments = 3
        bpy.ops.object.modifier_apply(modifier=mod.name)
    return obj


def rod(name, points, radius=0.01, sides=4):
    curve = bpy.data.curves.new(name, "CURVE")
    curve.dimensions = "3D"
    curve.bevel_depth = radius
    curve.bevel_resolution = 1
    curve.resolution_u = 1
    curve.use_fill_caps = True
    spline = curve.splines.new("POLY")
    spline.points.add(len(points)-1)
    for point, co in zip(spline.points, points):
        point.co = (*co, 1)
    obj = bpy.data.objects.new(name, curve)
    bpy.context.collection.objects.link(obj)
    select([obj])
    bpy.ops.object.convert(target="MESH")
    return obj


def railing():
    parts = []
    # Local X follows facade, Y is outward. Open back attaches to the building.
    segments = [((-1.2, 0.5), (1.2, 0.5)),
                ((-1.2, -0.5), (-1.2, 0.5)), ((1.2, -0.5), (1.2, 0.5))]
    for a, b in segments:
        dx, dy = b[0]-a[0], b[1]-a[1]
        length = math.hypot(dx, dy)
        for z, radius in [(0.12, 0.018), (0.54, 0.009), (0.93, 0.017), (1.08, 0.031)]:
            parts.append(rod("Continuous handrail", [(a[0], a[1], z), (b[0], b[1], z)], radius))
        bays = round(length/0.3)
        for i in range(bays+1):
            t = i/bays
            x, y = a[0]+dx*t, a[1]+dy*t
            parts.append(box("Square baluster", (x,y,0.525), (.02,.02,.81), .003))
            parts.append(box("Baluster collar", (x,y,.2), (.046,.046,.046), .005))
        for i in range(bays):
            center = (i+0.5)/bays
            # Mirrored forged scrolls: actual curved geometry, not solid panels.
            for mirror in [-1, 1]:
                points = []
                for n in range(37):
                    t = n/36
                    angle = t*math.pi*2.0
                    radius = .10*(1-.74*t)
                    u = mirror*(.026+radius*math.cos(angle))
                    z = .54 + .22*math.sin(angle)*(1-.65*t)
                    points.append((a[0]+dx*center+dx/length*u,
                                   a[1]+dy*center+dy/length*u, z))
                parts.append(rod("Forged volute", points, .008))
            parts.append(rod("Lower arch", [(a[0]+dx*center+dx/length*.115*math.cos(t*math.pi/24),
                a[1]+dy*center+dy/length*.115*math.cos(t*math.pi/24),
                .27+.095*math.sin(t*math.pi/24)) for t in range(25)], .008))
            for sign in [-1,1]:
                parts.append(rod("Arch weld tie",[(a[0]+dx*center+dx/length*sign*u,
                    a[1]+dy*center+dy/length*sign*u,.27) for u in [.115,length/bays/2]],.008))
    for x in [-1.2,1.2]:
        for y in [-.5,.5]:
            parts.append(box("Anchor post",(x,y,.53),(.055,.055,1.06),.007))
            parts.append(box("Post cap",(x,y,1.08),(.083,.083,.05),.01))
            parts.append(box("Base flange",(x,y,.035),(.1,.1,.07),.005))
    return parts, "iron"


def arch_block(name, a, b, inner, outer, spring, depth):
    verts = []
    for y in [-depth/2,depth/2]:
        for radius, angle in [(inner,a),(inner,b),(outer,b),(outer,a)]:
            verts.append((radius*math.cos(angle), y, spring+radius*math.sin(angle)))
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts,[],[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)])
    mesh.update()
    obj=bpy.data.objects.new(name,mesh)
    bpy.context.collection.objects.link(obj)
    select([obj])
    mod=obj.modifiers.new("Cut stone arris","BEVEL")
    mod.width=.006; mod.segments=2
    bpy.ops.object.modifier_apply(modifier=mod.name)
    return obj


def portal():
    parts=[]
    # 1.80 m clear width, 2.30 m spring + .90 m semicircular opening.
    for x in [-1.06,1.06]:
        for level in range(8):
            parts.append(box("Jamb ashlar",(x,0,.15+level*.287),(.32,.28,.277),.007))
        parts.append(box("Plinth",(x,.015,.12),(.46,.38,.24),.008))
        parts.append(box("Impost",(x,.015,2.29),(.45,.39,.13),.008))
        parts.append(box("Pilaster flute",(x,.162,1.3),(.08,.018,1.72),.005))
    for i in range(15):
        gap=.007
        parts.append(arch_block("Radial voussoir", i*math.pi/15+gap,(i+1)*math.pi/15-gap,.9,1.23,2.3,.28))
    parts.append(box("Keystone",(0,.04,3.48),(.20,.4,.31),.008))
    for width,depth,z,height in [(2.62,.36,3.66,.09),(2.72,.43,3.76,.10),(2.80,.48,3.85,.08)]:
        parts.append(box("Entablature moulding",(0,.03,z),(width,depth,height),.009))
    return parts,"stone"


def cornice():
    parts=[]
    for depth,z,height in [(.14,.05,.10),(.24,.14,.08),(.32,.22,.08),(.43,.31,.1),(.49,.40,.08)]:
        parts.append(box("Moulding course",(0,depth/2,z),(3,depth,height),.008))
    for i in range(15):
        parts.append(box("Dentil",(-1.4+i*.2,.14,.1),(.09,.20,.12),.005))
    for i in range(5):
        x=-1.2+i*.6
        parts.append(box("Modillion",(x,.18,.24),(.15,.3,.15),.018))
    return parts,"stone"


def balcony_slab():
    parts=[box("Load-bearing slab",(0,0,.52),(2.6,1.16,.16),.014),
           box("Drip moulding",(0,0,.43),(2.7,1.2,.10),.012)]
    for x in [-.8,.8]:
        # Stepped corbels transfer the cantilever towards its facade attachment.
        for i in range(5):
            depth=.32+i*.11
            parts.append(box("Stone corbel course",(x,-.56+depth/2,.045+i*.085),(.22,depth,.09),.014))
    return parts,"stone"


def paneled_door():
    parts=[]
    materials={}
    for kind,color,metal,rough in [("wood",(.18,.085,.025,1),0,.6),("metal",(.35,.29,.12,1),1,.27),("glass",(.09,.17,.22,1),0,.13)]:
        mat=bpy.data.materials.new("Door_"+kind); mat.use_nodes=True
        shader=mat.node_tree.nodes.get("Principled BSDF")
        shader.inputs["Base Color"].default_value=color
        shader.inputs["Metallic"].default_value=metal; shader.inputs["Roughness"].default_value=rough
        materials[kind]=mat
    def part(obj,kind): obj.data.materials.append(materials[kind]); parts.append(obj)
    for x in [-.45,.45]:
        part(box("Timber leaf",(x,0,1.15),(.894,.10,2.3),.006),"wood")
        for z,height in [(.42,.55),(1.24,.83),(2.0,.35)]:
            part(box("Raised recessed-panel border",(x,.057,z),(.72,.04,height),.009),"wood")
            part(box("Raised field",(x,.079,z),(.61,.022,height-.11),.006),"wood")
        part(box("Kick plate",(x,.058,.105),(.72,.018,.13),.003),"metal")
        hx=-.09 if x<0 else .09
        part(box("Handle backplate",(hx,.075,1.13),(.065,.018,.22),.006),"metal")
        part(rod("Pull handle",[(hx,.09,.99),(hx,.16,1.02),(hx,.16,1.22),(hx,.09,1.25)],.012),"metal")
    for x in [-.85,0,.85]:
        part(box("Mullion",(x,.07,2.0),(.042,.05,.48),.004),"wood")
    for x in [-.44,.44]:
        part(box("Glazed upper panel",(x,.092,2.0),(.68,.008,.26),.001),"glass")
    return parts,"door"


def material(kind):
    mat=bpy.data.materials.new("Authored_"+kind)
    mat.use_nodes=True
    nodes=mat.node_tree.nodes; links=mat.node_tree.links
    shader=nodes.get("Principled BSDF")
    for suffix, target, output in [("BC","Base Color","Color"),("ORM","Roughness","G")]:
        tex=nodes.new("ShaderNodeTexImage")
        tex.image=bpy.data.images.load(str(TEXTURES/f"{kind}_{suffix}.png"))
        if suffix=="ORM":
            tex.image.colorspace_settings.name="Non-Color"
            split=nodes.new("ShaderNodeSeparateColor")
            links.new(tex.outputs["Color"],split.inputs["Color"])
            links.new(split.outputs["Green"],shader.inputs["Roughness"])
            links.new(split.outputs["Blue"],shader.inputs["Metallic"])
        else: links.new(tex.outputs[output],shader.inputs[target])
    return mat


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--render",action="store_true")
    parser.add_argument("--asset",choices=["ornate-balcony-railing","stone-entrance-surround","dentil-cornice","balcony-slab","paneled-entrance-door"])
    args=parser.parse_args(sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else [])
    OUT.mkdir(parents=True,exist_ok=True)
    metadata=OUT/"geometry.json"
    report=json.loads(metadata.read_text(encoding="utf-8")) if args.asset and metadata.exists() else {"units":"metres","generator":"original bpy geometry","assets":[]}
    builders=[("ornate-balcony-railing",railing),("stone-entrance-surround",portal),("dentil-cornice",cornice),("balcony-slab",balcony_slab),("paneled-entrance-door",paneled_door)]
    for name, builder in builders:
        if args.asset and name!=args.asset: continue
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.context.preferences.filepaths.save_version=0
        bpy.context.scene.unit_settings.system="METRIC"
        parts,kind=builder()
        select(parts); bpy.ops.object.join()
        obj=bpy.context.object; obj.name=name
        if kind!="door":
            obj.data.materials.clear(); obj.data.materials.append(material(kind))
            for polygon in obj.data.polygons: polygon.material_index=0
        bpy.ops.object.mode_set(mode="EDIT"); bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.mesh.normals_make_consistent(inside=False)
        bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.01)
        bpy.ops.object.mode_set(mode="OBJECT")
        obj.data.calc_loop_triangles()
        report["assets"]=[asset for asset in report["assets"] if asset["id"]!=name]
        report["assets"].append({"id":name,"triangles":len(obj.data.loop_triangles),"uv_layers":len(obj.data.uv_layers),"dimensions_m":list(obj.dimensions)})
        if name=="ornate-balcony-railing":
            # Three discrete safety panels keep the rear and center open.
            for i,(center,size) in enumerate([((0,.5,.55),(2.45,.06,1.1)),
                    ((-1.2,0,.55),(.06,1.04,1.1)),((1.2,0,.55),(.06,1.04,1.1))]):
                collision=box("UCX_source_"+str(i),center,size,0)
                collision.hide_render=True
        bpy.ops.wm.save_as_mainfile(filepath=str(OUT/f"{name}.blend"))
        select([obj])
        bpy.ops.export_scene.gltf(filepath=str(OUT/f"{name}.glb"),export_format="GLB",use_selection=True)
        if args.render:
            scene=bpy.context.scene
            scene.render.engine="CYCLES"; scene.cycles.samples=24
            scene.render.resolution_x=1280; scene.render.resolution_y=960; scene.render.resolution_percentage=100
            scene.world=bpy.data.worlds.new("Neutral studio"); scene.world.use_nodes=True
            scene.world.node_tree.nodes["Background"].inputs["Color"].default_value=(.35,.35,.35,1)
            center=sum((obj.matrix_world @ Vector(corner) for corner in obj.bound_box),Vector())/8
            bpy.ops.object.camera_add(location=center+Vector((4,6,3)))
            camera=bpy.context.object; camera.rotation_euler=(center-camera.location).to_track_quat("-Z","Y").to_euler()
            camera.data.type="ORTHO"; camera.data.ortho_scale=max(obj.dimensions)*1.4; scene.camera=camera
            bpy.ops.object.light_add(type="AREA",location=center+Vector((1,3,5)))
            light=bpy.context.object; light.data.energy=1100; light.data.shape="DISK"; light.data.size=4
            light.rotation_euler=(center-light.location).to_track_quat("-Z","Y").to_euler()
            preview=ROOT/"Saved/Pipeline/source-previews"; preview.mkdir(parents=True,exist_ok=True)
            scene.render.filepath=str(preview/f"{name}.png"); bpy.ops.render.render(write_still=True)
    (OUT/"geometry.json").write_text(json.dumps(report,indent=2),encoding="utf-8")
    print("WTG_KIT_READY "+json.dumps(report))


if __name__=="__main__": main()
