import importlib.util
from pathlib import Path
import tempfile
import unittest
import wave
import struct

SPEC = importlib.util.spec_from_file_location('assets', Path(__file__).resolve().parents[1] / 'Scripts/make_source_assets.py')
assets = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(assets)


class Assets(unittest.TestCase):
    def test_valid_audio_and_texture_outputs(self):
        with tempfile.TemporaryDirectory() as temp:
            path = assets.generate(temp)
            for name, duration in assets.SOUNDS.items():
                with wave.open(str(path / f'{name}.wav')) as sound:
                    self.assertEqual((sound.getnchannels(), sound.getsampwidth(), sound.getframerate()), (1, 2, 22050))
                    self.assertAlmostEqual(sound.getnframes() / 22050, duration, places=4)
                    samples = struct.unpack('<' + 'h' * sound.getnframes(), sound.readframes(sound.getnframes()))
                    self.assertGreater(max(samples) - min(samples), 100)
                    self.assertLess(max(abs(x) for x in samples), 32767)
            for name in assets.MATERIALS:
                data = (path / f'T_{name}.tga').read_bytes()
                self.assertEqual(len(data), 18 + 256 * 256 * 3)
                self.assertEqual(data[2], 2)
                self.assertEqual(data[16], 24)


if __name__ == '__main__':
    unittest.main()
