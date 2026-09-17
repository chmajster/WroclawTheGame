import copy
import importlib.util
import json
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1]
SPEC=importlib.util.spec_from_file_location('catalog',ROOT/'Scripts/compile_chapter.py')
catalog=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(catalog)
class Catalog(unittest.TestCase):
    def setUp(self): self.data=json.loads((ROOT/'Data/chapter1.json').read_text(encoding='utf-8'))
    def test_generated_catalog_is_current(self):
        self.assertEqual(catalog.compile_catalog(self.data),(ROOT/'Source/WroclawTheGame/Content/ChapterCatalog.h').read_text(encoding='utf-8'))
    def test_unknown_dependency_and_cycle_are_rejected(self):
        self.data['actions'][0]['requires']=['missing']
        with self.assertRaisesRegex(ValueError,'Unknown references'): catalog.compile_catalog(self.data)
        self.data['actions'][0]['requires']=['look']
        with self.assertRaisesRegex(ValueError,'cycle'): catalog.compile_catalog(self.data)
    def test_invalid_variant_is_rejected(self):
        self.data['variants'][0]='1111'
        with self.assertRaisesRegex(ValueError,'permutations'): catalog.compile_catalog(self.data)
    def test_five_secrets_and_distinct_exit_actions(self):
        by_id={a['id']:a for a in self.data['actions']}
        self.assertEqual(sum(a['id'].startswith('secret_') for a in self.data['actions']),5)
        self.assertEqual(by_id['apartment_exit']['kind'],'zone')
        self.assertEqual(by_id['apartment_unlocked']['kind'],'door')
        self.assertEqual(by_id['basement_exit']['choice'],by_id['technical_exit']['choice'])
if __name__=='__main__': unittest.main()
