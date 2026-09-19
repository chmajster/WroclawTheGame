import importlib.util,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/service_dispatch.py";S=importlib.util.spec_from_file_location("d",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def test_nearest_reachable_depot(self):
  city={"road_graph":{"nodes":[{"id":"a","position":[0,0,0]},{"id":"b","position":[10,0,0]},{"id":"c","position":[20,0,0]}],"edges":[{"id":"ab","from":"a","to":"b","length_cm":10,"foot":True,"car_forward":True,"car_backward":True,"bridge":"no","layer":"0","tunnel":"no","surface_built":True},{"id":"bc","from":"b","to":"c","length_cm":10,"foot":True,"car_forward":True,"car_backward":True,"bridge":"no","layer":"0","tunnel":"no","surface_built":True}]}}
  r=M.dispatch(city,[{"id":"p","service":"police","position":[0,0,0]}],{"id":"i","position":[20,0,0]},"police");self.assertEqual(r["depot_id"],"p");self.assertEqual(r["route_nodes"],["a","b","c"])
if __name__=="__main__":unittest.main()
