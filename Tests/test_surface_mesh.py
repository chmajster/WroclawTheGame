"""Geometric contracts: coverage, holes, heights and actual cell boundaries."""
import json
import math
import sys
import unittest
from pathlib import Path
from shapely.geometry import Polygon

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'Scripts/gis'))
from surface_mesh import emit_surface, emit_triangle, CELL_CM


class SurfaceMeshTests(unittest.TestCase):
    def test_covered_area_and_hole_are_preserved_on_a_sloping_surface(self):
        shape=Polygon([(-500,-200),(13000,-200),(13500,2000),(700,3200)],
                      [[(1000,500),(1600,500),(1600,1000),(1000,1000)]])
        cells={};height=lambda x,y:100+x*.02-y*.01
        emit_surface(cells,shape,'sidewalk','Concrete',height)
        area=0
        for record in cells.values():
            vertices=[[p[i]+record['origin'][i] for i in range(3)] for p in record['vertices']]
            for i in range(0,len(record['triangles']),3):
                points=[vertices[j] for j in record['triangles'][i:i+3]]
                a,b,c=points
                self.assertGreater((b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]),0)
                triangle=Polygon([p[:2] for p in points]);area+=triangle.area
                self.assertLess(triangle.difference(shape).area,.5)
                for x,y,z in points:self.assertAlmostEqual(z,height(x,y),delta=.002)
        self.assertAlmostEqual(area,shape.area,delta=10)

    def test_vertical_wall_crossing_cells_keeps_area_and_winding(self):
        cells={};emit_triangle(cells,'building','Brick',[[-500,20,0],[26000,20,0],[26000,20,2000]])
        area=0
        for record in cells.values():
            v=record['vertices']
            for i in range(0,len(v),3):
                a,b,c=v[i:i+3];signed=(b[0]-a[0])*(c[2]-a[2])-(b[2]-a[2])*(c[0]-a[0])
                self.assertGreater(signed,0);area+=signed/2
        # Production coordinates are quantized to 0.001 cm after clipping.
        self.assertAlmostEqual(area,26500*2000/2,delta=10)

    def test_actual_generated_cells_have_finite_indices_and_bounded_xy(self):
        folder=ROOT/'Saved/PlayableDistrict/Meshes'
        for name in json.loads((folder/'meshes.json').read_text()):
            record=json.loads((folder/name).read_text())
            for point in record['vertices']:
                self.assertTrue(all(math.isfinite(v) for v in point))
                for v in point[:2]:self.assertGreaterEqual(v,-.001);self.assertLessEqual(v,CELL_CM+.001)
            for index in record['triangles']:self.assertTrue(0<=index<len(record['vertices']))
            if record['kind'] in ['road','rail','sidewalk','green','terrain']:
                v=record['vertices']
                for i in range(0,len(record['triangles']),3):
                    a,b,c=[v[index] for index in record['triangles'][i:i+3]]
                    self.assertGreater((b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]),0)

    def test_production_rail_profiles_are_flush_and_have_source_gauge(self):
        plan=json.loads((ROOT/'Saved/StartLocation/placement.json').read_text())
        rails=[item for item in plan['items'] if item['role']=='embedded-tram-rail']
        self.assertGreater(len(rails),20)
        for item in rails:
            self.assertTrue(item['source'].startswith(('way/','relation/')))
            self.assertEqual(item['scale'][1:],[1,1])
            self.assertLessEqual(item['scale'][0],2.02)
            self.assertTrue(100<item['gauge_cm']<170)
            self.assertLess(abs(item['pitch_deg']),20)


if __name__=='__main__':unittest.main()
