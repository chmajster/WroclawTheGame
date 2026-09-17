"""Readiness derives from imported geometry and evidence, never an authored status label."""
import hashlib
import json

STATUSES = ('Missing', 'GISOnly', 'Blockout', 'Playable', 'Detailed', 'Final')
COLORS = ('#737373', '#378bd4', '#e68a32', '#e3c848', '#51ad67', '#ad75df')
PLAYABLE_CHECKS = ('real_layout', 'collision', 'pedestrian_navigation', 'traffic', 'population',
                   'ambient', 'gameplay_location', 'activity', 'world_events', 'secret',
                   'save_load', 'streaming', 'performance')


def fingerprint(catalog, sources):
    return hashlib.sha256(json.dumps({'catalog': catalog, 'sources': sources},
                                     sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def evaluate(catalog, stats, connectivity, digest, evidence=None):
    evidence = evidence or {}
    if evidence and (evidence.get('schema_version') != 1 or evidence.get('fingerprint') != digest):
        raise ValueError('Stale or unsupported city verification evidence')
    sectors = []; district = {d['id']: d for d in catalog['districts']}
    for sector in catalog['sectors']:
        sid = sector['id']; facts = stats[sid]
        checks = evidence.get('sectors', {}).get(sid, {})
        # A non-empty evidence reference is required in addition to a true result.
        def passed(key):
            item = checks.get(key, {})
            return isinstance(item, dict) and item.get('passed') is True and isinstance(item.get('evidence'), str) and bool(item['evidence'].strip())
        imported = facts['buildings'] > 0 and facts['roads'] > 0
        status = 1 if imported else 0
        if imported and passed('engine_bake'):
            status = 2
        blockers = [key for key in PLAYABLE_CHECKS if not passed(key)]
        for mode in ('car', 'foot'):
            if not connectivity.get(sid, {}).get(mode, False):
                blockers.append(mode+'_connectivity')
        if not imported: blockers.insert(0, 'source_geometry')
        if not passed('engine_bake'): blockers.insert(0, 'engine_bake')
        if status == 2 and not blockers:
            status = 3
            if passed('manual_detail'): status = 4
            if status == 4 and passed('final_acceptance'): status = 5
        sectors.append({'id': sid, 'district': sector['district'], 'name': district[sector['district']]['name'],
                        'wave': district[sector['district']]['wave'], 'status': STATUSES[status],
                        'color': COLORS[status], 'blockers': blockers, 'counts': facts,
                        'checks': {key: passed(key) for key in PLAYABLE_CHECKS}})
    total = len(sectors)
    def ratio(predicate):
        return round(sum(bool(predicate(s)) for s in sectors)/total, 6) if total else 0
    return {'schema_version': 1, 'fingerprint': digest, 'TotalSectors': total,
            'PlayableSectors': sum(STATUSES.index(s['status']) >= 3 for s in sectors),
            'DetailedSectors': sum(STATUSES.index(s['status']) >= 4 for s in sectors),
            'RoadCoverage': ratio(lambda s: s['counts']['roads'] > 0),
            'BuildingCoverage': ratio(lambda s: s['counts']['buildings'] > 0),
            'PedestrianCoverage': ratio(lambda s: s['checks']['pedestrian_navigation']),
            'TrafficCoverage': ratio(lambda s: s['checks']['traffic']),
            'QuestCoverage': ratio(lambda s: s['checks']['activity']),
            'sectors': sectors,
            'metric_note': 'Coverage is a fraction of registered sectors, not a percentage of Wroclaw area. Road/Building are GIS presence; other fields require engine evidence.'}
