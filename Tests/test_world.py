import copy,json,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'Scripts'))
import compile_world,compile_tags
class WorldDefinitions(unittest.TestCase):
 def setUp(self):
  self.world=json.loads((ROOT/'Data/openworld.json').read_text());self.chapter=json.loads((ROOT/'Data/chapter1.json').read_text())
 def test_generated_world_and_tags_are_current(self):
  self.assertEqual(compile_world.generate(self.world,self.chapter),(ROOT/'Source/WroclawTheGame/Content/WorldCatalog.h').read_text())
  self.assertEqual(compile_tags.generate(),(ROOT/'Config/DefaultGameplayTags.ini').read_text())
 def test_invalid_event_patrol_and_evidence_are_rejected(self):
  for change in (lambda w:w['events'][0].update(chance=2),lambda w:w['guards'][0].update(patrol=[]),lambda w:w['combinations'][0].update(evidence=['missing'])):
   w=copy.deepcopy(self.world);change(w)
   with self.assertRaises(ValueError):compile_world.generate(w,self.chapter)
 def test_main_campaign_escape_is_not_chapter_completion(self):
  self.assertGreater(len(self.chapter['campaign']['required_chapters']),1)
  exit=next(a for a in self.chapter['actions'] if a['id']=='exit_city')
  self.assertIn('Campaign.Main.Completed',exit['require_tags'])
  self.assertFalse(any('Campaign.Main.Completed' in a['set_tags'] for a in self.chapter['actions']))
 def test_open_world_has_optional_routes_and_activities(self):
  self.assertGreaterEqual(len(self.chapter['side_quests']),5)
  self.assertLessEqual(len(self.chapter['side_quests']),10)
  self.assertGreaterEqual(len(self.world['locations']),4)
  self.assertTrue(any(g['requires']==[] for g in self.world['guards']))
  self.assertTrue(any(e['hostile'] for e in self.world['events']))
  self.assertTrue(any(not e['hostile'] for e in self.world['events']))
if __name__=='__main__':unittest.main()
