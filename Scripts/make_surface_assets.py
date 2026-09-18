"""Reproducible tileable PBR data without baked lighting; no external dependencies.

Normals are derived from physical relief, not from albedo. ARM = AO/Roughness/Metallic.
Only this generator writes its own Saved/SurfaceSource products.
"""
from array import array
import hashlib
import json
import math
from pathlib import Path
import random
import struct
from surface_profiles import PROFILES, QUALITY


def byte(value):
    return max(0, min(255, round(value)))


def tga(path, size, rgb):
    header = struct.pack('<BBBHHBHHHHBB', 0, 0, 2, 0, 0, 0, 0, 0, size, size, 24, 32)
    bgr = bytearray(len(rgb))
    bgr[0::3], bgr[1::3], bgr[2::3] = rgb[2::3], rgb[1::3], rgb[0::3]
    path.write_bytes(header + bgr)


def periodic_noise(size, cells, seed):
    rng = random.Random(seed)
    grid = [rng.random() for _ in range(cells * cells)]
    coordinates = []
    for i in range(size):
        v = i * cells / size
        a = int(v)
        t = v - a
        coordinates.append((a % cells, (a + 1) % cells, t*t*(3-2*t)))
    out = array('f')
    for ya, yb, ty in coordinates:
        for xa, xb, tx in coordinates:
            a = grid[ya*cells+xa]*(1-tx)+grid[ya*cells+xb]*tx
            b = grid[yb*cells+xa]*(1-tx)+grid[yb*cells+xb]*tx
            out.append(a*(1-ty)+b*ty)
    return out


def make_maps(name, size):
    color, rough, metal, tile, relief, pattern, _ = PROFILES[name]
    fine = periodic_noise(size, max(4, size//3), name+'fine')
    middle = periodic_noise(size, 32, name+'middle')
    macro = periodic_noise(size, 8, name+'macro')
    albedo, arm = bytearray(), bytearray()
    heights = array('f')
    # Separate per-brick reflectance from mortar relief: never paint directional shadows.
    rng = random.Random(name)
    stone_colors = [rng.uniform(.82, 1.12) for _ in range(256)]
    for y in range(size):
        for x in range(size):
            i = y*size+x
            f, m, a = fine[i], middle[i], macro[i]
            value = 1 + (f-.5)*.10 + (m-.5)*.09 + (a-.5)*.08
            h, cavity = (f-.5)*relief, 1.0
            r = rough + (f-.5)*.14 + (a-.5)*.08
            c = color
            if pattern in ('brick', 'cobble'):
                rows, columns = (24, 8) if pattern == 'brick' else (12, 12)
                yy = y*rows/size
                row = int(yy)
                xx = x*columns/size + .5*(row % 2)
                fx, fy = xx % 1, yy % 1
                edge = min(fx, 1-fx, fy*(columns/rows), (1-fy)*(columns/rows))
                grout = .024 if pattern == 'brick' else .038
                shaped = max(0, min(1, (edge-grout)*30))
                h += relief * shaped
                cavity = .78 + .22*shaped
                value *= stone_colors[(row*columns+int(xx)) % 256]
                if edge < grout:
                    c = (117, 113, 103) if pattern == 'brick' else (71, 68, 59)
                    r = .94
            elif pattern == 'grain':
                grain = math.sin(2*math.pi*(x/size*48 + .22*math.sin(y/size*2*math.pi)))
                value += grain*.10
                h += grain*relief*.4
                r += grain*.035
            elif pattern in ('weave', 'denim'):
                weave = math.sin(x*2*math.pi/4)*math.sin(y*2*math.pi/4)
                if pattern == 'denim':
                    weave = math.sin((x+y)*2*math.pi/6)
                h += weave*relief*.5
                value += weave*.035
            elif pattern == 'scratch':
                line = math.sin(y/size*2*math.pi*128)
                h += line*relief*.15
                r += line*.025
            elif pattern == 'aggregate':
                value += (f-.5)*.20
                cavity = .92 + .08*f
            elif pattern == 'glass':
                value = 1
                r = rough + a*.035
            heights.append(h)
            albedo.extend(byte(channel*value) for channel in c)
            arm.extend((byte(cavity*255), byte(max(.06, min(.98, r))*255), byte(metal*255)))
    normal = bytearray()
    pixel_cm = tile / size
    for y in range(size):
        for x in range(size):
            dx = (heights[y*size+(x+1)%size]-heights[y*size+(x-1)%size])/(2*pixel_cm)
            dy = (heights[((y+1)%size)*size+x]-heights[((y-1)%size)*size+x])/(2*pixel_cm)
            length = math.sqrt(dx*dx+dy*dy+1)
            # DirectX normal convention; TGA top-left origin.
            normal.extend((byte((.5-dx/length*.5)*255), byte((.5+dy/length*.5)*255), byte((.5+.5/length)*255)))
    return {'BC': albedo, 'N': normal, 'ARM': arm}


def generate(directory, size=1024, names=None):
    if size < 32 or size > 4096 or size & (size-1):
        raise ValueError('Resolution must be a power of two in [32, 4096]')
    directory = Path(directory)
    directory.mkdir(parents=True, exist_ok=True)
    manifest = {'schema_version': 1, 'provenance': 'Original deterministic procedural surfaces; not scans',
                'normal_convention': 'DirectX', 'packed_channels': 'R=AO G=Roughness B=Metallic',
                'quality_classes': QUALITY, 'profiles': {}}
    for name in names or PROFILES:
        record = {'resolution': size, 'tile_cm': PROFILES[name][3], 'files': {}}
        for suffix, pixels in make_maps(name, size).items():
            path = directory / f'T_{name}_{suffix}.tga'
            tga(path, size, pixels)
            record['files'][suffix] = {'file': path.name, 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
        manifest['profiles'][name] = record
    (directory/'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    return manifest


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, default=Path(__file__).resolve().parents[1]/'Saved/SurfaceSource')
    parser.add_argument('--size', type=int, default=1024)
    args = parser.parse_args()
    generate(args.output, args.size)
