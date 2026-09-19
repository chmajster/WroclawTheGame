import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/economy_model.py";S=importlib.util.spec_from_file_location("e",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def setUp(self):self.c=json.loads((R/"Data/economy.json").read_text())
 def test_transaction(self):self.assertEqual(M.apply(100,-35,"medkit",self.c)["balance"],65)
 def test_price_quantity(self):self.assertEqual(M.price(self.c,"fuel_per_litre",10),65)
 def test_credit_limit(self):
  with self.assertRaises(ValueError):M.apply(-1990,-20,"x",self.c)
if __name__=="__main__":unittest.main()
