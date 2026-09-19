import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/benchmark/analyze_city_benchmark.py";S=importlib.util.spec_from_file_location("b",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def test_good_passes_bad_fails(self):
  p=json.loads((R/"Data/benchmark_policy.json").read_text());good=[{"fps":"60","frame_ms":"16","cpu_ms":"10","gpu_ms":"11","ram_mb":"5000","vram_mb":"4000","streaming_misses":"0","draw_calls":"2000"}];bad=[{**good[0],"fps":"20"}];self.assertEqual(M.analyze(good,p)["status"],"PASS");self.assertEqual(M.analyze(bad,p)["status"],"FAIL")
if __name__=="__main__":unittest.main()
