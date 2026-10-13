"""Metre-space roof geometry shared by Blender and portable source checks."""
import math


def segment_distance(point, a, b):
    dx, dy = b[0]-a[0], b[1]-a[1]
    denominator = dx*dx+dy*dy
    t = max(0., min(1., ((point[0]-a[0])*dx+(point[1]-a[1])*dy)/denominator)) if denominator else 0.
    return math.hypot(point[0]-a[0]-t*dx, point[1]-a[1]-t*dy)


def roof_height(point, rings, base, maximum, flat=False):
    if flat:
        return base
    distance = min(segment_distance(point, a, b) for ring in rings for a, b in zip(ring, ring[1:]))
    return min(maximum, base+distance*.8)


def roof_triangles(triangles, rings, base, maximum, flat=False):
    # A common subdivision count gives every shared tessellation edge identical
    # vertices. Independently choosing counts per triangle would open cracks.
    longest = max(math.dist(a[:2], b[:2]) for triangle in triangles for a, b in zip(triangle, triangle[1:]+triangle[:1]))
    steps = 1 if flat else max(1, math.ceil(longest/1.5))
    def vertex(a, b, c, i, j):
        p = tuple(a[k]+(b[k]-a[k])*i/steps+(c[k]-a[k])*j/steps for k in range(2))
        return (*p, roof_height(p, rings, base, maximum, flat))
    for a, b, c in triangles:
        if (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]) < 0:
            b, c = c, b
        for i in range(steps):
            for j in range(steps-i):
                yield [vertex(a,b,c,i,j), vertex(a,b,c,i+1,j), vertex(a,b,c,i,j+1)]
                if i+j+1 < steps:
                    yield [vertex(a,b,c,i+1,j), vertex(a,b,c,i+1,j+1), vertex(a,b,c,i,j+1)]
