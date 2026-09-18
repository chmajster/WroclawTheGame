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
            definition = f"void UPlayerMenuWidget::{builder}()"
            self.assertEqual(self.cpp.count(definition), 1, builder)
            self.assertIn(f"void {builder}();", self.header, builder)

    def test_modern_visual_shell_is_not_removed(self):
        for token in (
            "FSlateRoundedBoxBrush", "UBackgroundBlur", "SetBlurStrength",
            "AmbientGlowA", "THE GAME  /  PRZEBUDZENIE",
            "SetWidthOverride(1600.0f)", "SetHeightOverride(900.0f)",
        ):
            self.assertIn(token, self.cpp)

    def test_navigation_has_active_indicator_and_live_context(self):
        for token in (
            "TabIndicators.Add(Indicator)",
            "SetHeightOverride(2.0f)",
            "TabIndicators[Index]->SetBrushColor",
            "UpdateContextStatus()",
            'TEXT("SESJA AKTYWNA")',
            'TEXT("MENU GŁÓWNE")',
            'TEXT("ZAPIS DOSTĘPNY")',
        ):
            self.assertIn(token, self.cpp)
        self.assertIn("TObjectPtr<class UTextBlock> ContextStatus", self.header)
        self.assertIn("TArray<TObjectPtr<class UBorder>> TabIndicators", self.header)

    def test_long_menu_panels_are_scrollable(self):
        for token in (
            "ActionScroll = WidgetTree->ConstructWidget<UScrollBox>()",
            "CenterScroll = WidgetTree->ConstructWidget<UScrollBox>()",
            "RightScroll = WidgetTree->ConstructWidget<UScrollBox>()",
            "SetScrollBarVisibility(ESlateVisibility::Hidden)",
            "SetAnimateWheelScrolling(true)",
            "ScrollToStart()",
        ):
            self.assertIn(token, self.cpp)
        for member in ("ActionScroll", "CenterScroll", "RightScroll"):
            self.assertIn(f"TObjectPtr<class UScrollBox> {member}", self.header)

    def test_game_hero_has_modern_summary_strip(self):
        for token in (
            'TEXT("NASTĘPNY CEL")',
            "ProgressPercent",
            "MakeHeroPill",
            'TEXT("SESJA")',
            'TEXT("ZAPIS")',
            'TEXT("POSTĘP")',
            "HeroSummary",
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

    def test_loading_during_active_session_is_guarded(self):
        self.assertIn('TEXT("WCZYTAĆ OSTATNI ZAPIS?")', self.cpp)
        self.assertIn("Action == 1 || Action == 2 || Action == 4", self.cpp)
        self.assertIn("else if (Action == 4)", self.cpp)
        load_body = self.cpp.split("void UPlayerMenuWidget::LoadGame()", 1)[1].split(
            "void UPlayerMenuWidget::QuitGame()", 1
        )[0]
        self.assertIn("Mission->bInGame", load_body)
        self.assertIn("ShowConfirmation(", load_body)

    def test_keyboard_and_gamepad_navigation_remains_available(self):
        for token in (
            "Gamepad_LeftShoulder", "Gamepad_RightShoulder",
            "Gamepad_FaceButton_Right", "Gamepad_Special_Right",
            "NativeOnPreviewKeyDown", "FocusPrimaryAction",
        ):
            self.assertIn(token, self.cpp)

    def test_settings_are_split_into_modern_sections(self):
        for token in (
            'TEXT("KATEGORIE")',
            'TEXT("WYDAJNOŚĆ")',
            'TEXT("INTERFEJS")',
            "SettingsDisplay",
            "SettingsPerformance",
            "SettingsInterface",
            'TEXT("AKTUALNY OBRAZ")',
            'TEXT("METRYKI RENDERU")',
            'TEXT("DOSTĘPNOŚĆ UI")',
        ):
            self.assertIn(token, self.cpp)
        self.assertIn("int32 SettingsSection = 0", self.header)

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

    def test_game_landing_is_context_aware_for_city_mode(self):
        self.assertIn("City->IsActive()", self.cpp)
        self.assertIn("CompletedActivityCount()", self.cpp)
        self.assertIn("TrackableActivityCount()", self.cpp)
        self.assertIn('TEXT("OTWARTY ŚWIAT")', self.cpp)
        self.assertIn('TEXT("POSTĘP MIASTA")', self.cpp)
        self.assertIn("City->NearbyObjective()", self.cpp)
        self.assertIn("City->IsWriteBlocked()", self.cpp)

    def test_city_statistics_use_city_progress_instead_of_campaign_counters(self):
        for token in (
            "CompletedEventCount()",
            "EventCount()",
            "AvailableActivityCount()",
            'TEXT("METRYKI MIASTA")',
            'TEXT("ZDARZENIA AMBIENTOWE")',
            'TEXT("DOSTĘPNE TERAZ")',
        ):
            self.assertIn(token, self.cpp)

    def test_map_uses_correct_coordinate_space_for_campaign_and_city(self):
        for token in (
            "UWroclawMapSubsystem",
            "MapSubsystem->SectorAt(PlayerPosition)",
            "MapSubsystem->GetCity()",
            "MapSubsystem->GetWaypoint()",
            'TEXT("TY")',
            'TEXT("SEKTORY MIASTA")',
            'TEXT("MAPA ODKRYĆ")',
            "NearestDistance",
        ):
            self.assertIn(token, self.cpp)

    def test_character_preview_keeps_camera_and_lighting_controls(self):
        for token in (
            "PreviewFullBody", "PreviewUpperBody", "PreviewFace",
            "PreviewLightingModern", "PreviewLightingDaylight",
            "PreviewLightingNight", "PreviewReset",
        ):
            self.assertIn(token, self.cpp)


if __name__ == "__main__":
    unittest.main()
