"""Original bpy production of CC0-derived, rigged creator meshes and real shape keys.

Uses only reviewed graphical MakeHuman data and the already-vendored Quaternius rig.
No upstream application/plugin code is imported. Run with Blender --background.
"""
from pathlib import Path
import math,json,hashlib
import bpy,numpy as np
from mathutils import Vector
from mathutils.kdtree import KDTree
ROOT=Path(__file__).resolve().parents[2]
MH=ROOT/'SourceAssets/Characters/MakeHuman/makehuman/data'
OUT=ROOT/'SourceAssets/Characters/WroclawCreator';OUT.mkdir(parents=True,exist_ok=True)
EXPORT=ROOT/'Saved/CreatorRebuild/Exports';EXPORT.mkdir(parents=True,exist_ok=True)
DEFS=json.loads((ROOT/'Data/creator_morphs.json').read_text(encoding='utf-8'))['morphs']
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
            try:mapping.append((np.array(t[:3],int),np.array(t[3:6],float)))
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
        if np.max(np.abs(d))<1e-8:continue
        key=o.shape_key_add(name=name);key.slider_min=-1;key.slider_max=1
        values=np.asarray(base+d,dtype=np.float32).ravel();key.data.foreach_set('co',values)

def save_export(o,rig,sex,kind):
    folder=OUT/sex;folder.mkdir(exist_ok=True)
    bpy.ops.object.select_all(action='DESELECT');o.select_set(True);rig.select_set(True);bpy.context.view_layer.objects.active=o
    fbx=EXPORT/f'{sex}_{o.name}.fbx'
    bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,
        bake_anim=False,use_mesh_modifiers=False,axis_forward='-Y',axis_up='Z')
    vertices=len(o.data.vertices);morphs=[]
    if o.data.shape_keys:
        base=np.array([v.co[:] for v in o.data.shape_keys.key_blocks[0].data])
        for k in o.data.shape_keys.key_blocks[1:]:
            delta=np.array([v.co[:] for v in k.data])-base
            morphs.append({'name':k.name,'changed_vertices':int(np.count_nonzero(np.linalg.norm(delta,axis=1)>1e-6)),'max_delta_cm':float(np.max(np.linalg.norm(delta,axis=1))*100)})
    records.append({'id':sex+'_'+o.name,'sex':sex,'kind':kind,'fbx':str(fbx.relative_to(ROOT)).replace('\\','/'),
        'vertices':vertices,'triangles':sum(len(p.vertices)-2 for p in o.data.polygons),'morphs':morphs,
        'materials':[m.name for m in o.data.materials],'source_blend':f'SourceAssets/Characters/WroclawCreator/{sex}/creator.blend'})

