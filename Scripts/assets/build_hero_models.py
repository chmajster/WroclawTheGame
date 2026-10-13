"""Civilian hero: MakeHuman anatomy and authored CC0 garment/hair proxies.

Reuses the project's existing graphical-data reader and rig/morph helpers.
Clothes are authored garment topology, not copied/inflated body sections.
No MakeHuman application/plugin code is imported. Run Blender headless.
"""
from pathlib import Path
import math,json,hashlib
import bpy,numpy as np
from mathutils import Vector
from mathutils.kdtree import KDTree
ROOT=Path(__file__).resolve().parents[2]
MH=ROOT/'SourceAssets/Characters/MakeHuman/makehuman/data'
OUT=ROOT/'SourceAssets/Characters/WroclawHero';OUT.mkdir(parents=True,exist_ok=True)
EXPORT=ROOT/'Saved/Pipeline/generated/wtg-civilian';EXPORT.mkdir(parents=True,exist_ok=True)
DEFS=json.loads((ROOT/'Data/hero_morphs.json').read_text(encoding='utf-8'))['morphs']
records=[]

def obj(path):
    verts=[];uv=[];faces=[];uvfaces=[];groups=[];group='body'
    for line in path.read_text().splitlines():
        p=line.split()
        if not p:continue
        if p[0]=='v':verts.append(list(map(float,p[1:4])))
        elif p[0]=='vt':uv.append(list(map(float,p[1:3])))
        elif p[0]=='g':group=p[1]
        elif p[0]=='f':
            tokens=[v.split('/') for v in p[1:]];faces.append([int(v[0])-1 for v in tokens]);uvfaces.append([int(v[1])-1 if len(v)>1 and v[1] else 0 for v in tokens]);groups.append(group)
    return np.array(verts),uv,faces,uvfaces,groups

RAW,UV,FACES,UVF,GROUPS=obj(MH/'3dobjs/base.obj')
EYE,EU,EF,EUF,EG=obj(MH/'eyes/high-poly/high-poly.obj')
mapping=[];active=False
for l in (MH/'eyes/high-poly/high-poly.mhclo').read_text().splitlines():
    if l.startswith('verts '):active=True;continue
    if active:
        t=l.split()
        if len(t)==9:
            try:mapping.append((np.array(t[:3],int),np.array(t[3:6],float),np.array(t[6:9],float)))
            except ValueError:break
        elif t:break
assert len(mapping)==len(EYE),(len(mapping),len(EYE))

def target(name):
    d=np.zeros_like(RAW)
    for l in (MH/'targets'/f'{name}.target').read_text().splitlines():
        t=l.split()
        if not t or l.startswith('#'):continue
        i=int(t[0]);assert 0<=i<len(RAW);d[i]+=np.array(t[1:4],float)
    return d

def mesh(name,vertices,faces,uvfaces=None,uvcoords=None):
    data=bpy.data.meshes.new(name);data.from_pydata(vertices,[],faces);data.update()
    o=bpy.data.objects.new(name,data);bpy.context.collection.objects.link(o)
    if uvfaces:
        layer=data.uv_layers.new(name='UVMap')
        for poly,tex in zip(data.polygons,uvfaces):
            for loop,tid in zip(poly.loop_indices,tex):layer.data[loop].uv=uvcoords[tid]
    else:
        bpy.context.view_layer.objects.active=o;o.select_set(True)
        bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(island_margin=.015);bpy.ops.object.mode_set(mode='OBJECT');o.select_set(False)
    for p in data.polygons:p.use_smooth=True
    return o

def material(name,color):
    m=bpy.data.materials.get(name) or bpy.data.materials.new(name);m.diffuse_color=(*color,1);return m

