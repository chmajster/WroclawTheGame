"""Project-owned, metre-scale railing source for the existing asset pipeline."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def generate():
    vertices, faces = [], []

    def beam(center, size):
        first = len(vertices) + 1
        for x, y, z in ((-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),
                        (-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)):
            vertices.append(tuple(center[i] + size[i] * v / 2 for i,v in enumerate((x,y,z))))
        for a,b,c,d in ((0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)):
            faces.extend(((first+a,first+b,first+c),(first+a,first+c,first+d)))

    for x in (-1.2,0,1.2):
        beam((x,0,.55),(.05,.05,1.1))
        beam((x,0,.025),(.14,.14,.05))
    for z in (.28,.64,1.1):
        beam((0,0,z),(2.45,.045,.045))
    output = ROOT/'SourceAssets/Models/Wroclaw/river_railing.obj'
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text('# WTG original river railing; metres, OBJ Y up\no river_railing\n'
                      + ''.join('v %.6f %.6f %.6f\n' % (x,z,-y) for x,y,z in vertices)
                      + ''.join('f %d %d %d\n' % f for f in faces),encoding='utf-8')
    print(f'{output}: {len(vertices)} vertices, {len(faces)} triangles')


if __name__ == '__main__':
    generate()
