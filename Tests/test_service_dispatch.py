import importlib.util,json,sys,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R/"Scripts/gis"));P=R/"Scripts/gis/service_dispatch.py";S=importlib.util.spec_from_file_location("d",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def city(self):return {"road_graph":{"nodes":[{"id":"a","position":[0,0,0]},{"id":"b","position":[1000,0,0]},{"id":"c","position":[2000,0,0]}],"edges":[{"id":"ab","from":"a","to":"b","length_cm":1000,"foot":True,"car_forward":True,"car_backward":True,"bridge":"no","layer":"0","tunnel":"no","surface_built":True},{"id":"bc","from":"b","to":"c","length_cm":1000,"foot":True,"car_forward":True,"car_backward":True,"bridge":"no","layer":"0","tunnel":"no","surface_built":True}]}}
 def cfg(self):return json.loads((R/"Data/emergency_response_policy.json").read_text())
 def test_nearest_reachable_depot_has_eta_and_units(self):
  r=M.dispatch(self.city(),[{"id":"p","service":"police","position":[0,0,0],"available_units":5}],{"id":"i","position":[2000,0,0],"heat":50},"police",self.cfg());self.assertEqual(r["status"],"dispatched");self.assertEqual(r["depot_id"],"p");self.assertEqual(r["route_nodes"],["a","b","c"]);self.assertEqual(r["units"],2);self.assertGreater(r["eta_seconds"],0);self.assertTrue(r["traffic_yield"])
 def test_incident_policy_and_unavailable_state(self):
  r=M.dispatch(self.city(),[],{"id":"i","position":[0,0,0],"type":"injury"},"ambulance",self.cfg());self.assertEqual(r["status"],"unavailable")
  rejected=M.dispatch(self.city(),[],{"id":"i","position":[0,0,0],"type":"fire"},"ambulance",self.cfg());self.assertEqual(rejected["status"],"rejected")
if __name__=="__main__":unittest.main()
