import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
CPP=(ROOT/"Source/WroclawTheGame/UI/CharacterCreatorWidget.cpp").read_text(encoding="utf-8")

class CharacterCreatorUIParityTests(unittest.TestCase):
    def test_creator_uses_player_menu_visual_language(self):
        for token in (
            "FSlateRoundedBoxBrush",
            "RoundedBrush",
            "PanelSoft",
            "AccentHover",
            'TEXT("PERSONALIZACJA")',
            'TEXT("PODGLĄD")',
            'TEXT("AKCJE")',
            'TEXT("STEROWANIE I ZATWIERDZENIE")',
            'TEXT("PODGLĄD NA ŻYWO")',
            "UScaleBox",
            "USafeZone",
        ):
            self.assertIn(token,CPP)

    def test_preview_uses_same_ui_material_as_player_menu(self):
        self.assertIn("M_CharacterPreview.M_CharacterPreview",CPP)
        self.assertIn('SetTextureParameterValue(TEXT("PreviewTexture"),Studio->RenderTarget)',CPP)
        self.assertIn("Image->SetBrushFromMaterial(MID)",CPP)

    def test_control_rows_are_cards_and_commands_are_styled(self):
        self.assertIn("if (Box==Controls)",CPP)
        self.assertIn("RowCard=Card",CPP)
        self.assertIn("StyleButton(B",CPP)
        self.assertIn("Style.Normal = RoundedBrush",CPP)

if __name__=="__main__":
    unittest.main()
