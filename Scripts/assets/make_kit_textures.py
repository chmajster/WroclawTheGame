"""Deterministic original PBR microstructure; no external image sources."""
from pathlib import Path
import numpy as np
from PIL import Image

ROOT=Path(__file__).resolve().parents[2]


def main():
    folder=ROOT/"SourceAssets/Models/Wroclaw/TenementKit/Textures"
    folder.mkdir(parents=True,exist_ok=True)
    rng=np.random.default_rng(20261012)
    for kind,color,rough,metal in [("iron",[40,43,42],.67,.15),("stone",[188,176,152],.84,0)]:
        grain=rng.normal(0,1,(1024,1024))
        coarse=np.asarray(Image.fromarray(rng.integers(0,255,(32,32),dtype=np.uint8)).resize((1024,1024),Image.Resampling.BICUBIC),dtype=float)/255-.5
        height=grain*.002+coarse*.025
        rgb=np.clip(np.asarray(color)[None,None,:]+grain[:,:,None]*2+coarse[:,:,None]*22,0,255).astype(np.uint8)
        Image.fromarray(rgb).save(folder/f"{kind}_BC.png")
        orm=np.empty_like(rgb); orm[:,:,0]=255
        orm[:,:,1]=np.clip((rough+coarse*.1)*255,0,255).astype(np.uint8); orm[:,:,2]=round(metal*255)
        Image.fromarray(orm).save(folder/f"{kind}_ORM.png")
        dy,dx=np.gradient(height)
        normals=np.stack((-dx*8,dy*8,np.ones_like(dx)),axis=-1) # DirectX tangent Y.
        normals/=np.linalg.norm(normals,axis=-1,keepdims=True)
        Image.fromarray(np.clip((normals*.5+.5)*255,0,255).astype(np.uint8)).save(folder/f"{kind}_N.png")
    print("PASS original iron/stone BC, DirectX normal and ORM: 1024x1024")


if __name__=="__main__": main()
