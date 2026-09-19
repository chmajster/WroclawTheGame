import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/benchmark/analyze_city_benchmark.py";S=importlib.util.spec_from_file_location("b",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def setUp(self):self.p=json.loads((R/"Data/benchmark_policy.json").read_text())
 def row(self,route="walk_nadodrze_centre",fps="60"):
  return {"route_id":route,"commit":"abc","build_config":"Development","hardware_id":"pc","fps":fps,"frame_ms":"16","cpu_ms":"10","gpu_ms":"11","ram_mb":"5000","vram_mb":"4000","streaming_misses":"0","draw_calls":"2000"}
 def test_good_passes_bad_fails(self):
  self.assertEqual(M.analyze([self.row()],self.p)["status"],"PASS");self.assertEqual(M.analyze([self.row(fps="20")],self.p)["status"],"FAIL")
 def test_route_aggregate_requires_all_routes_and_metadata(self):
  routes=json.loads((R/"Data/benchmark_routes.json").read_text());rows=[self.row(),self.row("drive_wave1_loop")];r=M.analyze_routes(rows,self.p,routes);self.assertEqual(r["status"],"PASS");self.assertEqual(set(r["routes"]),{"walk_nadodrze_centre","drive_wave1_loop"})
  missing=M.analyze_routes([self.row()],self.p,routes);self.assertEqual(missing["status"],"FAIL");self.assertIn("missing_route",{x["code"] for x in missing["issues"]})
 def test_real_baseline_regression(self):
  baseline={"frame_ms":{"p95":10},"fps":{"min":65}};r=M.analyze([self.row()],self.p,baseline);self.assertEqual(r["status"],"FAIL");self.assertGreater(len(r["regressions"]),0)
if __name__=="__main__":unittest.main()
