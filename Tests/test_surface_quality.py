import hashlib
import json
import math
import struct
from pathlib import Path
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'Scripts'))
from make_surface_assets import generate, make_maps, periodic_noise
from surface_profiles import PROFILES, QUALITY


class SurfaceQuality(unittest.TestCase):
    def test_reproducible_maps(self):
        a,b=make_maps('Brick',64),make_maps('Brick',64)
        self.assertEqual(a,b)
        self.assertNotEqual(a['BC'],make_maps('Cobble',64)['BC'])

    def test_pbr_channels_and_nonmetal(self):
        for name,profile in PROFILES.items():
            maps=make_maps(name,64)
            self.assertEqual(set(maps),{'BC','N','ARM'})
            self.assertTrue(all(len(v)==64*64*3 for v in maps.values()))
            self.assertEqual(set(maps['ARM'][2::3]),{round(profile[2]*255)})
            self.assertGreater(max(maps['ARM'][1::3])-min(maps['ARM'][1::3]),1)
            self.assertGreaterEqual(min(maps['ARM'][1::3]),15)
            if name!='Metal':self.assertEqual(profile[2],0)

    def test_unit_normals_and_up_facing(self):
        for name in ('Brick','Asphalt','Wood','Skin','Fabric'):
            normals=make_maps(name,64)['N']
            for i in range(0,len(normals),3):
                xyz=[v/255*2-1 for v in normals[i:i+3]]
                self.assertAlmostEqual(math.sqrt(sum(v*v for v in xyz)),1,delta=.012)
                self.assertGreater(xyz[2],0)

    def test_periodic_noise_has_no_border_discontinuity(self):
        size=256
        data=periodic_noise(size,8,'seam')
        for i in range(size):
            self.assertLess(abs(data[i*size]-data[i*size+size-1]),.015)
            self.assertLess(abs(data[i]-data[(size-1)*size+i]),.015)

    def test_import_manifest_and_checksums(self):
        with tempfile.TemporaryDirectory() as directory:
            manifest=generate(directory,32,['Asphalt','Glass'])
            self.assertEqual(json.loads((Path(directory)/'manifest.json').read_text()),manifest)
            for profile in manifest['profiles'].values():
                for record in profile['files'].values():
                    data=(Path(directory)/record['file']).read_bytes()
                    self.assertEqual(len(data),18+32*32*3)
                    self.assertEqual(data[16],24)
                    self.assertEqual(hashlib.sha256(data).hexdigest(),record['sha256'])

    def test_resolution_guard_and_quality_budget(self):
        for size in (0,31,1000,8192):
            with self.assertRaises(ValueError):generate('unused',size)
        self.assertEqual(QUALITY['HERO']['exception_max'],8192)
        self.assertLessEqual(QUALITY['STANDARD']['max_resolution'],2048)
        self.assertTrue(all(p[3]>0 and p[4]>=0 for p in PROFILES.values()))

    def test_legacy_coverage_and_cook(self):
        from make_source_assets import MATERIALS
        self.assertTrue(set(MATERIALS).issubset(PROFILES))
        self.assertIn('/Game/SurfaceQuality',(ROOT/'Config/DefaultGame.ini').read_text())

    def test_cc0_provenance(self):
        sources=json.loads((ROOT/'Data/surface_sources.json').read_text())
        self.assertEqual(sources['license'],'CC0-1.0')
        self.assertEqual(len(sources['profiles']),6)
        for name,record in sources['profiles'].items():
            self.assertIn(name,PROFILES)
            self.assertTrue(record['authors'])
            self.assertGreater(record['tile_cm'],0)
            for item in record['files'].values():
                self.assertTrue(item['url'].startswith('https://dl.polyhaven.org/'))
                self.assertEqual(len(item['sha256']),64)
                path=ROOT/'Saved/SurfaceSource/PolyHaven'/item['file']
                if path.exists():
                    data=path.read_bytes()
                    self.assertEqual(hashlib.sha256(data).hexdigest(),item['sha256'])
                    self.assertEqual(struct.unpack('>II',data[16:24]),(2048,2048))


if __name__=='__main__':unittest.main()
