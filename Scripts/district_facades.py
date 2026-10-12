"""Fit imported opening bounds to source GIS walls, including offset pivots."""
import math


def opening_pose(item, origin, extent):
    width_y = extent[1] >= extent[0]
    outward = float(item.get('yaw_deg', 0))
    yaw = outward if width_y else outward - 90
    width_axis = 1 if width_y else 0
    normal_axis = 1 - width_axis
    scale = [1., 1., item.get('height_m', 2) * 100 / max(2 * extent[2], 1)]
    scale[width_axis] = item.get('width_m', 1) * 100 / max(2 * extent[width_axis], 1)
    scale[normal_axis] = min(1., 18 / max(2 * extent[normal_axis], 1))
    p = list(item['world_position'])
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    x, y = origin[0] * scale[0], origin[1] * scale[1]
    p[0] += math.cos(math.radians(outward)) * 20 - (c * x - s * y)
    p[1] += math.sin(math.radians(outward)) * 20 - (s * x + c * y)
    p[2] -= origin[2] * scale[2]
    return p, yaw, scale
