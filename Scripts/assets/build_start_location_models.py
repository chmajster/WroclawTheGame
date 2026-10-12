"""Original metre-scale Nadodrze meshes. Run with Blender --background --python.

Buildings follow versioned OSM footprints; openings are actual wall gaps with
recessed glazing, frames, mouldings and service fronts, not window decals.
Estimated architecture is explicitly recorded in Data/start_location.json.
"""
import json
import math
import random
from pathlib import Path
import bpy
from mathutils import Vector
from mathutils.geometry import tessellate_polygon

ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT/'SourceAssets/Models/Wroclaw/StartLocation'
MATERIALS = ['Plaster', 'Stone', 'Frame', 'Glazing', 'Metal', 'Roof', 'Wood', 'Leaf', 'Rubber', 'Paint', 'Light']
COLORS = [(.78,.73,.63,1), (.64,.60,.52,1), (.82,.80,.71,1), (.035,.07,.095,1),
          (.08,.085,.09,1), (.26,.10,.045,1), (.16,.085,.035,1), (.08,.22,.035,1),
          (.025,.025,.025,1), (.12,.18,.23,1), (.75,.75,.6,1)]


class Mesh:
    def __init__(self): self.vertices, self.faces, self.slots = [], [], []

    def face(self, points, slot):
        start = len(self.vertices)
        self.vertices.extend(points)
        self.faces.append(tuple(range(start, start+len(points))))
        self.slots.append(slot)

    def box(self, center, size, slot=0, axes=None):
        axes = axes or ((1,0,0),(0,1,0),(0,0,1))
        c = Vector(center); a = [Vector(v) for v in axes]
        verts = [c+a[0]*x*size[0]/2+a[1]*y*size[1]/2+a[2]*z*size[2]/2
                 for x,y,z in [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]]
        faces = [(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]
        invert = a[0].cross(a[1]).dot(a[2]) < 0
        for f in faces: self.face([verts[i] for i in (tuple(reversed(f)) if invert else f)],slot)

    def rod(self, start, end, radius, slot=4, sides=10):
        a,b = Vector(start),Vector(end); direction = b-a
        if direction.length < 1e-5: return
        d = direction.normalized(); u = d.cross(Vector((0,0,1)))
        if u.length < .01: u = d.cross(Vector((0,1,0)))
        u.normalize(); v = d.cross(u)
        rings = [[p+radius*(u*math.cos(i*math.tau/sides)+v*math.sin(i*math.tau/sides)) for i in range(sides)] for p in (a,b)]
        for i in range(sides): self.face([rings[0][i],rings[0][(i+1)%sides],rings[1][(i+1)%sides],rings[1][i]],slot)
        self.face(list(reversed(rings[0])),slot); self.face(rings[1],slot)

    def object(self, name, collision=False):
        mesh = bpy.data.meshes.new(name); mesh.from_pydata(self.vertices,[],self.faces); mesh.update()
        obj = bpy.data.objects.new(name,mesh); bpy.context.collection.objects.link(obj)
        if not collision:
            for material in bpy.data.materials: mesh.materials.append(material)
            uv = mesh.uv_layers.new(name='UV_Physical2m')
            for poly,slot in zip(mesh.polygons,self.slots):
                poly.material_index = slot
                normal = poly.normal; axis = max(range(3),key=lambda i:abs(normal[i]))
                projection = ((1,2),(0,2),(0,1))[axis]
                for loop in poly.loop_indices:
                    p = mesh.vertices[mesh.loops[loop].vertex_index].co
                    uv.data[loop].uv = (p[projection[0]]/2,p[projection[1]]/2)
        return obj


def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.preferences.filepaths.save_version = 0
    bpy.context.scene.unit_settings.system = 'METRIC'
    for name,color in zip(MATERIALS,COLORS):
        mat = bpy.data.materials.new(name); mat.diffuse_color = color
    # Iteration order of bpy.data.materials is alphabetical; use explicit names.


def fix_slots(obj):
    obj.data.materials.clear()
    for name in MATERIALS: obj.data.materials.append(bpy.data.materials[name])


def building(config):
    m = Mesh(); collision = []
    height, floors = config['height_m'], config['floors']
    rings = [[Vector((p[0],-p[1],0)) for p in r] for r in config['rings_m']]
    street = Vector(((44611.8-config['origin_cm'][0])/100,-(62830.93-config['origin_cm'][1])/100,0))
    edges = []
    for ring in rings:
        area = sum(a.x*b.y-b.x*a.y for a,b in zip(ring,ring[1:]))
        if area < 0: ring = list(reversed(ring))
        for a,b in zip(ring,ring[1:]):
            tangent = b-a; length = tangent.length
            if length < .05: continue
            tangent.normalize(); normal = Vector((tangent.y,-tangent.x,0))
            edges.append((a,b,tangent,normal,length))
    front = max(range(len(edges)),key=lambda i: ((street-(edges[i][0]+edges[i][1])/2).normalized().dot(edges[i][3]) if edges[i][4]>9 else -2))
    for edge_index,(a,b,t,n,length) in enumerate(edges):
        axes = (t,n,Vector((0,0,1)))
        def box(x,z,w,h,depth=.28,offset=-.14,slot=0):
            m.box(a+t*x+n*offset+Vector((0,0,z)),(w,depth,h),slot,axes)
        c = Mesh(); c.box((a+b)/2-n*.14+Vector((0,0,height/2)),(length,.32,height),0,axes)
        collision.append(c)
        bays = max(1,int(length/2.8)); bay = length/bays
        if length < 2.3:
            box(length/2,height/2,length,height)
            continue
        heights = [3.8]+[(height-6)/max(1,floors-2)]*(floors-2)+[2.2]
        floor_base = 0
        for floor, floor_height in enumerate(heights):
            for column in range(bays):
                x = (column+.5)*bay
                storefront = floor==0 and edge_index==front
                w = min(bay-.55,2.15 if storefront else 1.18)
                h = min(floor_height-.5,2.9 if storefront else (1.15 if floor==floors-1 else 1.95))
                sill = floor_base+(.2 if storefront else .72)
                top = sill+h
                box(column*bay+(bay-w)/4,floor_base+floor_height/2,(bay-w)/2,floor_height)
                box((column+1)*bay-(bay-w)/4,floor_base+floor_height/2,(bay-w)/2,floor_height)
                box(x,(floor_base+sill)/2,w,sill-floor_base)
                box(x,(top+floor_base+floor_height)/2,w,floor_base+floor_height-top)
                # Recess reveals and glazing inside a real opening.
                box(x,sill+h/2,w-.06,h-.06,.035,-.23,3)
                for dx in (-w/2,w/2): box(x+dx,sill+h/2,.08,h+.12,.13,-.07,2)
                for dz in (0,h): box(x,sill+dz,w+.12,.08,.13,-.07,2)
                box(x,sill+h/2,.052,h,.08,-.045,2)
                box(x,sill+h*.7,w,.048,.08,-.045,2)
                box(x,sill-.08,w+.28,.13,.39,.025,1)
                if floor>0 and floor<floors-1 and config['style']!='plain':
                    box(x,top+.13,w+.35,.14,.22,.045,1)
                    box(x,top+.23,w+.43,.06,.28,.07,1)
                    if config['style']=='pediment' and floor in (2,3,4):
                        base=a+t*(x-w*.65)+n*.13+Vector((0,0,top+.28))
                        peak=a+t*x+n*.13+Vector((0,0,top+.57))
                        end=a+t*(x+w*.65)+n*.13+Vector((0,0,top+.28))
                        m.rod(base,peak,.055,1,8);m.rod(peak,end,.055,1,8);m.rod(base,end,.055,1,8)
                if storefront:
                    box(x,top+.25,w+.1,.26,.08,.03,6)
                    for dx in (-w*.4,w*.4): box(x+dx,sill+.2,.025,.28,.06,.01,4)
            if floor in (0,1,floors-2):
                for depth,thickness,dz in [(.16,.09,0),(.25,.08,.09),(.34,.055,.16)]:
                    box(length/2,floor_base+floor_height+dz,length,thickness,depth,depth/2-.01,1)
            floor_base += floor_height
        box(length/2,.22,length,.44,.33,.015,1)
        for level,width in [(height+.02,.38),(height+.13,.50),(height+.23,.58)]:
            box(length/2,level,length,.12,width,width/2-.1,1)
        # Eaves and real drain pipes, not flat texture lines.
        m.rod(a+n*.27+Vector((0,0,height+.12)),b+n*.27+Vector((0,0,height+.12)),.065,4,12)
        m.rod(a+t*.25+n*.15+Vector((0,0,.3)),a+t*.25+n*.15+Vector((0,0,height)),.055,4,12)
        if edge_index==front and config['style']!='plain':
            x = (int(bays/2)+.5)*bay
            for floor in (2,3,4):
                if floor>=floors-1: continue
                z = 3.8+(floor-1)*(height-6)/max(1,floors-2)+.65
                box(x,z,2.4,.18,1.04,.46,1)
                for dx in (-1.13,1.13):
                    m.rod(a+t*(x+dx)+n*.97+Vector((0,0,z)),a+t*(x+dx)+n*.97+Vector((0,0,z+1.04)),.035,4,12)
                    m.rod(a+t*(x+dx)+n*.1+Vector((0,0,z+1.04)),a+t*(x+dx)+n*.97+Vector((0,0,z+1.04)),.025,4,10)
                for dz in (.2,1.04):m.rod(a+t*(x-1.13)+n*.97+Vector((0,0,z+dz)),a+t*(x+1.13)+n*.97+Vector((0,0,z+dz)),.028,4,10)
                for i in range(13):
                    dx=-1.1+i*2.2/12
                    m.rod(a+t*(x+dx)+n*.97+Vector((0,0,z+.2)),a+t*(x+dx)+n*.97+Vector((0,0,z+1.04)),.014,4,8)
                    points=[a+t*(x+dx+.065*math.cos(k*math.tau/16))+n*.97+Vector((0,0,z+.62+.15*math.sin(k*math.tau/16))) for k in range(17)]
                    for p,q in zip(points,points[1:]):m.rod(p,q,.009,4,6)
    # Tiled upper roof, parapet, ventilation chimneys on the source footprint.
    flattened=[p for ring in rings for p in ring]
    for tri in tessellate_polygon(rings):
        points=[flattened[p] if isinstance(p,int) else p for p in tri]
        m.face([Vector((p.x,p.y,height+.05)) for p in points],5)
    for i,p in enumerate(rings[0][:-1]):
        if i%4==0:m.box((p.x*.65,p.y*.65,height+.45),(.65,.55,.8),1)
    return m,collision


def curb():
    m=Mesh();m.box((0,0,.115),(1,.16,.23),1)
    # Chamfered top lip, one-centimetre joints separate neighbouring pieces.
    m.box((0,-.005,.236),(.98,.13,.012),1)
    return m,[]


def drain():
    m=Mesh();m.box((0,0,.02),(.45,.35,.04),4)
    for i in range(9):m.box((-.18+i*.045,0,.043),(.018,.3,.014),4)
    for x in (-.21,.21):m.box((x,0,.052),(.02,.35,.02),4)
    return m,[]


def rack():
    m=Mesh(); points=[(-.42,0,0),(-.42,0,.57)]+[(-.42*math.cos(k*math.pi/20),0,.57+.42*math.sin(k*math.pi/20)) for k in range(21)]+[(.42,0,0)]
    for a,b in zip(points,points[1:]):m.rod(a,b,.03,4,16)
    for x in (-.42,.42):
        m.box((x,0,.025),(.15,.13,.05),4)
        for y in (-.04,.04):m.rod((x,y,.05),(x,y,.065),.011,4,6)
    return m,[]


def tree():
    rng=random.Random(45047);m=Mesh()
    m.rod((0,0,0),(.08,.04,3.1),.095,6,16)
    for i in range(15):
        angle=i*2.399;start=Vector((.06,.03,2+i*.16));end=Vector((math.cos(angle)*1.35,math.sin(angle)*1.35,3.5+i*.10))
        m.rod(start,end,.032,6,10)
        for j in range(4):
            q=start.lerp(end,.35+j*.2);e=q+Vector((rng.uniform(-.5,.5),rng.uniform(-.5,.5),.65))
            m.rod(q,e,.014,6,8)
            for k in range(15):
                center=q.lerp(e,rng.random())+Vector((rng.uniform(-.28,.28),rng.uniform(-.28,.28),rng.uniform(-.1,.18)))
                u=Vector((math.cos(angle+k),math.sin(angle+k),rng.uniform(-.3,.3)))*rng.uniform(.08,.14)
                v=Vector((-u.y,u.x,.04))*.5
                for side in (-1,1):
                    m.face([center-u,center-v*side,center+u,center+v*side],7)
    for x,y in [(-.28,0),(.28,0)]:
        m.rod((x,y,0),(x,y,1.4),.032,6,10)
        m.rod((x,y,1.1),(.06,0,1.1),.014,4,8)
    return m,[]


def car():
    m=Mesh()
    # Rounded original saloon shell, sectioned body, curved cabin, separate tyres.
    sections=[(-2.2,.55,.48),(-2.05,.83,.7),(-1.65,.9,.8),(-.8,.9,.86),(.65,.9,.86),(1.65,.84,.72),(2.12,.62,.56)]
    rings=[]
    for x,w,h in sections:
        rings.append([Vector((x,w*math.cos(k*math.tau/24),.47+h*.45+h*.45*math.sin(k*math.tau/24))) for k in range(24)])
    for a,b in zip(rings,rings[1:]):
        for k in range(24):m.face([a[k],a[(k+1)%24],b[(k+1)%24],b[k]],9)
    m.face(list(reversed(rings[0])),9);m.face(rings[-1],9)
    cabin=[(-1.05,.77,.88),(-.45,.66,1.45),(.65,.64,1.46),(1.24,.77,.9)]
    for a,b in zip(cabin,cabin[1:]):
        for side in (-1,1):m.face([(a[0],a[1]*side,a[2]),(b[0],b[1]*side,b[2]),(b[0],0,b[2]),(a[0],0,a[2])],9 if a==cabin[1] else 3)
    for side in (-1,1):
        m.face([(-1.05,.775*side,.89),(-.45,.665*side,1.45),(.65,.645*side,1.46),(1.24,.775*side,.91)],3)
        for x in (-.9,.12,1.15):m.rod((x,.8*side,.88),(min(.65,max(-.45,x)),.67*side,1.44),.025,9,8)
        for x in (-1.4,1.35):
            m.rod((x,.76*side,.34),(x,.99*side,.34),.31,8,32)
            m.rod((x,.99*side,.34),(x,1.005*side,.34),.19,4,24)
            for k in range(8):
                a=k*math.tau/8;m.rod((x,1.015*side,.34),(x+.17*math.cos(a),1.015*side,.34+.17*math.sin(a)),.016,2,6)
        for x in (-.45,.55):m.box((x,.89*side,.83),(.17,.04,.035),4)
        m.box((.88,.94*side,1.05),(.22,.16,.13),9)
        for end in (-1,1):m.box((2.02*end,.46*side,.75),(.08,.36,.12),10)
    for x in (-2.09,2.05):m.box((x,0,.53),(.1,1.25,.12),4)
    m.box((2.09,0,.7),(.055,.58,.16),2)
    return m,[]


def save(name,mesh,colliders,area):
    obj=mesh.object(name);fix_slots(obj)
    used=sorted(set(mesh.slots)); remap={old:new for new,old in enumerate(used)}
    obj.data.materials.clear()
    for slot in used:obj.data.materials.append(bpy.data.materials[MATERIALS[slot]])
    for poly,slot in zip(obj.data.polygons,mesh.slots):poly.material_index=remap[slot]
    for i,collider in enumerate(colliders):collider.object(f'UCX_{name}_{i:02d}',True)
    bpy.context.view_layer.update()
    points=[Vector(p) for p in obj.bound_box]
    minimum=Vector(tuple(min(p[i] for p in points) for i in range(3)))
    maximum=Vector(tuple(max(p[i] for p in points) for i in range(3)))
    obj.data.calc_loop_triangles()
    folder=OUTPUT/area;folder.mkdir(parents=True,exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(folder/(name+'.blend')))
    bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
    bpy.ops.export_scene.gltf(filepath=str(folder/(name+'.glb')),export_format='GLB',use_selection=True)
    return dict(asset=name,area=area,triangles=len(obj.data.loop_triangles),
                bounds_m=list(maximum-minimum),source_center_m=[(minimum.x+maximum.x)/2,(minimum.y+maximum.y)/2,minimum.z],
                collision_shapes=len(colliders),materials=[MATERIALS[i] for i in used],uv='2m physical projection')


def main():
    records=[]
    data=json.loads((ROOT/'Data/start_location_buildings.json').read_text(encoding='utf-8'))
    for config in data['buildings']:
        reset();mesh,collision=building(config);records.append(save(config['asset'],mesh,collision,'Buildings'))
    for name,func,area in [('curb',curb,'Roads'),('drain',drain,'Roads'),('bike-rack',rack,'Props'),('young-tree',tree,'Vegetation'),('parked-car',car,'Props')]:
        reset();mesh,collision=func();records.append(save(name,mesh,collision,area))
    (OUTPUT/'geometry.json').write_text(json.dumps(dict(schema_version=1,author='WroclawTheGame original procedural authorship',
        license='Project-owned original geometry; building footprints derived from OpenStreetMap ODbL 1.0',assets=records),indent=2)+'\n',encoding='utf-8')
    print('START_MODELS_CREATED '+json.dumps({r['asset']:r['triangles'] for r in records}))


if __name__=='__main__':main()
