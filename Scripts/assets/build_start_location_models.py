"""Original metre-scale Nadodrze meshes. Run with Blender --background --python.

Buildings follow versioned OSM footprints; openings are actual wall gaps with
recessed glazing, frames, mouldings and service fronts, not window decals.
Estimated architecture is explicitly recorded in Data/start_location.json.
"""
import json
import math
import random
import argparse
import sys
from pathlib import Path
import bpy
from mathutils import Vector
from mathutils.geometry import tessellate_polygon

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0,str(Path(__file__).resolve().parent))
from architecture_geometry import roof_triangles, roof_height
OUTPUT = ROOT/'SourceAssets/Models/Wroclaw/StartLocation'
MATERIALS = ['Plaster', 'Stone', 'Frame', 'Glazing', 'Metal', 'Roof', 'Wood', 'Leaf', 'Rubber', 'Paint', 'Light', 'Bark', 'Soil']
COLORS = [(.78,.73,.63,1), (.64,.60,.52,1), (.82,.80,.71,1), (.035,.07,.095,1),
          (.08,.085,.09,1), (.26,.10,.045,1), (.16,.085,.035,1), (.08,.22,.035,1),
          (.025,.025,.025,1), (.12,.18,.23,1), (.75,.75,.6,1), (.16,.12,.08,1), (.08,.06,.035,1)]


