import copy
import json
import math
import random
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'Scripts/gis'))
from build_city import validate_catalog, owner, generate
from city_coverage import PLAYABLE_CHECKS, evaluate, fingerprint
from city_routing import HierarchicalRouter
from road_routes import route, adjacency
from import_sector import Geography
from city_structures import apply_structures


def graph(edges, size=6):
    return {'nodes': [{'id': str(i)} for i in range(size)],
            'edges': [{'id': str(i), 'from': str(a), 'to': str(b), 'length_cm': cost,
                       'car_forward': True, 'car_backward': False, 'foot': True,
                       'bridge': 'no', 'layer': '0'} for i,(a,b,cost) in enumerate(edges)]}


class CityRoutingTests(unittest.TestCase):
    def test_portal_path_can_leave_and_reenter_same_sector(self):
        data = graph([(0,1,10), (1,2,1), (2,3,1), (3,4,1), (4,5,1), (0,5,100)])
        owners = {str(n): ('a' if n in (0,1,4,5) else 'b') for n in range(6)}
        result = HierarchicalRouter(data, owners).route('0','5')
        self.assertEqual(result['nodes'], ['0','1','2','3','4','5'])
        self.assertEqual(result['sectors'], ['a','b','a'])
        self.assertEqual(result['length_cm'], 14)
        with self.assertRaises(ValueError): HierarchicalRouter(data, owners).route('5','0')
        self.assertEqual(HierarchicalRouter(data, owners, 'foot').route('5','0')['length_cm'], 14)

    def test_matches_global_shortest_paths_on_directed_networks(self):
        rng = random.Random(510917)
        for _ in range(12):
            data = graph([(a,b,rng.randint(1,30)) for a in range(18) for b in range(18)
                          if a != b and rng.random() < .12], 18)
            owners = {str(n): f'sector{n//3}' for n in range(18)}
            router = HierarchicalRouter(data, owners)
            links = adjacency(data)
            for start, end in [('0','17'), ('4','9'), ('7','8'), ('3','3')]:
                try: expected = route(data, start, end)
                except ValueError:
                    with self.assertRaises(ValueError): router.route(start, end)
                    continue
                result = router.route(start, end)
                cost = sum(min(length for target,length,_ in links[a] if target == b)
                           for a,b in zip(expected, expected[1:]))
                self.assertEqual(result['length_cm'], cost)
                for a,b in zip(result['nodes'],result['nodes'][1:]):
                    self.assertIn(b, {edge[0] for edge in links[a]})

    def test_unreviewed_tunnels_and_bridges_are_not_routable(self):
        for tag in ('tunnel', 'bridge'):
            data = graph([(0,1,1)], 2); data['edges'][0][tag] = 'yes'
            with self.assertRaises(ValueError):
                HierarchicalRouter(data, {'0':'a','1':'b'}).route('0','1')
        with self.assertRaises(ValueError): adjacency(data, 'tram')
        with self.assertRaises(ValueError): HierarchicalRouter(data, {'0':'a'})


class CityCoverageTests(unittest.TestCase):
    def setUp(self):
        self.catalog = json.loads((ROOT/'Data/city.json').read_text())
        self.stats = {s['id']: {'buildings':2,'roads':3} for s in self.catalog['sectors']}
        self.links = {sid: {'car':True,'foot':True} for sid in self.stats}
        self.digest = fingerprint(self.catalog, [])
        self.evidence = {'schema_version':1,'fingerprint':self.digest,'sectors':{sid:{
            key:{'passed':True,'evidence':'Saved/verified-engine-run.json'}
            for key in (*PLAYABLE_CHECKS,'engine_bake')} for sid in self.stats}}

    def test_geometry_alone_never_claims_playable(self):
        report = evaluate(self.catalog, self.stats, self.links, self.digest)
        self.assertEqual(report['PlayableSectors'], 0)
        self.assertTrue(all(s['status'] == 'GISOnly' for s in report['sectors']))
        self.assertEqual(report['PedestrianCoverage'], 0)
        self.assertEqual(report['TrafficCoverage'], 0)
        self.assertEqual(report['RoadCoverage'], 1)

    def test_every_readiness_gate_is_required(self):
        sid = 'sector.huby'
        self.assertEqual(evaluate(self.catalog,self.stats,self.links,self.digest,self.evidence)['PlayableSectors'], 6)
        for key in (*PLAYABLE_CHECKS,'engine_bake'):
            evidence = copy.deepcopy(self.evidence); evidence['sectors'][sid][key]['passed'] = False
            report = evaluate(self.catalog,self.stats,self.links,self.digest,evidence)
            self.assertEqual(report['PlayableSectors'], 5, key)
        self.links[sid]['car'] = False
        self.assertEqual(evaluate(self.catalog,self.stats,self.links,self.digest,self.evidence)['PlayableSectors'], 5)

    def test_stale_missing_and_invalid_evidence(self):
        with self.assertRaises(ValueError): evaluate(self.catalog,self.stats,self.links,'changed',self.evidence)
        self.evidence['sectors']['sector.huby']['save_load']['evidence'] = None
        report = evaluate(self.catalog,self.stats,self.links,self.digest,self.evidence)
        self.assertEqual(report['PlayableSectors'], 5)
        self.stats['sector.huby']['buildings'] = 0
        report = evaluate(self.catalog,self.stats,self.links,self.digest)
        self.assertEqual(next(s for s in report['sectors'] if s['id']=='sector.huby')['status'], 'Missing')

    def test_catalogue_identity_and_boundaries(self):
        validate_catalog(self.catalog)
        self.assertEqual(owner(self.catalog,17.04,51.09),'sector.huby')
        reordered = copy.deepcopy(self.catalog); reordered['sectors'].reverse()
        self.assertEqual(owner(self.catalog,17.025,51.083), owner(reordered,17.025,51.083))
        for field, value in [('id','sector.nadodrze'), ('district','invalid'), ('bbox',[17,51,17,52])]:
            changed = copy.deepcopy(self.catalog); changed['sectors'][2][field] = value
            with self.assertRaises(ValueError): validate_catalog(changed)

    def test_expanding_source_bounds_preserves_world_origin(self):
        terrain = ROOT/'Data/source/wroclaw/terrain/nadodrze.csv'
        original = Geography([17.025,51.118,17.038,51.126], terrain)
        city = Geography([16.995,51.065,17.06,51.126], terrain, original.origin)
        self.assertEqual(original.world(17.03,51.12,115), city.world(17.03,51.12,115))


class WaveOneSourceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.data, cls.geo, cls.report = generate(Path(cls.tmp.name))
        cls.routing = json.loads((Path(cls.tmp.name)/'routing.json').read_text())

    @classmethod
    def tearDownClass(cls): cls.tmp.cleanup()

    def test_real_sources_and_four_southern_sectors(self):
        self.assertEqual(self.data['origin_projected_m'], (641702,5665787,115.0))
        self.assertEqual(len([s for s in self.report['sectors'] if s['wave']==1]), 4)
        for s in self.report['sectors']:
            self.assertGreater(s['counts']['buildings'], 20)
            self.assertGreater(s['counts']['roads'], 20)
            self.assertEqual(s['status'], 'GISOnly')
        names = {e['name'] for e in self.data['road_graph']['edges']}
        self.assertTrue({'Powstańców Śląskich','Hubska','Borowska'} <= names)
        self.assertEqual(self.report['PlayableSectors'], 0)

    def test_all_sectors_connected_with_authored_bridge_decks(self):
        self.assertTrue(all(modes['car'] and modes['foot'] for modes in self.routing['connectivity'].values()))
        self.assertEqual(len(self.data['authored_structures']),6)
        self.assertTrue(all(not s['engine_verified'] and not s['surveyed'] for s in self.data['authored_structures']))

    def test_city_activities_interiors_and_directed_population_loops(self):
        content=json.loads((Path(self.tmp.name)/'content.json').read_text())
        self.assertEqual(len(content['activities']),32)
        self.assertEqual(len(content['interiors']),4)
        self.assertEqual(len(content['population_routes']),8)
        for loop in content['population_routes']:
            links=adjacency(self.data['road_graph'],'car' if loop['vehicle'] else 'foot')
            self.assertEqual(loop['source_nodes'][0],loop['source_nodes'][-1])
            for a,b in zip(loop['source_nodes'],loop['source_nodes'][1:]):
                self.assertIn(b,{e[0] for e in links[a]})
        self.assertEqual(len({a['id'] for a in content['activities']}),32)
        self.assertTrue(all(i['building'] for i in content['interiors']))

    def test_topology_addresses_and_courtyards(self):
        self.assertEqual(len(self.routing['owners']),len(self.data['road_graph']['nodes']))
        addresses = json.loads((Path(self.tmp.name)/'addresses.json').read_text())
        buildings = {f['id'] for f in self.data['features'] if f['kind']=='building'}
        streets = {s['id'] for s in json.loads((Path(self.tmp.name)/'streets.json').read_text())}
        self.assertGreater(len(addresses),100)
        for address in addresses:
            self.assertIn(address['building_id'],buildings)
            self.assertIn(address['street_id'],streets)
        for courtyard in json.loads((Path(self.tmp.name)/'courtyards.json').read_text()):
            self.assertIn(courtyard['building_id'], buildings)
            self.assertFalse(courtyard['accessible'])


if __name__ == '__main__': unittest.main()


class AuthoredBridgeTests(unittest.TestCase):
    def test_deck_and_graph_share_height_and_bad_grades_fail(self):
        data={'features':[{'id':'way/1:line:0','tags':{'bridge':'yes'},'points':[[0,0,100],[1000,0,-500],[2000,0,200]]}],
              'road_graph':{'nodes':[{'id':str(i),'position':p[:]} for i,p in enumerate([[0,0,100],[1000,0,-500],[2000,0,200]])],
              'edges':[{'way':'1','from':'0','to':'1'},{'way':'1','from':'1','to':'2'}]}}
        definition={'schema_version':1,'bridges':[{'way':'1','name':'test','max_grade':.1}]}
        apply_structures(data,definition)
        self.assertEqual(data['features'][0]['points'][1][2],150)
        self.assertEqual(data['road_graph']['nodes'][1]['position'][2],150)
        self.assertTrue(all(e['surface_built'] for e in data['road_graph']['edges']))
        bad=copy.deepcopy(data);bad['features'][0]['points'][-1][2]=10000
        with self.assertRaises(ValueError):apply_structures(bad,definition)
