"""Original metre-scale window frame; source OBJ Y up, UE normal +X."""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

def generate():
    vertices=[];faces=[]
    def box(center,size):
        start=len(vertices)+1
        for x,y,z in ((-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)):
            vertices.append(tuple(center[i]+size[i]*v/2 for i,v in enumerate((x,y,z))))
        for a,b,c,d in ((0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)):
            faces.extend(((start+a,start+b,start+c),(start+a,start+c,start+d)))
    for y in (-.58,.58):box((0,y,.75),(.09,.07,1.5))
    for z in (.035,1.465):box((0,0,z),(.09,1.2,.07))
    box((0,0,.75),(.09,.04,1.36))
    box((0,0,.75),(.09,1.06,.04))
    box((.04,0,.01),(.24,1.3,.06))
    # Solid thin glazing has two renderable sides and one distinct material slot.
    start=len(faces);box((-.025,0,.75),(.008,1.06,1.36))
    target=ROOT/'SourceAssets/Models/Wroclaw/window_frame.obj'
    target.parent.mkdir(parents=True,exist_ok=True)
    target.with_suffix('.mtl').write_text('newmtl Frame\nKd 0.2 0.23 0.24\nnewmtl Glazing\nKd 0.08 0.15 0.2\n',encoding='utf-8')
    target.write_text('# WTG original window frame; metres, Y up\nmtllib window_frame.mtl\no window_frame\n'
      +''.join('v %.6f %.6f %.6f\n'%(x,z,-y) for x,y,z in vertices)
      +'usemtl Frame\n'+''.join('f %d %d %d\n'%f for f in faces[:start])
      +'usemtl Glazing\n'+''.join('f %d %d %d\n'%f for f in faces[start:]),encoding='utf-8')
    print(target)

if __name__=='__main__':generate()