class Mesh:
    def __init__(self): self.vertices, self.faces, self.slots, self.face_uvs, self.smooth = [], [], [], [], []

    def face(self, points, slot, uvs=None, smooth=False):
        start = len(self.vertices)
        self.vertices.extend(points)
        self.faces.append(tuple(range(start, start+len(points))))
        self.slots.append(slot)
        self.face_uvs.append(uvs);self.smooth.append(smooth)

    def box(self, center, size, slot=0, axes=None):
        if any(v<=0 for v in size):raise ValueError('Nonpositive architectural/prop box size '+str(size))
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
                poly.use_smooth=self.smooth[poly.index]
                normal = poly.normal; axis = max(range(3),key=lambda i:abs(normal[i]))
                projection = ((1,2),(0,2),(0,1))[axis]
                for corner,loop in enumerate(poly.loop_indices):
                    p = mesh.vertices[mesh.loops[loop].vertex_index].co
                    uv.data[loop].uv = self.face_uvs[poly.index][corner] if self.face_uvs[poly.index] else (p[projection[0]]/2,p[projection[1]]/2)
            # Connect bark rings for continuous normals; leaf vertices stay
            # independent, with their own atlas islands and flat leaf normals.
            if any(self.smooth):
                bpy.context.view_layer.objects.active=obj;obj.select_set(True)
                bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
                bpy.ops.mesh.remove_doubles(threshold=.00001);bpy.ops.object.mode_set(mode='OBJECT')
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
    flat = config['tags'].get('roof:shape') == 'flat'
    eaves = height-(.35 if flat else (2.46 if height>=18 else 1.66))
    foundation = min(p[2] for r in config['rings_m'] for p in r)-.3
    rings = [[Vector((p[0],-p[1],0)) for p in r] for r in config['rings_m']]
    street = Vector(((44611.8-config['origin_cm'][0])/100,-(62830.93-config['origin_cm'][1])/100,0))
    edges = []
    for ring_index,ring in enumerate(rings):
        area = sum(a.x*b.y-b.x*a.y for a,b in zip(ring,ring[1:]))
        if (area < 0) == (ring_index==0): ring = list(reversed(ring))
        rings[ring_index] = ring
        for a,b in zip(ring,ring[1:]):
            tangent = b-a; length = tangent.length
            if length < .05: continue
            tangent.normalize(); normal = Vector((tangent.y,-tangent.x,0))
            edges.append((a,b,tangent,normal,length))
    front = max(range(len(edges)),key=lambda i: ((street-(edges[i][0]+edges[i][1])/2).normalized().dot(edges[i][3]) if edges[i][4]>9 else -2))
    # The entrance datum follows its source DEM edge, while the roof datum is
    # still the source maximum height. Foundations extend through the slope.
    source_z={(round(p[0],5),round(-p[1],5)):p[2] for ring in config['rings_m'] for p in ring}
    entrance_base=sum(source_z[(round(p.x,5),round(p.y,5))] for p in edges[front][:2])/2+.2
    first_height=min(3.8,max(3.1,(eaves-entrance_base)/floors+0.5))
    attic_height=min(2.2,(eaves-entrance_base-first_height)/(floors-1))
    heights=[first_height]+[(eaves-entrance_base-first_height-attic_height)/max(1,floors-2)]*(floors-2)+[attic_height]
    floor_bases=[entrance_base+sum(heights[:i]) for i in range(floors)]
    for edge_index,(a,b,t,n,length) in enumerate(edges):
        axes = (t,n,Vector((0,0,1)))
        def box(x,z,w,h,depth=.28,offset=-.14,slot=0):
            m.box(a+t*x+n*offset+Vector((0,0,z)),(w,depth,h),slot,axes)
        c = Mesh(); c.box((a+b)/2-n*.14+Vector((0,0,(eaves+foundation)/2)),(length,.32,eaves-foundation),0,axes)
        collision.append(c)
        bays = max(1,int(length/2.8)); bay = length/bays
        if length < 2.3:
            box(length/2,(eaves+foundation)/2,length,eaves-foundation)
            continue
        floor_base = entrance_base
        for floor, floor_height in enumerate(heights):
            for column in range(bays):
                x = (column+.5)*bay
                storefront = floor==0 and edge_index==front
                entrance = storefront and column==bays//2
                w = min(bay-.55,1.65 if entrance else (2.15 if storefront else 1.18))
                h = min(floor_height-(.4 if storefront else .95),2.9 if storefront else (1.15 if floor==floors-1 else 1.95))
                sill = floor_base+(.08 if entrance else (.2 if storefront else .72))
                top = sill+h
                box(column*bay+(bay-w)/4,floor_base+floor_height/2,(bay-w)/2,floor_height)
                box((column+1)*bay-(bay-w)/4,floor_base+floor_height/2,(bay-w)/2,floor_height)
                box(x,(floor_base+sill)/2,w,sill-floor_base)
                box(x,(top+floor_base+floor_height)/2,w,floor_base+floor_height-top)
                # Recess reveals and glazing inside a real opening.
                box(x,sill+h/2,w-.06,h-.06,.035,-.36 if entrance else -.23,3)
                for dx in (-w/2,w/2): box(x+dx,sill+h/2,.08,h+.12,.13,-.07,2)
                for dz in (0,h): box(x,sill+dz,w+.12,.08,.13,-.07,2)
                box(x,sill+h/2,.052,h,.08,-.29 if entrance else -.045,2)
                box(x,sill+h*.7,w,.048,.08,-.045,2)
                box(x,sill-.08,w+.28,.13,.39,.025,1)
                if entrance:
                    # Deep stone jambs, timber leaf rails and transom; the
                    # exterior collision stays closed (no new interior system).
                    for dx in (-w/2-.1,w/2+.1):box(x+dx,sill+h/2,.17,h+.3,.48,-.1,1)
                    box(x,top+.12,w+.44,.24,.48,-.1,1)
                    for dx in (-w*.4,0,w*.4):box(x+dx,sill+h*.45,.085,h*.9,.10,-.29,6)
                    box(x,sill+h*.1,w,.24,.1,-.29,6)
                    box(x,sill+h*.83,w,.12,.1,-.29,6)
                    box(x+.13,sill+h*.45,.025,.30,.05,-.21,4)
                    box(x,sill-.09,w+.25,.16,.56,-.06,1)
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
        plinth_top=entrance_base if edge_index==front else entrance_base+.42
        box(length/2,(foundation+plinth_top)/2,length,plinth_top-foundation,.33,.015,1)
        for level,width in [(eaves+.02,.38),(eaves+.13,.50),(eaves+.23,.58)]:
            box(length/2,level,length,.12,width,width/2-.1,1)
        # Eaves and real drain pipes, not flat texture lines.
        # Open half-round gutter, seam lip and an elbow down to the plinth.
        for k in range(8):
            theta=math.pi+k*math.pi/8;theta2=theta+math.pi/8
            p=n*(.27+.065*math.cos(theta))+Vector((0,0,eaves+.16+.065*math.sin(theta)))
            q=n*(.27+.065*math.cos(theta2))+Vector((0,0,eaves+.16+.065*math.sin(theta2)))
            m.face([a+p,b+p,b+q,a+q],4)
        m.rod(a+n*.34+Vector((0,0,eaves+.16)),b+n*.34+Vector((0,0,eaves+.16)),.009,4,8)
        down=a+t*.25+n*.15
        ground=source_z[(round(a.x,5),round(a.y,5))]+.2
        m.rod(down+Vector((0,0,ground+.12)),down+Vector((0,0,eaves-.1)),.055,4,12)
        m.rod(down+Vector((0,0,eaves-.1)),a+t*.25+n*.27+Vector((0,0,eaves+.14)),.055,4,12)
        m.rod(down+Vector((0,0,ground+.12)),down+n*.1+Vector((0,0,ground+.03)),.055,4,12)
        if edge_index==front and config['style']!='plain':
            x = (int(bays/2)+.5)*bay
            for floor in (2,3,4):
                if floor>=floors-1: continue
                z = floor_bases[floor]+.65
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
    triangles=[[tuple(flattened[p] if isinstance(p,int) else p) for p in tri] for tri in tessellate_polygon(rings)]
    coordinates=[[tuple(p) for p in ring] for ring in rings]
    for tri in roof_triangles(triangles,coordinates,eaves+.29,height-.02,flat):m.face(tri,5)
    for i,p in enumerate(rings[0][:-1]):
        if i%4!=0:continue
        centre=p*.65
        # Only place a chimney if this roof point lies safely inside the roof.
        roof_z=roof_height(tuple(centre),coordinates,eaves+.29,height-.02,flat)
        if roof_z<eaves+.5 and not flat:continue
        chimney_top=min(height,roof_z+.8)
        if chimney_top-roof_z>.15:
            m.box((centre.x,centre.y,(roof_z+chimney_top)/2),(.65,.55,chimney_top-roof_z),1)
            m.box((centre.x,centre.y,chimney_top-.03),(.75,.65,.06),4)
    if max(p[2] for p in m.vertices)>height+.001:raise ValueError('Architecture exceeded source height '+config['asset'])
    return m,collision


