"""One bounded source selection for the GIS baker and production asset inputs."""
import hashlib
import re
from shapely.geometry import Point, Polygon


def select_buildings(data, policy):
    primary=[dict(item,context=False) for item in policy['buildings']]
    selected={item['id'] for item in primary}
    rule=policy.get('architecture_context',{})
    priority=set(rule.get('priority_ids',[]))
    centre=Point(policy['player_anchor_cm'][:2])
    candidates=[]
    for feature in data['features']:
        if feature['kind']!='building' or feature['id'] in selected:continue
        shape=Polygon([p[:2] for p in feature['rings'][0]],[[p[:2] for p in ring] for ring in feature['rings'][1:]])
        distance=shape.distance(centre)
        if distance>rule.get('radius_m',0)*100 or shape.area<80*10000 or feature['height_m']<8:continue
        candidates.append((0 if feature['id'] in priority else 1,distance,feature['id'],feature))
    palette=[(.74,.70,.63),(.68,.66,.59),(.77,.75,.68),(.65,.69,.65)]
    context=[]
    for _,_,identifier,feature in sorted(candidates)[:rule.get('max_buildings',0)]:
        try:floors=int(float(feature['tags'].get('building:levels',round(feature['height_m']/3.2))))
        except ValueError:floors=round(feature['height_m']/3.2)
        colour=palette[int(hashlib.sha256(identifier.encode()).hexdigest()[:8],16)%len(palette)]
        context.append(dict(id=identifier,asset='context-'+re.sub('[^a-z0-9]+','-',identifier.lower()).strip('-'),
            height_m=feature['height_m'],floors=max(2,floors),tint=list(colour),style='plain',context=True,
            accuracy='Exact source footprint/height; interpreted restrained facade and roof, not a surveyed replica'))
    return primary+context
