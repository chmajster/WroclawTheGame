"""Geometry and reachability checks on the actual generated district source."""
import json
import math
import sys
import unittest
from pathlib import Path
from shapely.geometry import LineString, Point, Polygon
from shapely.ops import unary_union

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'Scripts/gis'))
from build_playable_district import route_between, attach_car_anchor


class DistrictTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.output = ROOT / 'Saved/PlayableDistrict'
        cls.plan = json.loads((cls.output / 'district.json').read_text(encoding='utf-8'))
        cls.sector = json.loads((cls.output / 'sector.json').read_text(encoding='utf-8'))
        cls.obstacles = unary_union([
            Polygon([p[:2] for p in f['rings'][0]], [[p[:2] for p in r] for r in f['rings'][1:]])
            for f in cls.sector['features'] if f['kind'] in ('building', 'water')])

    def test_anchors_clear_buildings_water_and_boundary(self):
        for p in self.plan['anchors'].values():
            self.assertFalse(self.obstacles.buffer(35).covers(Point(p[:2])))
            for axis in (0, 1):
                self.assertLessEqual(self.plan['bounds_min'][axis], p[axis])
                self.assertLessEqual(p[axis], self.plan['bounds_max'][axis])

    def test_pedestrians_do_not_cross_buildings_or_water(self):
        for route in self.plan['population_routes']:
            if not route['vehicle']:
                self.assertFalse(LineString([p[:2] for p in route['points']]).intersects(self.obstacles))

    def test_mesh_indices_finite_geometry_and_budget(self):
        triangles = 0
        for name in json.loads((self.output / 'Meshes/meshes.json').read_text()):
            mesh = json.loads((self.output / 'Meshes' / name).read_text())
            self.assertEqual(len(mesh['triangles']) % 3, 0)
            self.assertTrue(all(0 <= i < len(mesh['vertices']) for i in mesh['triangles']))
            self.assertTrue(all(math.isfinite(x) for p in mesh['vertices'] for x in p))
            triangles += len(mesh['triangles']) // 3
        self.assertLessEqual(triangles, self.plan['budgets']['mesh_triangles'])

    def test_dressing_clearance(self):
        self.assertGreater(len(self.plan['dressing']), 10)
        walks = unary_union([LineString([p[:2] for p in r['points']]).buffer(45)
                             for r in self.plan['population_routes'] if not r['vehicle']])
        for item in self.plan['dressing']:
            footprint = Point(item['position'][:2]).buffer(item['radius'])
            self.assertFalse(footprint.intersects(self.obstacles))
            self.assertFalse(footprint.intersects(walks))

    def test_parked_vehicle_faces_legal_exit(self):
        a=self.plan['anchors']['vehicle'];b=self.plan['delivery_route'][0]
        if math.dist(a[:2],b[:2])<150:
            b=self.plan['delivery_route'][1]
        direction=math.atan2(b[1]-a[1],b[0]-a[0])
        self.assertGreater(math.cos(direction-math.radians(self.plan['vehicle_yaw'])),.95)
        yaw=math.radians(self.plan['vehicle_yaw']);c,s=math.cos(yaw),math.sin(yaw)
        footprint=Polygon([(a[0]+x*c-y*s,a[1]+x*s+y*c) for x,y in ((-210,-90),(-210,90),(210,90),(210,-90))])
        self.assertFalse(footprint.intersects(self.obstacles))

    def test_interior_road_anchor_respects_one_way(self):
        graph={'a':[('b',1000)],'b':[]};nodes={'a':[0,0,0],'b':[1000,0,0]}
        edges=[{'from':'a','to':'b','bridge':'no','tunnel':'no','layer':'0','car_forward':True,'car_backward':False}]
        key=attach_car_anchor(graph,nodes,edges,[500,100,0],lambda x,y:0)
        self.assertEqual(route_between(graph,key,'b'),[key,'b'])
        self.assertEqual(route_between(graph,'a',key),['a',key])
        with self.assertRaises(ValueError):route_between(graph,key,'a')
        with self.assertRaises(ValueError):route_between(graph,'b',key)

    def test_dijkstra_respects_direction_and_disconnected_destinations(self):
        graph = {'a': [('b', 10), ('c', 50)], 'b': [('c', 10)], 'c': []}
        self.assertEqual(route_between(graph, 'a', 'c'), ['a', 'b', 'c'])
        with self.assertRaises(ValueError):
            route_between(graph, 'c', 'a')

    def test_traffic_loop_closes_and_mission_has_travel(self):
        routes = [r for r in self.plan['population_routes'] if r['vehicle']]
        self.assertTrue(routes)
        for route in routes:
            self.assertEqual(route['points'][0], route['points'][-1])
        path = self.plan['delivery_route']
        self.assertGreater(sum(math.dist(a, b) for a, b in zip(path, path[1:])), 25000)
        self.assertFalse(LineString([p[:2] for p in path]).buffer(90).intersects(self.obstacles))


if __name__ == '__main__':
    unittest.main()