def curb():
    m=Mesh()
    # Chamfered top lip, one-centimetre joints separate neighbouring pieces.
    # Batch two real metre-long stones rather than stretch one stone to 2 m.
    for x in (-.5,.5):
        m.box((x,0,.115),(.99,.16,.23),1)
        m.box((x,-.005,.236),(.98,.13,.012),1)
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


def tree(variant=0):
    rng=random.Random(45047+variant*173);m=Mesh()
    height=[5.4,5.9,7.2][variant];spread=[1.7,1.9,2.5][variant]
    def branch(points,radii,sides=10):
        rings=[]
        for i,(p,r) in enumerate(zip(points,radii)):
            d=(Vector(points[min(i+1,len(points)-1)])-Vector(points[max(0,i-1)])).normalized()
            u=d.cross(Vector((0,1,0))).normalized();v=d.cross(u)
            rings.append([Vector(p)+r*(u*math.cos(k*math.tau/sides)+v*math.sin(k*math.tau/sides)) for k in range(sides)])
        for a,b in zip(rings,rings[1:]):
            for k in range(sides):m.face([a[k],a[(k+1)%sides],b[(k+1)%sides],b[k]],11,smooth=True)
        m.face(list(reversed(rings[0])),11);m.face(rings[-1],11)
    trunk=[(0,0,-.06),(.04,-.03,.35),(.08,.02,1.8),(.02,.08,height*.58),(.12,.04,height*.88)]
    branch(trunk,[.17,.135,.105,.068,.014],16)
    for i in range(5):
        a=i*math.tau/5+.3
        branch([(0,0,.16),(.3*math.cos(a),.3*math.sin(a),.03),(.48*math.cos(a),.48*math.sin(a),-.015)],[.045,.028,.006],8)
    # Each branch supports an irregular ellipsoidal lobe of individual leaves.
    lobes=[]
    for i in range(18):
        a=i*2.399+rng.uniform(-.18,.18);level=i/17
        start=Vector((.06,.03,height*(.34+level*.38)))
        reach=spread*(.74+.26*math.sin(level*math.pi))
        end=Vector((math.cos(a)*reach,math.sin(a)*reach,height*(.60+level*.30)))
        mid=start.lerp(end,.52)+Vector((0,0,.18))
        branch([start,mid,end],[.055*(1-level*.45),.029,.006],10)
        for j in range(3):
            q=start.lerp(end,.46+j*.23)
            tip=q+Vector((math.cos(a+.6*(j-1))*.5,math.sin(a+.6*(j-1))*.5,.42))
            branch([q,tip],[.017,.003],7)
        lobes.append(end*.83+start*.17)
    count=[6000,6000,6800][variant]
    for i in range(count):
        c=lobes[i%len(lobes)];az=rng.random()*math.tau;z=rng.uniform(-1,1);r=rng.random()**(1/3)
        center=c+Vector((math.cos(az)*math.sqrt(1-z*z)*r*.95,math.sin(az)*math.sqrt(1-z*z)*r*.95,z*r*.95))
        angle=rng.random()*math.tau;tilt=rng.uniform(-.8,.8)
        u=Vector((math.cos(angle),math.sin(angle),tilt)).normalized()*rng.uniform(.07,.12)
        v=Vector((-math.sin(angle),math.cos(angle),rng.uniform(-.5,.5))).normalized()*u.length*.65
        # One leaf card, never duplicate a coincident backface. Shader is two-sided.
        cell=rng.randrange(2)+(2 if variant==1 else 0);x=(cell%2)*.5;y=(cell//2)*.5
        uv=[(x+.008,y+.008),(x+.492,y+.008),(x+.492,y+.492),(x+.008,y+.492)]
        m.face([center-u-v,center+u-v,center+u+v,center-u+v],7,uvs=uv)
    collision=Mesh();collision.box((.04,.01,1.55),(.29,.29,3.22),11)
    return m,[collision]


def tree_pit():
    m=Mesh();sides=32
    # Soil sits 2 mm above the selected ground; lip projects only 12 mm.
    for i in range(sides):
        a=i*math.tau/sides;b=(i+1)*math.tau/sides
        m.face([(0,0,0),(.72*math.cos(a),.72*math.sin(a),0),(.72*math.cos(b),.72*math.sin(b),0)],12)
        m.face([(.72*math.cos(a),.72*math.sin(a),-.012),(.75*math.cos(a),.75*math.sin(a),-.012),(.75*math.cos(b),.75*math.sin(b),-.012),(.72*math.cos(b),.72*math.sin(b),-.012)],4)
    return m,[]


def lamp():
    m=Mesh();m.rod((0,0,.04),(0,0,5.5),.055,4,20)
    # Functional modern street fixture, estimated placement on mapped lit roads.
    pts=[(0,0,5.5),(.12,0,5.75),(.42,0,5.88),(1.0,0,5.88)]
    for a,b in zip(pts,pts[1:]):m.rod(a,b,.039,4,16)
    m.box((1.06,0,5.86),(.55,.24,.075),4);m.box((1.06,0,5.813),(.45,.19,.015),10)
    m.box((0,0,.025),(.24,.24,.05),4)
    for x in (-.085,.085):
        for y in (-.085,.085):m.rod((x,y,.05),(x,y,.075),.014,4,6)
    c=Mesh();c.box((0,0,2.8),(.15,.15,5.65),4)
    return m,[c]


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
    parser=argparse.ArgumentParser();parser.add_argument('--asset');parser.add_argument('--buildings-only',action='store_true')
    options=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
    metadata=OUTPUT/'geometry.json'
    records=json.loads(metadata.read_text(encoding='utf-8'))['assets'] if (options.asset or options.buildings_only) and metadata.exists() else []
    updated=[]
    data=json.loads((ROOT/'Data/start_location_buildings.json').read_text(encoding='utf-8'))
    for config in data['buildings']+data.get('context_buildings',[]):
        if options.asset and options.asset!=config['asset']:continue
        reset();mesh,collision=building(config)
        record=save(config['asset'],mesh,collision,'Buildings')
        record['architecture']={'source_id':config['id'],'height_datum_m':config['height_m'],
            'maximum_z_m':round(max(p[2] for p in mesh.vertices),5),
            'foundation_z_m':round(min(p[2] for p in mesh.vertices),5),
            'roof':'flat' if config['tags'].get('roof:shape')=='flat' else 'interpreted hipped roof',
            'context':bool(config.get('context')),'footprint':'unaltered source rings; mouldings project beyond wall'}
        updated.append(record)
    for name,func,area in [('curb',curb,'Roads'),('drain',drain,'Roads'),('bike-rack',rack,'Props'),('young-tree',tree,'Vegetation'),('young-maple',lambda:tree(1),'Vegetation'),('park-lime',lambda:tree(2),'Vegetation'),('tree-pit',tree_pit,'Props'),('modern-lamp',lamp,'Props'),('parked-car',car,'Props')]:
        if options.buildings_only or (options.asset and options.asset!=name):continue
        reset();mesh,collision=func();updated.append(save(name,mesh,collision,area))
    if not updated:raise ValueError('Unknown production asset '+str(options.asset))
    if options.asset or options.buildings_only:
        by_name={r['asset']:r for r in updated}
        records=[by_name.pop(r['asset'],r) for r in records]+list(by_name.values())
    else:records=updated
    (OUTPUT/'geometry.json').write_text(json.dumps(dict(schema_version=1,author='WroclawTheGame original procedural authorship',
        license='Project-owned original geometry; building footprints derived from OpenStreetMap ODbL 1.0',assets=records),indent=2)+'\n',encoding='utf-8')
    print('START_MODELS_CREATED '+json.dumps({r['asset']:r['triangles'] for r in records}))


if __name__=='__main__':main()
