"""Authoring contract: linear roughness/metallic; albedo is sRGB, lengths in cm.

These are original procedural starter surfaces, NOT scanned/photogrammetric assets.
Stable profile IDs are used by the importer, runtime replacement and tests.
"""
PROFILES = {
    # name: (sRGB albedo, dry roughness, metal, tile cm, relief cm, pattern, master)
    'Asphalt': ((55, 57, 59), .86, 0, 200, .12, 'aggregate', 'Road'),
    'Concrete': ((139, 137, 130), .82, 0, 200, .06, 'pores', 'Surface'),
    'Plaster': ((185, 177, 159), .83, 0, 200, .025, 'pores', 'Building'),
    'Brick': ((144, 83, 61), .79, 0, 200, .35, 'brick', 'Building'),
    'Cobble': ((119, 117, 110), .76, 0, 200, .55, 'cobble', 'Ground'),
    'Stone': ((170, 163, 145), .73, 0, 200, .08, 'aggregate', 'Building'),
    'Wood': ((122, 85, 52), .62, 0, 200, .045, 'grain', 'Wood'),
    'Metal': ((180, 183, 187), .38, 1, 100, .006, 'scratch', 'Metal'),
    'PaintedMetal': ((64, 79, 72), .46, 0, 100, .008, 'scratch', 'Metal'),
    'Glass': ((180, 193, 201), .12, 0, 200, .001, 'glass', 'Glass'),
    'Water': ((52, 68, 60), .10, 0, 400, .008, 'glass', 'Water'),
    'Fabric': ((93, 100, 87), .88, 0, 100, .01, 'weave', 'CharacterClothing'),
    'Denim': ((50, 68, 92), .87, 0, 100, .012, 'denim', 'CharacterClothing'),
    'Player': ((53, 76, 91), .86, 0, 100, .01, 'weave', 'CharacterClothing'),
    'Enemy': ((91, 36, 34), .86, 0, 100, .01, 'weave', 'CharacterClothing'),
    'Paper': ((217, 207, 182), .86, 0, 100, .002, 'pores', 'Surface'),
    'SignBlue': ((27, 64, 134), .44, 0, 100, .003, 'paint', 'Surface'),
    'SignRed': ((164, 36, 28), .45, 0, 100, .003, 'paint', 'Surface'),
    'RoadPaint': ((192, 188, 163), .68, 0, 100, .015, 'paint', 'Road'),
    'Soil': ((79, 64, 44), .94, 0, 200, .16, 'aggregate', 'Ground'),
    'PlasticMatte': ((83, 88, 86), .68, 0, 100, .006, 'pores', 'Surface'),
    'PlasticGloss': ((67, 71, 70), .25, 0, 100, .002, 'pores', 'Surface'),
    'Skin': ((163, 111, 86), .48, 0, 50, .002, 'pores', 'Skin'),
}

QUALITY = {
    'HERO': {'texels_per_meter': 1024, 'max_resolution': 4096, 'exception_max': 8192},
    'HIGH': {'texels_per_meter': 512, 'max_resolution': 4096},
    'STANDARD': {'texels_per_meter': 512, 'max_resolution': 2048},
    'DISTANT': {'texels_per_meter': 128, 'max_resolution': 1024},
}

# Legacy shader names remain readable; actual replacements are material instances.
LEGACY = {name: name for name in PROFILES}
