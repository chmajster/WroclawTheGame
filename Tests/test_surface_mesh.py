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

    def test_raised_slab_has_outward_edges_reaching_the_ground(self):
        cells={};shape=Polygon([(0,0),(1200,0),(1200,1200),(0,1200)])
        floor=lambda x,y:x*.02-y*.01
        emit_surface(cells,shape,'sidewalk','Concrete',lambda x,y:floor(x,y)+20,
                     skirt_bottom=lambda x,y:floor(x,y)-3)
        wall_area=0
        for record in cells.values():
            vertices=record['vertices']
            for i in range(0,len(vertices),3):
                a,b,c=vertices[i:i+3]
                normal=[(b[(k+1)%3]-a[(k+1)%3])*(c[(k+2)%3]-a[(k+2)%3])-(b[(k+2)%3]-a[(k+2)%3])*(c[(k+1)%3]-a[(k+1)%3]) for k in range(3)]
                if abs(normal[2])>.001:continue
                wall_area+=math.sqrt(sum(v*v for v in normal))*.5
                center=[sum(p[k] for p in [a,b,c])/3 for k in range(2)]
                self.assertGreater(normal[0]*(center[0]-600)+normal[1]*(center[1]-600),0)
                offsets=[p[2]-floor(*p[:2]) for p in [a,b,c]]
                self.assertAlmostEqual(min(offsets),-3);self.assertAlmostEqual(max(offsets),20)
        self.assertAlmostEqual(wall_area,4800*23)

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
                    normal_z=(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
                    self.assertGreaterEqual(normal_z,0) # Vertical edge risers have Z=0.
                    self.assertGreater(sum(((b[(k+1)%3]-a[(k+1)%3])*(c[(k+2)%3]-a[(k+2)%3])-(b[(k+2)%3]-a[(k+2)%3])*(c[(k+1)%3]-a[(k+1)%3]))**2 for k in range(3)),0)

    def test_production_rail_profiles_are_flush_and_have_source_gauge(self):
        plan=json.loads((ROOT/'Saved/StartLocation/placement.json').read_text())
        for item in plan['items']:
            if item['role']=='curb':self.assertLessEqual(item['scale'][0],1)
        rails=[item for item in plan['items'] if item['role']=='embedded-tram-rail']
        self.assertGreater(len(rails),20)
        for item in rails:
            self.assertTrue(item['source'].startswith(('way/','relation/')))
            self.assertEqual(item['scale'][1:],[1,1])
            self.assertLessEqual(item['scale'][0],2.02)
            self.assertTrue(100<item['gauge_cm']<170)
            self.assertLess(abs(item['pitch_deg']),20)


if __name__=='__main__':unittest.main()
