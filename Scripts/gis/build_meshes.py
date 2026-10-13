"""Bake projected terrain, footprints and surfaces into spatially grouped mesh input.
The UE editor consumes these centimetre coordinates without OBJ axis conversion.
"""
import json, math
from pathlib import Path
from shapely.geometry import Polygon, box
from shapely.ops import triangulate
from shapely.geometry.polygon import orient
from import_sector import build, parts, OUTPUT, ROOT
from surface_mesh import emit_triangle, emit_surface, surface_shapes, elevation_sampler

def generate(destination, data=None, geo=None, exclude_feature_ids=None):
    if data is None or geo is None: data,geo=build()
    excluded=set(exclude_feature_ids or ())
    cells={};size=12800
    shapes=surface_shapes(data);elevation=elevation_sampler(geo)
    material=None
    def triangle(kind,points):
        emit_triangle(cells,kind,material,points)
    def face(kind,ring):
        if kind in ('road','rail'):ring=list(reversed(ring))
        triangle(kind,[ring[0],ring[1],ring[2]]);triangle(kind,[ring[0],ring[2],ring[3]])
    for feature in data['features']:
        if feature.get('id') in excluded:continue
        kind=feature['kind']
        material=feature.get('material')
        if kind in ('building','water'):
            rings=feature['rings']; polygon=Polygon([p[:2] for p in rings[0]],[[p[:2] for p in r] for r in rings[1:]])
            # Source OSM rings may have either winding. Walls need one
            # outward mathematical normal before UE converts front winding.
            ordered=orient(polygon,sign=1)
            heights={(p[0],p[1]):p[2] for r in rings for p in r}
            rings=[[[x,y,heights.get((x,y),rings[0][0][2])] for x,y in r.coords]
                   for r in [ordered.exterior,*ordered.interiors]]
            base=max(p[2] for p in rings[0]);height=feature['height_m']*100 if kind=='building' else 4
            for tri in triangulate(polygon):
                if polygon.covers(tri):triangle(kind,[[x,y,base+height] for x,y in list(tri.exterior.coords)[:3]])
            if kind=='building':
                for ring in rings:
                    for a,b in zip(ring,ring[1:]):face(kind,[[a[0],a[1],min(a[2],base)-50],[b[0],b[1],min(b[2],base)-50],[b[0],b[1],base+height],[a[0],a[1],base+height]])
    emit_surface(cells,shapes['green'],'green',None,lambda x,y:elevation(x,y)+4)
    emit_surface(cells,shapes['rail'],'rail',None,lambda x,y:elevation(x,y)+8)
    for road_material,shape in shapes['roads'].items():
        emit_surface(cells,shape,'road',road_material,lambda x,y:elevation(x,y)+(20 if road_material=='Concrete' else 8))
    # Terrain samples are interpolation of the source raster, never invented hills.
    west,south,east,north=data['bbox']
    material=None
    width=math.dist(geo.world(west,south),geo.world(east,south))
    height=math.dist(geo.world(west,south),geo.world(west,north))
    columns=max(128,math.ceil(width/1200));rows=max(128,math.ceil(height/1200))
    for y in range(rows):
        for x in range(columns):
            ring=[geo.world(west+(east-west)*u/columns,north-(north-south)*v/rows) for u,v in ((x,y),(x+1,y),(x+1,y+1),(x,y+1))]
            tile=Polygon([p[:2] for p in ring]).difference(shapes['ground_covered'])
            emit_surface(cells,tile,'terrain',None,elevation)
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
