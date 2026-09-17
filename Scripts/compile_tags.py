import json,argparse
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def generate():
 c=json.loads((ROOT/'Data/chapter1.json').read_text());w=json.loads((ROOT/'Data/openworld.json').read_text());tags=set()
 for a in c['actions']:
  for k in ('tags','set_tags','require_tags'):tags.update(a[k])
 for i in c['items']:tags.update(i['tags'])
 tags.update(['Event.Action.Completed','Event.Inventory.Used','Event.Noise','Event.AI.Alert','Event.Alarm','Event.World.Encounter','Event.Location.Discovered','State.Player.Hidden','State.Player.InCombat','State.World.Alert'])
 for n in w['noise']:tags.add('Noise.'+n['id'])
 return '[/Script/GameplayTags.GameplayTagsSettings]\nImportTagsFromConfig=True\n'+''.join('+GameplayTagList=(Tag="'+t+'",DevComment="")\n' for t in sorted(tags))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--check',action='store_true');a=p.parse_args();out=ROOT/'Config/DefaultGameplayTags.ini';text=generate()
 if a.check:
  if not out.exists() or out.read_text()!=text:raise SystemExit('Gameplay Tags are stale')
 else:out.write_text(text,encoding='utf-8')
