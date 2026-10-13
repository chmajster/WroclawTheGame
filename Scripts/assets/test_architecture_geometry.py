"""Portable contracts: source footprints, roof continuity and capped heights."""
import json
import sys
import unittest
from collections import Counter
from pathlib import Path
from shapely import constrained_delaunay_triangles
from shapely.geometry import Polygon
from architecture_geometry import roof_triangles

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'Scripts/gis'))
from start_architecture import select_buildings


class ArchitectureContracts(unittest.TestCase):
    def roof(self, polygon, flat=False):
        rings=[list(polygon.exterior.coords)]+[list(r.coords) for r in polygon.interiors]
        triangles=[list(t.exterior.coords)[:3] for t in constrained_delaunay_triangles(polygon).geoms]
        roof=list(roof_triangles(triangles,rings,8.,10.,flat))
        self.assertAlmostEqual(sum(Polygon([p[:2] for p in t]).area for t in roof),polygon.area,places=7)
        counts=Counter()
        for tri in roof:
            a,b,c=tri
            self.assertGreater((b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]),0)
            self.assertTrue(all(8<=p[2]<=10 for p in tri))
            for a,b in zip(tri,tri[1:]+tri[:1]):counts[tuple(sorted(tuple(round(x,7) for x in p) for p in (a,b)))]+=1
        for edge,count in counts.items():
            self.assertIn(count,(1,2))
            if count==1:
                midpoint=tuple((edge[0][i]+edge[1][i])/2 for i in range(2))
                from shapely.geometry import Point
                self.assertLess(polygon.boundary.distance(Point(midpoint)),1e-6,'open internal roof seam')
        return roof

    def test_hipped_concave_roof_has_no_cracks(self):
        self.roof(Polygon([(0,0),(18,0),(18,6),(7,6),(7,17),(0,17)]))

    def test_courtyard_stays_open(self):
        self.roof(Polygon([(0,0),(25,0),(25,20),(0,20)],holes=[[(5,5),(5,15),(18,15),(18,5)]]))

    def test_flat_tag_preserves_flat_roof(self):
        roof=self.roof(Polygon([(0,0),(12,0),(12,9),(0,9)]),True)
        self.assertEqual({p[2] for t in roof for p in t},{8.})

    def test_actual_selection_keeps_source_and_bounded_scope(self):
        policy=json.loads((ROOT/'Data/start_location.json').read_text(encoding='utf-8'))
        sector=json.loads((ROOT/'Saved/PlayableDistrict/sector.json').read_text(encoding='utf-8'))
        saved=json.loads((ROOT/'Data/start_location_buildings.json').read_text(encoding='utf-8'))
        selected=select_buildings(sector,policy)
        self.assertEqual(len({c['id'] for c in selected}),len(selected))
        self.assertEqual([c['id'] for c in selected],[c['id'] for c in saved['buildings']+saved['context_buildings']])
        self.assertLessEqual(len(saved['context_buildings']),policy['architecture_context']['max_buildings'])
        from shapely.geometry import Point
        start=Point(policy['player_anchor_cm'][:2]);features={c['id']:c for c in sector['features']}
        for c in saved['buildings']+saved['context_buildings']:
            feature=features[c['id']]
            restored=[[(p[0]*100+c['origin_cm'][0],p[1]*100+c['origin_cm'][1]) for p in ring] for ring in c['rings_m']]
            expected=Polygon([p[:2] for p in feature['rings'][0]],[[p[:2] for p in ring] for ring in feature['rings'][1:]])
            actual=Polygon(restored[0],restored[1:])
            self.assertLess(expected.hausdorff_distance(actual),.002)
            if c['context']:
                self.assertEqual(c['height_m'],feature['height_m'])
                self.assertLessEqual(expected.distance(start),policy['architecture_context']['radius_m']*100)


if __name__=='__main__':unittest.main()