def bind(o,rig,weights=None,head=False):
    if head:
        g=o.vertex_groups.new(name='Head');g.add(list(range(len(o.data.vertices))),1,'REPLACE')
    elif weights is not None:
        source,source_indices=weights
        for group in source.vertex_groups:o.vertex_groups.new(name=group.name)
        for i,source_index in enumerate(source_indices):
            for group in source.data.vertices[source_index].groups:o.vertex_groups[group.group].add([i],group.weight,'REPLACE')
    else:
        bones=[b for b in rig.data.bones if b.use_deform and b.name not in {'Root','root','Armature'}]
        positions=np.array([v.co[:] for v in o.data.vertices]);dist=[]
        for b in bones:
            a=np.array(b.head_local);z=np.array(b.tail_local)-a
            t=np.clip(((positions-a)@z)/max(z@z,1e-8),0,1);dist.append(np.sum((positions-a-t[:,None]*z)**2,axis=1))
        d=np.array(dist).T;ids=np.argsort(d,axis=1)[:,:4];w=1/(np.take_along_axis(d,ids,axis=1)+.000025)**2;w/=w.sum(axis=1)[:,None]
        for bi,b in enumerate(bones):
            g=o.vertex_groups.new(name=b.name)
            for vi,col in zip(*np.where(ids==bi)):g.add([int(vi)],float(w[vi,col]),'REPLACE')
    mod=o.modifiers.new('Creator rig','ARMATURE');mod.object=rig;o.parent=rig

def shapes(o,base,deltas):
    o.shape_key_add(name='Basis')
    for name,d in deltas.items():
        if np.max(np.abs(d))<.000001:continue
        if np.max(np.abs(d))<1e-8:continue
        key=o.shape_key_add(name=name);key.slider_min=-1;key.slider_max=1
        values=np.asarray(base+d,dtype=np.float32).ravel();key.data.foreach_set('co',values)

def save_export(o,rig,sex,kind):
    folder=OUT/sex;folder.mkdir(exist_ok=True)
    bpy.ops.object.select_all(action='DESELECT');o.select_set(True);rig.select_set(True);bpy.context.view_layer.objects.active=o
    fbx=EXPORT/f'{sex}_{o.name}.fbx'
    bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,
        bake_anim=False,use_mesh_modifiers=False,mesh_smooth_type='FACE',axis_forward='-Y',axis_up='Z')
    vertices=len(o.data.vertices);morphs=[]
    if o.data.shape_keys:
        base=np.array([v.co[:] for v in o.data.shape_keys.key_blocks[0].data])
        for k in o.data.shape_keys.key_blocks[1:]:
            delta=np.array([v.co[:] for v in k.data])-base
            morphs.append({'name':k.name,'changed_vertices':int(np.count_nonzero(np.linalg.norm(delta,axis=1)>1e-6)),'max_delta_cm':float(np.max(np.linalg.norm(delta,axis=1))*100)})
    records.append({'id':sex+'_'+o.name,'sex':sex,'kind':kind,'fbx':str(fbx.relative_to(ROOT)).replace('\\','/'),
        'vertices':vertices,'triangles':sum(len(p.vertices)-2 for p in o.data.polygons),'morphs':morphs,
        'bounds_min_m':np.min(np.array([v.co[:] for v in o.data.vertices]),axis=0).tolist(),
        'bounds_max_m':np.max(np.array([v.co[:] for v in o.data.vertices]),axis=0).tolist(),
        'uv_layers':len(o.data.uv_layers),'weighted_vertices':sum(bool(v.groups) for v in o.data.vertices),
        'materials':[m.name for m in o.data.materials],'source_blend':f'SourceAssets/Characters/WroclawHero/{sex}/hero.blend',
        'fbx_sha256':hashlib.sha256(fbx.read_bytes()).hexdigest()})

