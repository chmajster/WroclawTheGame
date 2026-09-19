import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/build_data_layer_plan.py";S=importlib.util.spec_from_file_location("d",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def test_base_and_building_layers(self):
  c=json.loads((R/"Data/city_data_layers.json").read_text());self.assertEqual(M.lookup(c,kind="road"),"DL_BaseGeometry");self.assertEqual(M.lookup(c,kind="building"),"DL_Buildings");self.assertEqual(M.lookup(c,system="interior"),"DL_Interiors")
if __name__=="__main__":unittest.main()
