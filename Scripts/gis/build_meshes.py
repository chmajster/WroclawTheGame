"""Bake projected terrain, footprints and surfaces into spatially grouped mesh input.
The UE editor consumes these centimetre coordinates without OBJ axis conversion.
"""
import json, math
from pathlib import Path
from shapely.geometry import Polygon, box
from shapely.ops import triangulate
from import_sector import build, parts, OUTPUT, ROOT

def generate(destination, data=None, geo=None):
    if data is None or geo is None: data,geo=build()
    cells={};size=12800
    material=None
    def triangle(kind,points):
        center=[sum(p[i] for p in points)/3 for i in range(3)]
        key=(kind,math.floor(center[0]/size),math.floor(center[1]/size),material)
        record=cells.setdefault(key,{'kind':kind,'material':material,'origin':[key[1]*size,key[2]*size,0],'vertices':[],'triangles':[]})
        index=len(record['vertices']);record['vertices'] += [[round(p[i]-record['origin'][i],3) for i in range(3)] for p in points]
        record['triangles'] += [index,index+1,index+2]
    def face(kind,ring):
        if kind in ('road','rail'):ring=list(reversed(ring))
        triangle(kind,[ring[0],ring[1],ring[2]]);triangle(kind,[ring[0],ring[2],ring[3]])
    for feature in data['features']:
        kind=feature['kind']
        material=feature.get('material')
        if kind in ('building','green','water'):
            rings=feature['rings']; polygon=Polygon([p[:2] for p in rings[0]],[[p[:2] for p in r] for r in rings[1:]])
            base=max(p[2] for p in rings[0]);height=feature['height_m']*100 if kind=='building' else 4
            for tri in triangulate(polygon):
                if polygon.covers(tri):triangle(kind,[[x,y,base+height] for x,y in list(tri.exterior.coords)[:3]])
            if kind=='building':
                for ring in rings:
                    for a,b in zip(ring,ring[1:]):face(kind,[[a[0],a[1],min(a[2],base)-50],[b[0],b[1],min(b[2],base)-50],[b[0],b[1],base+height],[a[0],a[1],base+height]])
        elif kind in ('road','rail'):
            if not feature.get('surface_built',False) and (feature['tags'].get('bridge','no')!='no' or feature['tags'].get('tunnel','no')!='no'):continue
            half=feature.get('width_m',1.4)*50
            for a,b in zip(feature['points'],feature['points'][1:]):
                dx=b[0]-a[0];dy=b[1]-a[1];length=math.hypot(dx,dy)
                if length<1:continue
                ox=-dy/length*half;oy=dx/length*half
                face(kind,[[a[0]+ox,a[1]+oy,a[2]+8],[b[0]+ox,b[1]+oy,b[2]+8],[b[0]-ox,b[1]-oy,b[2]+8],[a[0]-ox,a[1]-oy,a[2]+8]])
    # Terrain samples are interpolation of the source raster, never invented hills.
    west,south,east,north=data['bbox']
    material=None
    width=math.dist(geo.world(west,south),geo.world(east,south))
    height=math.dist(geo.world(west,south),geo.world(west,north))
    columns=max(128,math.ceil(width/1200));rows=max(128,math.ceil(height/1200))
    for y in range(rows):
        for x in range(columns):
            face('terrain',[geo.world(west+(east-west)*u/columns,north-(north-south)*v/rows) for u,v in ((x,y),(x+1,y),(x+1,y+1),(x,y+1))])
    destination.mkdir(parents=True,exist_ok=True)
    meshes=[]
    for (kind,x,y,material),record in sorted(cells.items(),key=lambda item: str(item[0])):
        filename=f'{kind}_{x}_{y}'+(f'_{material}' if material else '')+'.json';(destination/filename).write_text(json.dumps(record,separators=(',',':')))
        meshes.append(filename)
    (destination/'meshes.json').write_text(json.dumps(meshes))
    return len(meshes)
if __name__=='__main__':
    print('Baked mesh cells:',generate(ROOT/'Saved/GISMeshes'))
    from make_routes import generate as make_routes
    make_routes()