def create(sex):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.preferences.filepaths.save_version=0
    bpy.ops.import_scene.gltf(filepath=str(ROOT/'SourceAssets/Characters/Quaternius/UniversalBaseCharacters/BaseCharacters/Superhero_Male_FullBody.gltf'))
    rig=next(o for o in bpy.data.objects if o.type=='ARMATURE')
    for o in list(bpy.data.objects):
        if o.type!='ARMATURE':bpy.data.objects.remove(o,do_unlink=True)
    raw=RAW+target('macrodetails/caucasian-'+sex+'-young')
    bodyfaces=[f for f,g in zip(FACES,GROUPS) if g=='body'];used=sorted({i for f in bodyfaces for i in f});index={v:i for i,v in enumerate(used)}
    scale=1.8/(raw[used,1].max()-raw[used,1].min());floor=raw[used,1].min()
    def convert(v):return np.column_stack((v[:,0]*scale,-v[:,2]*scale,(v[:,1]-floor)*scale))
    def joint(name):
        ids=set(i for f,g in zip(FACES,GROUPS) if g==name for i in f);return convert(raw[list(ids)]).mean(axis=0)
    # Raise the base's A-pose arms to the unchanged game's T-pose rest rig.
    segments=[]
    for side,suffix in [('l','l'),('r','r')]:
        a=joint('joint-'+side+'-shoulder');b=joint('joint-'+side+'-elbow');c=joint('joint-'+side+'-hand')
        for src1,src2,bone in [(a,b,'upperarm_'+suffix),(b,c,'lowerarm_'+suffix)]:
            dest=rig.data.bones[bone];q=Vector(src2-src1).rotation_difference(dest.tail_local-dest.head_local)
            segments.append((side,src1,src2,np.array(dest.head_local),np.array(q.to_matrix())))
    def pose(v):
        result=v.copy()
        for side in ['l','r']:
            mask=(v[:,0]*(1 if side=='l' else -1)>.12)&(v[:,2]>1.0)
            ids=np.where(mask)[0]
            if not len(ids):continue
            points=v[ids];mapped=[];dist=[]
            for s,a,b,d,r in segments:
                if s!=side:continue
                z=b-a;t=np.clip((points-a)@z/(z@z),0,1)
                dist.append(np.sum((points-a-t[:,None]*z)**2,axis=1));mapped.append((points-a)@r.T+d)
            w=1/(np.array(dist).T+.0001)**3;w/=w.sum(axis=1)[:,None]
            p=sum(m*w[:,i,None] for i,m in enumerate(mapped));fade=np.clip((np.abs(points[:,0])-.12)/.1,0,1)
            result[ids]=points*(1-fade[:,None])+p*fade[:,None]
        return result
    def eyes_for(v):
        # Proxy offsets locate the eyes inside the base mesh, not in the OBJ's authoring coordinates.
        return np.array([np.sum(v[ids]*w[:,None],axis=0)+offset for ids,w,offset in mapping])
    base=pose(convert(raw[used]));eyes=pose(convert(eyes_for(raw)));nv=len(base);base=np.vstack((base,eyes))
    faces=[[index[i] for i in f] for f in bodyfaces]+[[nv+i for i in f] for f in EF]
    uvfaces=[f for f,g in zip(UVF,GROUPS) if g=='body']+[[len(UV)+i for i in f] for f in EUF]
    deltas={}
    for definition in DEFS:
        d=sum((target(s) for s in definition['sources']),np.zeros_like(raw))
        shifted=pose(convert((raw+d)[used]));shift_eye=pose(convert(eyes_for(raw+d)))
        diff=np.vstack((shifted,shift_eye))-base
        custom=definition.get('custom');p=base
        if custom:
            z=p[:,2];x=p[:,0];y=p[:,1]
            if custom=='jaw-width':diff[:,0]=x*.3*np.exp(-((z-1.58)/.045)**2)*(np.abs(x)<.12)
            elif custom=='shoulders':diff[:,0]=x*.18*np.exp(-((z-1.43)/.16)**2)
            elif custom=='waist':diff[:,0]=x*.22*np.exp(-((z-1.11)/.13)**2);diff[:,1]=y*.15*np.exp(-((z-1.11)/.13)**2)
            elif custom=='hips':diff[:,0]=x*.2*np.exp(-((z-.92)/.13)**2)
            elif custom in ['weight','muscle']:
                f=np.clip((1.52-z)/.15,0,1)*np.clip((z-.1)/.2,0,1)
                diff[:,0]=x*(.2 if custom=='weight' else .09)*f;diff[:,1]=y*(.3 if custom=='weight' else .18)*f
            elif custom=='torso':diff[:,2]=.045*np.exp(-((z-1.23)/.25)**2)
        name=definition['id'].replace('.','_');assert np.max(np.abs(diff))>1e-6,name;deltas[name]=diff
    body=mesh('Body',base,faces,uvfaces,UV+EU)
    for name,col in [('Face',(.55,.32,.22)),('Torso',(.55,.32,.22)),('Arms',(.55,.32,.22)),('Legs',(.55,.32,.22)),('Feet',(.55,.32,.22)),('Eyes',(.85,.87,.83)),('Hands',(.55,.32,.22))]:body.data.materials.append(material(name,col))
    for poly in body.data.polygons:
        center=np.mean(base[list(poly.vertices)],axis=0);z=center[2]
        poly.material_index=5 if min(poly.vertices)>=nv else (0 if z>1.53 and abs(center[0])<.13 else (4 if z<.16 else (3 if z<1.0 else (2 if abs(center[0])>.20 else 1))))
        if poly.material_index==2 and abs(center[0])>abs(rig.data.bones['hand_l'].head_local.x)-.01:
            poly.material_index=6
    colors=body.data.color_attributes.new(name='CreatorRegions',type='FLOAT_COLOR',domain='CORNER')
    for poly in body.data.polygons:
        for loop in poly.loop_indices:
            p=base[body.data.loops[loop].vertex_index];face=float(p[2]>1.52 and abs(p[0])<.1)
            lip=math.exp(-((p[2]-1.643)/.011)**2-((p[0])/.04)**2) if p[1]<-.05 else 0
            colors.data[loop].color=(face,lip,0,1)
    bind(body,rig);shapes(body,base,deltas);save_export(body,rig,sex,'body')
    tree=KDTree(nv)
    for i,p in enumerate(base[:nv]):tree.insert(Vector(p),i)
    tree.balance()

    def proxy(folder,slug,name,kind,filter_faces=None):
        source=ROOT/'SourceAssets/Characters/MakeHuman/system-assets'/folder/slug
        pv,pu,pf,puf,_=obj(source/(slug+'.obj'))
        scales=np.ones(3);mapping=[];active=False
        for line in (source/(slug+'.mhclo')).read_text().splitlines():
            tokens=line.split()
            if not tokens or tokens[0].startswith('#'):continue
            if tokens[0] in ['x_scale','y_scale','z_scale']:
                axis='xyz'.index(tokens[0][0]);a,b=int(tokens[1]),int(tokens[2])
                scales[axis]=abs(raw[a,axis]-raw[b,axis])/float(tokens[3])
            if tokens[0]=='verts':active=True;continue
            if active:
                if len(tokens)==9:
                    mapping.append((np.array(tokens[:3],int),np.array(tokens[3:6],float),np.array(tokens[6:9],float)))
                elif len(tokens)==1 and tokens[0].isdigit():
                    mapping.append((np.array([int(tokens[0])]*3),np.array([1.,0,0]),np.zeros(3)))
                else:break
        assert len(mapping)==len(pv),(slug,len(mapping),len(pv))
        fitted=np.array([np.sum(raw[ids]*w[:,None],axis=0)+offset*scales for ids,w,offset in mapping])
        fitted=pose(convert(fitted))
        selected=[i for i,f in enumerate(pf) if filter_faces is None or filter_faces(fitted[f].mean(axis=0))]
        used=sorted({v for i in selected for v in pf[i]});remap={v:i for i,v in enumerate(used)}
        positions=fitted[used]
        faces=[[remap[v] for v in pf[i]] for i in selected];texfaces=[puf[i] for i in selected]
        o=mesh(name,positions,faces,texfaces,pu)
        if kind=='clothing' and name!='Shoes':
            # Export actual thickness while retaining shape keys (FBX's modifier
            # evaluation otherwise drops morphs). Copy the inner shell and hems.
            normals=np.array([v.normal[:] for v in o.data.vertices]);n=len(positions)
            positions=np.vstack((positions,positions-normals*.0018))
            edge_counts={}
            for face in faces:
                for a,b in zip(face,face[1:]+face[:1]):
                    key=tuple(sorted((a,b)));edge_counts[key]=edge_counts.get(key,0)+1
            inner=[[v+n for v in reversed(face)] for face in faces]
            sides=[[a,b,b+n,a+n] for (a,b),count in edge_counts.items() if count==1]
            sideuv=[[0,0,0,0] for _ in sides]
            bpy.data.objects.remove(o,do_unlink=True)
            o=mesh(name,positions,faces+inner+sides,texfaces+[list(reversed(t)) for t in texfaces]+sideuv,pu)
        o.data.materials.append(material('Hair' if kind=='hair' else ('Brows' if kind=='brows' else ('Shoe' if name=='Shoes' else ('Shirt' if name=='Shirt' else 'Denim'))),(.1,.12,.14)))
        nearest=np.array([tree.find(Vector(p))[1] for p in positions])
        bind(o,rig,weights=(body,nearest))
        # The same morphology affects fitted garments/accessories, avoiding drift.
        shapes(o,positions,{k:d[nearest] for k,d in deltas.items() if k.startswith('Body_') or kind in ['hair','brows']})
        save_export(o,rig,sex,kind)
        return o

    shirt=proxy('clothes','male_casualsuit01','Shirt','clothing',lambda p:p[2]>.98)
    trousers=proxy('clothes','male_casualsuit01','Trousers','clothing',lambda p:p[2]<=.98)
    shoes=proxy('clothes','shoes01','Shoes','clothing')
    hair=proxy('hair','short02','HairShort','hair')
    brows=proxy('eyebrows','eyebrow001','BrowsNatural','brows')

    def panel(center,size,radius):
        bpy.ops.mesh.primitive_cube_add(size=1,location=center);o=bpy.context.object;o.scale=size
        bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
        bevel=o.modifiers.new('Sewn soft panel','BEVEL');bevel.width=radius;bevel.segments=5
        bpy.ops.object.modifier_apply(modifier=bevel.name)
        return o

    def ribbon(points,width,thickness):
        vertices=[];faces=[]
        for p in points:
            vertices.extend([(p[0]-width/2,p[1],p[2]),(p[0]+width/2,p[1],p[2])])
        for i in range(len(points)-1):faces.append((i*2,i*2+1,i*2+3,i*2+2))
        o=mesh('Webbing',vertices,faces);solid=o.modifiers.new('Woven strap thickness','SOLIDIFY');solid.thickness=thickness
        bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=solid.name)
        return o

    def seam(points,radius):
        curve=bpy.data.curves.new('Stitch','CURVE');curve.dimensions='3D';curve.bevel_depth=radius;curve.bevel_resolution=2
        spline=curve.splines.new('POLY');spline.points.add(len(points)-1)
        for point,p in zip(spline.points,points):point.co=(*p,1)
        o=bpy.data.objects.new('Stitch',curve);bpy.context.collection.objects.link(o)
        bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o;bpy.ops.object.convert(target='MESH')
        return o

    pack_parts=[panel((0,.168,1.30),(.285,.135,.385),.035),panel((0,.24,1.245),(.225,.048,.19),.021)]
    for side in [-1,1]:
        # Contact along the upper back, over the shoulder and down the chest.
        path=[(side*.078,.13,1.46),(side*.112,.10,1.49),(side*.14,.045,1.49),
              (side*.151,-.032,1.455),(side*.148,-.095,1.375),(side*.132,-.096,1.23),
              (side*.105,-.040,1.13),(side*.085,.11,1.13)]
        pack_parts.append(ribbon(path,.038,.003))
        pack_parts.append(panel((side*.13,-.10,1.29),(.044,.008,.022),.003))
    pack_parts.append(ribbon([(-.04,.15,1.48),(-.035,.16,1.535),(.035,.16,1.535),(.04,.15,1.48)],.016,.003))
    pack_parts.append(seam([(-.11,.267,1.32),(-.108,.268,1.338),(0,.268,1.346),(.108,.268,1.338),(.11,.267,1.32)],.0015))
    for side in [-1,1]:
        pack_parts.append(seam([(side*.117,.266,1.18),(side*.119,.266,1.24),(side*.117,.266,1.31)],.0009))
    bpy.ops.object.select_all(action='DESELECT')
    for p in pack_parts:p.select_set(True)
    bpy.context.view_layer.objects.active=pack_parts[0];bpy.ops.object.join();pack=pack_parts[0];pack.name='Backpack'
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    # Normalize material slots after joining original garment-independent pieces.
    pack.data.materials.clear();pack.data.materials.append(material('Pack',(.05,.06,.065)))
    for p in pack.data.polygons:p.material_index=0;p.use_smooth=True
    bpy.ops.object.select_all(action='DESELECT');pack.select_set(True);bpy.context.view_layer.objects.active=pack
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(island_margin=.015);bpy.ops.object.mode_set(mode='OBJECT')
    # Bag panels follow the torso; straps follow the nearest shoulder/torso weights.
    positions=np.array([v.co[:] for v in pack.data.vertices]);nearest=np.array([tree.find(v.co)[1] for v in pack.data.vertices])
    bind(pack,rig,weights=(body,nearest));shapes(pack,positions,{k:d[nearest] for k,d in deltas.items() if k.startswith('Body_')})
    save_export(pack,rig,sex,'accessory')
    for o in bpy.data.objects:
        if o.type=='MESH' and o not in [body,shirt,trousers,shoes,hair,brows,pack]:o.hide_render=True
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/sex/'hero.blend'))


create('male')
(OUT/'geometry.json').write_text(json.dumps({'generator':'Scripts/assets/build_hero_models.py',
    'source_catalog':'Data/hero_source_catalog.json','license':'CC0-1.0 graphical derivation and original accessory',
    'assets':records},indent=2))
print('WTG_HERO_MODELS_PASS',len(records))
