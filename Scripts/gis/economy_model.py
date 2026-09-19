#!/usr/bin/env python3
import argparse,copy,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CFG=ROOT/"Data/economy.json"

def apply(balance,amount,reason,cfg):
 amount=float(amount)
 if abs(amount)>cfg["limits"]["max_transaction_abs"]:raise ValueError("Transaction exceeds limit")
 new=round(float(balance)+amount,2)
 if new<cfg["limits"]["min_balance"]:raise ValueError("Insufficient credit limit")
 return {"previous":round(float(balance),2),"amount":round(amount,2),"balance":new,"reason":reason}

def price(cfg,key,quantity=1):
 if key not in cfg["prices"]:raise KeyError(key)
 return round(float(cfg["prices"][key])*float(quantity),2)

def new_wallet(cfg):
 return {"schema_version":2,"currency":cfg["currency"],"balance":round(float(cfg["starting_balance"]),2),"ledger":[],"applied_transaction_ids":[]}

def transact(wallet,transaction_id,amount,reason,cfg,metadata=None):
 if not transaction_id:raise ValueError("transaction_id required")
 if transaction_id in wallet.get("applied_transaction_ids",[]):return {**copy.deepcopy(wallet),"duplicate":True}
 result=apply(wallet["balance"],amount,reason,cfg);out=copy.deepcopy(wallet);out["balance"]=result["balance"];out.setdefault("ledger",[]).append({"id":transaction_id,"amount":result["amount"],"balance_after":result["balance"],"reason":reason,"metadata":metadata or {}});out.setdefault("applied_transaction_ids",[]).append(transaction_id);out["duplicate"]=False
 max_entries=int(cfg.get("ledger",{}).get("max_entries",1000))
 if len(out["ledger"])>max_entries:out["ledger"]=out["ledger"][-max_entries:]
 return out

def purchase(wallet,transaction_id,key,quantity,cfg,metadata=None):
 return transact(wallet,transaction_id,-price(cfg,key,quantity),key,cfg,metadata)

def reward(wallet,transaction_id,key,quantity,cfg,metadata=None):
 if key not in cfg["rewards"]:raise KeyError(key)
 return transact(wallet,transaction_id,float(cfg["rewards"][key])*float(quantity),key,cfg,metadata)

def fuel_cost(cfg,litres):return price(cfg,"fuel_per_litre",litres)

def service_cost(cfg,service_key):return price(cfg,service_key,1)

def main():
 p=argparse.ArgumentParser();p.add_argument("amount",type=float);p.add_argument("--balance",type=float,default=None);p.add_argument("--reason",default="manual");p.add_argument("--config",type=Path,default=CFG);a=p.parse_args();c=json.loads(a.config.read_text());print(json.dumps(apply(c["starting_balance"] if a.balance is None else a.balance,a.amount,a.reason,c)))
if __name__=="__main__":main()