def create(sex):
    bpy.ops.wm.read_factory_settings(use_empty=True)
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
        # Preserve each source eye's high-quality topology and UV; carry proxy target deltas.
        delta=v-RAW
        return EYE+np.array([np.sum(delta[ids]*w[:,None],axis=0) for ids,w in mapping])
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
    for name,col in [('Face',(.55,.32,.22)),('Torso',(.55,.32,.22)),('Arms',(.55,.32,.22)),('Legs',(.55,.32,.22)),('Feet',(.55,.32,.22)),('Eyes',(.85,.87,.83))]:body.data.materials.append(material(name,col))
    for poly in body.data.polygons:
        center=np.mean(base[list(poly.vertices)],axis=0);z=center[2]
        poly.material_index=5 if min(poly.vertices)>=nv else (0 if z>1.5 else (4 if z<.13 else (3 if z<1.0 else (2 if abs(center[0])>.23 else 1))))
    colors=body.data.color_attributes.new(name='CreatorRegions',type='FLOAT_COLOR',domain='CORNER')
    for poly in body.data.polygons:
        for loop in poly.loop_indices:
            p=base[body.data.loops[loop].vertex_index];face=float(p[2]>1.52 and abs(p[0])<.1)
            lip=math.exp(-((p[2]-1.643)/.011)**2-((p[0])/.04)**2) if p[1]<-.05 else 0
            colors.data[loop].color=(face,lip,0,1)
    bind(body,rig);shapes(body,base,deltas);save_export(body,rig,sex,'body')
    # Fitted garment surfaces carry precisely the same per-vertex body deltas/weights.
    for name,low,high,sleeve,offset in [('TShirt01',1.015,1.475,.41,.012),('TShirt02',1.015,1.475,.56,.013),('Jacket01',1.015,1.485,.79,.022),('Jacket02',1.015,1.485,.79,.027),('Jeans01',.11,1.025,9,.018),('Jeans02',.11,1.025,9,.024),('Sneakers01',-.01,.13,9,.012),('Sneakers02',-.01,.16,9,.018)]:
        selected=[]
        for poly in body.data.polygons:
            c=np.mean(base[list(poly.vertices)],axis=0)
            if min(poly.vertices)<nv and low<c[2]<high and abs(c[0])<sleeve:
                if name.startswith('Jacket') and c[1]<-.025 and abs(c[0])<.03:continue
                selected.append(poly.index)
        ids=sorted({i for pi in selected for i in body.data.polygons[pi].vertices});remap={v:i for i,v in enumerate(ids)}
        b=base[ids]+np.array([body.data.vertices[i].normal[:] for i in ids])*offset
        o=mesh(name,b,[[remap[i] for i in body.data.polygons[pi].vertices] for pi in selected],
            [uvfaces[pi] for pi in selected],UV+EU)
        o.data.materials.append(material('Cloth' if not name.startswith('Sneakers') else 'Shoe',(.11,.14,.18)))
        bind(o,rig);shapes(o,b,{k:v[ids] for k,v in deltas.items() if k.startswith('Body_')});save_export(o,rig,sex,'clothing')
    headtree=KDTree(nv)
    for i,p in enumerate(base[:nv]):headtree.insert(Vector(p),i)
    headtree.balance()
    def headshapes(o):
        b=np.array([v.co[:] for v in o.data.vertices]);nearest=np.array([headtree.find(Vector(p))[1] for p in b]);shapes(o,b,{k:d[nearest] for k,d in deltas.items() if not k.startswith('Body_')});bind(o,rig,head=True)
    # Original sculpted hair shells, actual fibres, and distinct beard geometry.
    headcenter=np.array([0,.015,1.70])
    for name,length,start in [('HairShort',.095,.15),('HairCrop',.028,.06),('HairMedium',.16,.02),('BrowsNatural',.008,.0),('BrowsFine',.004,.0),('BrowsThick',.012,.0),('BeardStubble',.004,.0),('BeardShort',.02,.0),('BeardFull',.065,.0)]:
        vertices=[];fs=[]
        if name.startswith('Hair'):
            rings=14;segments=64
            for r in range(rings):
                phi=.08+(1.60 if name!='HairMedium' else 2.15)*r/(rings-1)
                for s in range(segments):
                    theta=2*math.pi*s/segments;wave=.003*math.sin(theta*11+r*.7)
                    vertices.append((math.sin(phi)*math.cos(theta)*(.092+wave),.015+math.sin(phi)*math.sin(theta)*(.092+wave),1.70+math.cos(phi)*(.102+length*.3)))
            for r in range(rings-1):
                for s in range(segments):a=r*segments+s;b=r*segments+(s+1)%segments;fs.append((a,b,b+segments,a+segments))
        else:
            for side in [-1,1]:
                center=np.array([side*.032,-.090,1.704]) if name.startswith('Brows') else np.array([side*.035,-.069,1.605])
                for strand in range(90 if name.startswith('Beard') else 35):
                    t=strand/(89 if name.startswith('Beard') else 34)
                    pos=center+np.array([side*(t-.5)*.057, .010*math.sin(t*math.pi), .01*math.sin(t*math.pi)])
                    width=.0008;end=pos+np.array([side*.0006,0,-length]);idx=len(vertices)
                    vertices.extend([pos+[-width,0,0],pos+[width,0,0],end+[width,0,0],end+[-width,0,0]]);fs.append((idx,idx+1,idx+2,idx+3))
        o=mesh(name,vertices,fs);o.data.materials.append(material('Hair',(.035,.018,.01)));headshapes(o);save_export(o,rig,sex,'hair' if name.startswith('Hair') else ('brows' if name.startswith('Brows') else 'beard'))
    # Accessories are complete authored meshes on the same rig.
    for name in ['Cap01','Glasses01','Backpack01']:
        if name=='Cap01':
            bpy.ops.mesh.primitive_uv_sphere_add(segments=40,ring_count=20,location=(0,.014,1.756));o=bpy.context.object;o.scale=(.098,.101,.046)
        elif name=='Glasses01':
            parts=[]
            for side in [-1,1]:
                bpy.ops.mesh.primitive_torus_add(major_radius=.024,minor_radius=.0018,major_segments=40,minor_segments=8,location=(side*.034,-.100,1.682),rotation=(math.pi/2,0,0));parts.append(bpy.context.object)
            bpy.ops.object.select_all(action='DESELECT')
            for p in parts:p.select_set(True)
            bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object
        else:
            bpy.ops.mesh.primitive_cube_add(size=1,location=(0,.165,1.285));o=bpy.context.object;o.scale=(.25,.11,.37)
            bevel=o.modifiers.new('Rounded seams','BEVEL');bevel.width=.08;bevel.segments=3;bpy.ops.object.modifier_apply(modifier=bevel.name)
        o.name=name;bpy.ops.object.transform_apply(location=True,rotation=True,scale=True);o.data.materials.append(material('Metal' if name=='Glasses01' else 'Cloth',(.08,.09,.1)))
        if name=='Backpack01':
            g=o.vertex_groups.new(name='spine_03');g.add(list(range(len(o.data.vertices))),1,'REPLACE');o.parent=rig;mod=o.modifiers.new('Rig','ARMATURE');mod.object=rig
        else:headshapes(o)
        save_export(o,rig,sex,'accessory')
    # Save editable project with only body displayed; exported parts remain inspectable.
    for o in bpy.data.objects:
        if o.type=='MESH' and o!=body:o.hide_viewport=True;o.hide_render=True
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/sex/'creator.blend'))

for sex in ['male','female']:create(sex)
(OUT/'geometry.json').write_text(json.dumps({'generator':'Scripts/assets/build_creator_models.py','license':'CC0-1.0 graphical derivation + original geometry','source_catalog':'Data/creator_source_catalog.json','assets':records},indent=2))
print('WTG_CREATOR_MODELS_PASS',len(records))
