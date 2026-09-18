"""Deterministic, original placeholder textures and PCM audio; Python standard library only."""
from pathlib import Path
import math
import random
import struct
import wave

MATERIALS = {
    'Wood': ((105, 73, 47), .72, 0), 'Plaster': ((161, 153, 133), .92, 0),
    'Concrete': ((112, 116, 114), .88, 0), 'Cobble': ((102, 104, 102), .9, 0),
    'Asphalt': ((43, 46, 49), .96, 0), 'Brick': ((121, 73, 54), .92, 0),
    'Stone': ((178, 172, 155), .82, 0), 'Metal': ((68, 82, 78), .45, .75),
    'Glass': ((31, 48, 62), .13, .25), 'Paper': ((223, 214, 186), .92, 0),
    'Fabric': ((91, 99, 84), .98, 0), 'Player': ((53, 76, 91), .85, 0),
    'Enemy': ((91, 36, 34), .9, 0), 'SignBlue': ((24, 65, 151), .45, .2),
    'SignRed': ((185, 28, 24), .5, .1),
}
LOOPS = {'Apartment', 'Street', 'Chase'}
SOUNDS = {'Footstep': .24, 'Door': .85, 'Drawer': .6, 'Pickup': .26, 'Switch': .12,
          'Horn': .75, 'Hit': .2, 'Alarm': .8, 'UIHover': .055, 'UIClick': .09,
          'Apartment': 8, 'Street': 12, 'Chase': 4}


def make_texture(path, name, color):
    rng = random.Random(name)
    size = 256
    pixels = bytearray()
    for y in range(size):
        for x in range(size):
            noise = rng.uniform(-6, 6)
            if name == 'Wood':
                noise += 11 * math.sin(x * .22 + 2 * math.sin(y * .025))
            if name in {'Brick', 'Cobble'}:
                offset = 32 if (y // 32) % 2 else 0
                if y % 32 < 3 or (x + offset) % 64 < 3:
                    noise += 35
            if name == 'Asphalt':
                noise *= 1.8
            rgb = [max(0, min(255, round(c + noise))) for c in color]
            pixels.extend(reversed(rgb))
    header = struct.pack('<BBBHHBHHHHBB', 0, 0, 2, 0, 0, 0, 0, 0, size, size, 24, 32)
    path.write_bytes(header + pixels)


def make_sound(path, name, duration):
    rate = 22050
    rng = random.Random(name)
    pcm = bytearray()
    low = 0.
    for i in range(round(rate * duration)):
        t = i / rate
        low = .97 * low + .03 * rng.uniform(-1, 1)
        noise = rng.uniform(-1, 1)
        if name == 'Footstep':
            sample = (noise * .35 + math.sin(2 * math.pi * 90 * t) * .5) * math.exp(-t * 24)
        elif name in {'Door', 'Drawer'}:
            sample = (low * 2 + math.sin(2 * math.pi * (110 * t + 25 * t * t)) * .08) * math.sin(math.pi * t / duration)
        elif name == 'Pickup':
            sample = .25 * math.sin(2 * math.pi * 880 * t) * math.exp(-t * 14)
        elif name == 'Switch':
            sample = noise * .45 * math.exp(-t * 70)
        elif name == 'UIHover':
            sample = (.13 * math.sin(2 * math.pi * 980 * t) +
                      .05 * math.sin(2 * math.pi * 1470 * t)) * math.exp(-t * 58)
        elif name == 'UIClick':
            sample = (.2 * math.sin(2 * math.pi * 720 * t) +
                      .08 * math.sin(2 * math.pi * 1440 * t) +
                      noise * .025) * math.exp(-t * 36)
        elif name == 'Hit':
            sample = (noise * .35 + math.sin(2 * math.pi * 65 * t) * .4) * math.exp(-t * 22)
        elif name == 'Horn':
            sample = .22 * math.sin(2 * math.pi * 440 * t) + .16 * math.sin(2 * math.pi * 550 * t)
        elif name == 'Alarm':
            sample = .3 * math.sin(2 * math.pi * (780 if int(t * 8) % 2 else 520) * t)
        elif name == 'Apartment':
            sample = .08 * math.sin(2 * math.pi * 50 * t) + .15 * low
        elif name == 'Street':
            # Traffic rumble and a distant tram-like bell, all synthesized.
            bell = math.exp(-(t % 6) * 2) * (.07 * math.sin(2 * math.pi * 880 * t) + .03 * math.sin(2 * math.pi * 1320 * t))
            sample = low * .7 + .09 * math.sin(2 * math.pi * 42 * t) + bell
        else:
            beat = math.exp(-(t % .5) * 18)
            sample = .28 * math.sin(2 * math.pi * 70 * t) * beat + .06 * math.sin(2 * math.pi * 140 * t)
        # Seam fade prevents pops on loop boundaries and import previews.
        fade = min(1., t / .012, (duration - t) / .025)
        pcm += struct.pack('<h', int(max(-1, min(1, sample * fade)) * 26000))
    with wave.open(str(path), 'wb') as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(rate)
        output.writeframes(pcm)


def generate(directory):
    directory = Path(directory)
    directory.mkdir(parents=True, exist_ok=True)
    for name, (color, _, _) in MATERIALS.items():
        make_texture(directory / f'T_{name}.tga', name, color)
    for name, duration in SOUNDS.items():
        make_sound(directory / f'{name}.wav', name, duration)
    return directory


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, default=Path(__file__).resolve().parents[1] / 'Saved/SourceAssets')
    args = parser.parse_args()
    generate(args.output)
    print(f'Generated {len(MATERIALS)} textures and {len(SOUNDS)} sounds in {args.output}')
