"""Shared continuous DEM surfaces and exact spatial cells (centimetres).

GIS remains authoritative. Surface priority removes coincident rail/road/path
layers; height sampling happens at XY vertices, including across road width.
"""
import math
from collections import defaultdict
from shapely.geometry import Polygon, LineString, box, GeometryCollection
from shapely.ops import triangulate, unary_union
from shapely import constrained_delaunay_triangles

CELL_CM = 12800
GRID_CM = 600
DRIVING = {'primary', 'secondary', 'tertiary', 'residential', 'service',
           'living_street', 'unclassified', 'primary_link', 'secondary_link', 'tertiary_link'}


def polygons(geometry):
    if geometry.geom_type == 'Polygon':
        yield geometry
    elif hasattr(geometry, 'geoms'):
        for part in geometry.geoms:
            yield from polygons(part)


def clip_triangle(points, bounds):
    """Clip a 3D polygon in XY, interpolating Z; also supports vertical walls."""
    output = list(points)
    for axis, boundary, greater in [(0, bounds[0], True), (0, bounds[2], False),
                                     (1, bounds[1], True), (1, bounds[3], False)]:
        old, output = output, []
        if not old:
            break
        previous = old[-1]
        previous_inside = previous[axis] >= boundary if greater else previous[axis] <= boundary
        for current in old:
            inside = current[axis] >= boundary if greater else current[axis] <= boundary
            if inside != previous_inside:
                t = (boundary - previous[axis]) / (current[axis] - previous[axis])
                output.append([previous[i] + t * (current[i] - previous[i]) for i in range(3)])
            if inside:
                output.append(current)
            previous, previous_inside = current, inside
    return output


def emit_triangle(cells, kind, material, points):
    for x in range(math.floor(min(p[0] for p in points) / CELL_CM), math.floor(max(p[0] for p in points) / CELL_CM) + 1):
        for y in range(math.floor(min(p[1] for p in points) / CELL_CM), math.floor(max(p[1] for p in points) / CELL_CM) + 1):
            ring = clip_triangle(points, (x * CELL_CM, y * CELL_CM, (x + 1) * CELL_CM, (y + 1) * CELL_CM))
            for i in range(1, len(ring) - 1):
                tri = [ring[0], ring[i], ring[i + 1]]
                a = [tri[1][j] - tri[0][j] for j in range(3)]
                b = [tri[2][j] - tri[0][j] for j in range(3)]
                cross = [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]
                if sum(v*v for v in cross) < .000001:
                    continue
                key = kind, x, y, material
                record = cells.setdefault(key, dict(kind=kind, material=material, origin=[x*CELL_CM, y*CELL_CM, 0], vertices=[], triangles=[]))
                index = len(record['vertices'])
                record['vertices'].extend([[round(p[j] - record['origin'][j], 3) for j in range(3)] for p in tri])
                record['triangles'].extend([index, index + 1, index + 2])


def emit_surface(cells, geometry, kind, material, height, grid=GRID_CM):
    """Bound tessellation length and evaluate the same DEM at shared XY points."""
    for polygon in polygons(geometry):
        if polygon.is_empty:
            continue
        west, south, east, north = polygon.bounds
        for x in range(math.floor(west/grid), math.ceil(east/grid)):
            for y in range(math.floor(south/grid), math.ceil(north/grid)):
                tile = polygon.intersection(box(x*grid, y*grid, (x+1)*grid, (y+1)*grid))
                for part in polygons(tile):
                    if part.area<.01:
                        continue
                    for triangle in constrained_delaunay_triangles(part).geoms:
                        points = [[px, py, height(px, py)] for px, py in list(triangle.exterior.coords)[:3]]
                        emit_triangle(cells, kind, material, points)


def surface_shapes(data):
    buildings, water, green, drive, rails = [], [], [], [], []
    road_materials = defaultdict(list)
    for feature in data['features']:
        kind = feature['kind']
        if kind in ('building', 'water', 'green'):
            rings = feature['rings']
            polygon = Polygon([p[:2] for p in rings[0]], [[p[:2] for p in ring] for ring in rings[1:]])
            {'building': buildings, 'water': water, 'green': green}[kind].append(polygon)
        elif kind in ('road', 'rail'):
            tags = feature['tags']
            if tags.get('tunnel', 'no') != 'no' or (tags.get('bridge', 'no') != 'no' and not feature.get('surface_built')):
                continue
            line = LineString([p[:2] for p in feature['points']])
            if kind == 'rail':
                # A tram's rail gauge is 1.435 m; a complete embedded bed is
                # wider than the old 1.4 m ribbon which left rail heads outside.
                rails.append(line.buffer(max(120, feature.get('width_m', 1.4)*50), cap_style=2, join_style=2))
            else:
                material = feature.get('material', 'Asphalt')
                road_materials[material].append(line.buffer(feature.get('width_m', 3)*50, cap_style=2, join_style=2))
                if tags.get('highway') in DRIVING and tags.get('bridge', 'no') == 'no' and tags.get('access') not in ('private', 'no'):
                    drive.append(line.buffer(feature.get('width_m', 3)*50 + 160, cap_style=2, join_style=2))
    building, wet = unary_union(buildings), unary_union(water)
    rail = unary_union(rails).difference(building)
    road = {}
    covered = rail
    # Vehicle surfaces have priority over crossing footway/path ribbons.
    for material in sorted(road_materials, key=lambda m: {'Asphalt': 0, 'Stone': 1, 'Concrete': 2}.get(m, 3)):
        shape = unary_union(road_materials[material]).difference(building).difference(covered)
        road[material] = shape
        covered = covered.union(shape)
    sidewalk = unary_union(drive).difference(covered).difference(building).difference(wet)
    grass = unary_union(green).difference(covered).difference(sidewalk).difference(building).difference(wet)
    return dict(building=building, water=wet, rail=rail, roads=road, sidewalk=sidewalk, green=grass,
                ground_covered=covered.union(sidewalk).union(grass).union(wet))


def elevation_sampler(geo):
    cache = {}
    def elevation(x, y):
        key = round(x, 5), round(y, 5)
        if key not in cache:
            lon, lat, _ = geo.geographic([x, y, 0])
            cache[key] = geo.world(lon, lat)[2]
        return cache[key]
    return elevation
