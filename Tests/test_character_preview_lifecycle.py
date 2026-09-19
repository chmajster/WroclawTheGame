import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
SUBSYSTEM_H=(ROOT/"Source/WroclawTheGame/Character/CharacterCreatorSubsystem.h").read_text(encoding="utf-8")
SUBSYSTEM_CPP=(ROOT/"Source/WroclawTheGame/Character/CharacterCreatorSubsystem.cpp").read_text(encoding="utf-8")
STUDIO_CPP=(ROOT/"Source/WroclawTheGame/Character/CharacterCreator.cpp").read_text(encoding="utf-8")
CONTROLLER_CPP=(ROOT/"Source/WroclawTheGame/UI/SliceController.cpp").read_text(encoding="utf-8")
APPEARANCE_CPP=(ROOT/"Source/WroclawTheGame/Character/CharacterAppearanceComponent.cpp").read_text(encoding="utf-8")
MENU_CPP=(ROOT/"Source/WroclawTheGame/UI/PlayerMenuWidget.cpp").read_text(encoding="utf-8")

class CharacterPreviewLifecycleTests(unittest.TestCase):
    def test_reopening_creator_starts_from_committed_character(self):
        self.assertIn("void UCharacterCreatorSubsystem::FinishCreation();",SUBSYSTEM_H)
        self.assertIn("Draft=UCharacterCreatorValidator::Normalize(*Catalog,Committed.PlayerAppearanceData)",SUBSYSTEM_CPP)
        self.assertNotIn("BeginCreation() { bEditing=true; bSummary=false; Draft=Catalog->FallbackDefinition",SUBSYSTEM_CPP)

    def test_successful_start_finishes_creation_after_save(self):
        save_fail=CONTROLLER_CPP.index("if (!Saved) { Creator->Restore(Previous); return false; }")
        finish=CONTROLLER_CPP.index("Creator->FinishCreation();")
        self.assertLess(save_fail,finish)

    def test_preview_actor_uses_draft_only_while_editing(self):
        self.assertIn("Creator->bEditing ? Creator->Draft : Creator->Committed.PlayerAppearanceData",STUDIO_CPP)

    def test_player_and_menu_use_committed_character(self):
        self.assertIn("ApplyAppearance(S->Committed.PlayerAppearanceData)",APPEARANCE_CPP)
        self.assertIn("Studio->Appearance->ApplyAppearance(Creator->Committed.PlayerAppearanceData)",MENU_CPP)

if __name__=="__main__":
    unittest.main()
