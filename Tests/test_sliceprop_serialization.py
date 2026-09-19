import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SLICE_PROP = ROOT / "Source/WroclawTheGame/Interaction/SliceProp.h"


class SlicePropSerializationContract(unittest.TestCase):
    def test_legacy_serialized_properties_keep_stable_order(self):
        text = SLICE_PROP.read_text(encoding="utf-8")
        names = re.findall(
            r"UPROPERTY\([^\n]*\)\s+(?:TObjectPtr<[^>]+>|FString)\s+(\w+)\s*;",
            text,
        )
        legacy = ["Mesh", "ActionId", "Door", "Puzzle"]
        self.assertEqual(names[:4], legacy)
        self.assertIn("Collider", names)
        self.assertGreater(names.index("Collider"), names.index("Puzzle"))
        collider_decl = re.search(
            r"UPROPERTY\\(([^\\n]*)\\)\\s+TObjectPtr<[^>]+>\\s+Collider\\s*;",
            text,
        )
        self.assertIsNotNone(collider_decl)
        self.assertIn("SkipSerialization", collider_decl.group(1))


if __name__ == "__main__":
    unittest.main()
