import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SLICE_PROP_H = ROOT / "Source/WroclawTheGame/Interaction/SliceProp.h"
SLICE_PROP_CPP = ROOT / "Source/WroclawTheGame/Interaction/SliceProp.cpp"


class SlicePropSerializationContract(unittest.TestCase):
    def test_reflected_schema_matches_authored_map(self):
        text = SLICE_PROP_H.read_text(encoding="utf-8")
        names = re.findall(
            r"UPROPERTY\([^\n]*\)\s+(?:TObjectPtr<[^>]+>|FString)\s+(\w+)\s*;",
            text,
        )
        self.assertEqual(names, ["Mesh", "ActionId", "Door", "Puzzle"])
        self.assertNotIn("Collider", names)
        self.assertNotRegex(text, r"UPROPERTY\([^\n]*\)[^;]*\bCollider\b")

    def test_interaction_bounds_are_runtime_only(self):
        source = SLICE_PROP_CPP.read_text(encoding="utf-8")
        self.assertIn('RootComponent = Mesh;', source)
        self.assertNotIn('CreateDefaultSubobject<UBoxComponent>', source)
        self.assertIn('NewObject<UBoxComponent>', source)
        self.assertIn('AddInstanceComponent(Bounds)', source)


if __name__ == "__main__":
    unittest.main()
