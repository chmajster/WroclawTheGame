import importlib.util,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/gps_router.py";S=importlib.util.spec_from_file_location("g",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def graph(self):return {"nodes":[{"id":"a"},{"id":"b"},{"id":"c"}],"edges":[{"id":"ab","from":"a","to":"b","length_cm":10,"car_forward":True,"car_backward":False,"foot":True,"name":"A"},{"id":"bc","from":"b","to":"c","length_cm":10,"car_forward":True,"car_backward":False,"foot":True,"name":"B"}]}
 def test_route_and_instructions(self):
  r=M.route(self.graph(),"a","c");self.assertEqual([x["id"] for x in r["edges"]],["ab","bc"]);self.assertEqual(M.instructions(r)[-1]["type"],"arrive")
 def test_turn_can_be_blocked(self):
  self.assertIsNone(M.route(self.graph(),"a","c",blocked_turns=[("ab","bc")]))
if __name__=="__main__":unittest.main()
