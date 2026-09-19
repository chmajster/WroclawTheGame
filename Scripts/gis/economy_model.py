#!/usr/bin/env python3
import argparse,json
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
def main():
 p=argparse.ArgumentParser();p.add_argument("amount",type=float);p.add_argument("--balance",type=float,default=None);p.add_argument("--reason",default="manual");p.add_argument("--config",type=Path,default=CFG);a=p.parse_args();c=json.loads(a.config.read_text());print(json.dumps(apply(c["starting_balance"] if a.balance is None else a.balance,a.amount,a.reason,c)))
if __name__=="__main__":main()
