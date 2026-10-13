"""Original leaf silhouettes/veins and bark, generated deterministically.

No third-party photographs. Four atlas cells: two heart-shaped lime leaves
and two lobed maple leaves. Public source redistribution permitted by authorship.
"""
import json
import hashlib
from pathlib import Path
import numpy as np
from PIL import Image

ROOT=Path(__file__).resolve().parents[2]


def build():
    out=ROOT/'SourceAssets/Textures/Wroclaw/Foliage';out.mkdir(parents=True,exist_ok=True)
    n=512;y,x=np.mgrid[0:n,0:n];u=(x%256+.5)/256;v=(y%256+.5)/256
    xx=(u-.5)*2;yy=(v-.5)*2;angle=np.arctan2(xx,yy);radius=np.hypot(xx,yy)
    lime=(xx/(.69*(1-.28*yy)))**2+((yy+.1)/.82)**2<1
    lime&=(yy<.72-.26*np.exp(-(xx/.16)**2))
    maple=radius<(.68+.16*np.cos(5*angle)+.045*np.cos(10*angle))
    inside=np.where(y<256,lime,maple)&(u>.03)&(u<.97)&(v>.03)&(v<.97)
    central=np.exp(-(xx/.017)**2)
    veins=np.exp(-((np.abs(xx)*.6+yy*1.1+.08)% .23-.115)**2/.00015)
    ridge=central*.014+veins*.007
    base=np.zeros((n,n,4),dtype=np.uint8)
    tone=1+((x//256)*.1)-central*.10-veins*.06+yy*.08
    for c,val in enumerate([68,106,32]):base[:,:,c]=np.clip(val*tone,0,255)
    base[:,:,3]=inside*255
    # Color extends through transparent pixels to avoid black mip borders.
    Image.fromarray(base).save(out/'leaf_base_color.png')
    gy,gx=np.gradient(ridge);normal=np.stack((-gx*18,gy*18,np.ones_like(gx)),axis=-1)
    normal/=np.linalg.norm(normal,axis=-1)[:,:,None]
    Image.fromarray(np.uint8((normal*.5+.5)*255)).save(out/'leaf_normal.png')
    bark=.5+.16*np.sin(x*.16+np.sin(y*.035))+.09*np.sin(x*.73+y*.006)
    rgb=np.stack([bark*98,bark*83,bark*66],axis=-1).astype('uint8')
    Image.fromarray(rgb).save(out/'bark_base_color.png')
    gy,gx=np.gradient(bark*.025);bn=np.stack((-gx*8,gy*8,np.ones_like(gx)),axis=-1);bn/=np.linalg.norm(bn,axis=-1)[:,:,None]
    Image.fromarray(np.uint8((bn*.5+.5)*255)).save(out/'bark_normal.png')
    report={'author':'WroclawTheGame original procedural authorship','license':'Project-owned original texture; source and game redistribution permitted',
            'resolution':512,'normal_convention':'DirectX','files':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.glob('*.png')}}
    (out/'provenance.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('FOLIAGE_TEXTURES_PASS')


if __name__=='__main__':build()
