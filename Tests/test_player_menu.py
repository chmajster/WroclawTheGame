from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
MENU_CPP = ROOT / "Source" / "WroclawTheGame" / "UI" / "PlayerMenuWidget.cpp"
MENU_H = ROOT / "Source" / "WroclawTheGame" / "UI" / "PlayerMenuWidget.h"
PERF_H = ROOT / "Source" / "WroclawTheGame" / "UI" / "PerformanceSettings.h"
AUDIO_CPP = ROOT / "Source" / "WroclawTheGame" / "Audio" / "SliceAudio.cpp"
ASSETS = ROOT / "Scripts" / "make_source_assets.py"


class PlayerMenuRegressionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cpp = MENU_CPP.read_text(encoding="utf-8")
        cls.header = MENU_H.read_text(encoding="utf-8")
        cls.perf = PERF_H.read_text(encoding="utf-8")
        cls.audio = AUDIO_CPP.read_text(encoding="utf-8")
        cls.assets = ASSETS.read_text(encoding="utf-8")

    def test_all_primary_tabs_are_present(self):
        labels = (
            'TEXT("GRA")', 'TEXT("POSTAĆ")', 'TEXT("EKWIPUNEK")',
            'TEXT("DZIENNIK")', 'TEXT("MAPA")', 'TEXT("STATYSTYKI")',
            'TEXT("USTAWIENIA")',
        )
        for label in labels:
            self.assertIn(label, self.cpp)

        builders = (
            "BuildGameTab", "BuildCharacterTab", "BuildInventoryTab",
            "BuildJournalTab", "BuildMapTab", "BuildStatsTab", "BuildSettingsTab",
        )
        for builder in builders:
            self.assertEqual(self.cpp.count(f"UPlayerMenuWidget::{builder}()"), 1)
            self.assertIn(f"void {builder}();", self.header)

    def test_modern_visual_shell_is_not_removed(self):
        for token in (
            "FSlateRoundedBoxBrush", "UBackgroundBlur", "SetBlurStrength",
            "AmbientGlowA", "THE GAME  /  PRZEBUDZENIE",
            "SetWidthOverride(1600.0f)", "SetHeightOverride(900.0f)",
        ):
            self.assertIn(token, self.cpp)

    def test_game_tab_has_real_campaign_hero(self):
        self.assertIn("UPlayerMenuWidget::AddGameHero()", self.cpp)
        self.assertIn("Mission->State.QuestComplete(Quest)", self.cpp)
        self.assertIn("Mission->HasSave()", self.cpp)
        self.assertIn("POSTĘP ROZDZIAŁU", self.cpp)
        self.assertIn("Progress->SetPercent", self.cpp)

    def test_secondary_tabs_remain_data_driven_dashboards(self):
        for token in (
            "State.inventory", "OwnedClothing", "State.evidence",
            "State.QuestComplete", "WorldState.discoveries", "UCanvasPanel",
            "State.history", "LifetimeAchievements", "MakeInfoRow",
        ):
            self.assertIn(token, self.cpp)

    def test_video_mode_has_confirmation_and_automatic_revert(self):
        self.assertIn("ConfirmVideoMode()", self.cpp)
        self.assertIn("RevertVideoMode()", self.cpp)
        self.assertIn("ConfirmationSecondsRemaining = Action == 3 ? 15.0f", self.cpp)
        self.assertIn("AUTOMATYCZNE COFNIĘCIE", self.cpp)

    def test_destructive_actions_are_guarded(self):
        self.assertIn('TEXT("ROZPOCZĄĆ NOWĄ GRĘ?")', self.cpp)
        self.assertIn('TEXT("WYJŚĆ Z GRY?")', self.cpp)
        self.assertIn("bDestructive ? Cancel : Confirm", self.cpp)
        self.assertIn("DangerStyle", self.cpp)
        self.assertIn("OSTATNI ZAPIS: POPRAWNY", self.cpp)

    def test_keyboard_and_gamepad_navigation_remains_available(self):
        for token in (
            "Gamepad_LeftShoulder", "Gamepad_RightShoulder",
            "Gamepad_FaceButton_Right", "Gamepad_Special_Right",
            "NativeOnPreviewKeyDown", "FocusPrimaryAction",
        ):
            self.assertIn(token, self.cpp)

    def test_accessibility_preferences_are_persistent_and_used(self):
        for field in ("bReduceUIMotion", "bMenuBackgroundBlur", "bUISounds"):
            self.assertIn(f"bool {field}", self.perf)
            self.assertIn(field, self.cpp)
        for action in ("ToggleReduceUIMotion", "ToggleMenuBackgroundBlur", "ToggleUISounds"):
            self.assertIn(f"UPlayerMenuWidget::{action}()", self.cpp)

    def test_ui_audio_assets_and_non_spatial_playback_exist(self):
        self.assertIn("'UIHover':", self.assets)
        self.assertIn("'UIClick':", self.assets)
        self.assertIn("PlaySound2D", self.audio)
        self.assertIn("true);", self.audio)

    def test_character_preview_keeps_camera_and_lighting_controls(self):
        for token in (
            "PreviewFullBody", "PreviewUpperBody", "PreviewFace",
            "PreviewLightingModern", "PreviewLightingDaylight",
            "PreviewLightingNight", "PreviewReset",
        ):
            self.assertIn(token, self.cpp)


if __name__ == "__main__":
    unittest.main()
