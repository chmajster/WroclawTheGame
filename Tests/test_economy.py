import importlib.util,json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[1];P=R/"Scripts/gis/economy_model.py";S=importlib.util.spec_from_file_location("e",P);M=importlib.util.module_from_spec(S);S.loader.exec_module(M)
class T(unittest.TestCase):
 def setUp(self):self.c=json.loads((R/"Data/economy.json").read_text())
 def test_transaction(self):self.assertEqual(M.apply(100,-35,"medkit",self.c)["balance"],65)
 def test_price_quantity(self):self.assertEqual(M.price(self.c,"fuel_per_litre",10),65)
 def test_credit_limit(self):
  with self.assertRaises(ValueError):M.apply(-1990,-20,"x",self.c)
 def test_wallet_ledger_and_idempotency(self):
  w=M.new_wallet(self.c);a=M.purchase(w,"buy-1","medkit",1,self.c);b=M.purchase(a,"buy-1","medkit",1,self.c);self.assertEqual(a["balance"],215);self.assertEqual(b["balance"],215);self.assertTrue(b["duplicate"]);self.assertEqual(len(b["ledger"]),1)
 def test_reward_and_integration_costs(self):
  w=M.reward(M.new_wallet(self.c),"quest-1","minor_quest",1,self.c);self.assertEqual(w["balance"],400);self.assertEqual(M.fuel_cost(self.c,10),65);self.assertEqual(M.service_cost(self.c,"tow_release"),350)
 def test_runtime_subsystem_exists(self):
  h=(R/"Source/WroclawTheGame/Systems/WTGEconomySubsystem.h").read_text();cpp=(R/"Source/WroclawTheGame/Systems/WTGEconomySubsystem.cpp").read_text();self.assertIn("UGameInstanceSubsystem",h);self.assertIn("ApplyTransaction",h);self.assertIn("ExportSaveState",h);self.assertIn("AppliedTransactionIds.Contains",cpp)
if __name__=="__main__":unittest.main()
