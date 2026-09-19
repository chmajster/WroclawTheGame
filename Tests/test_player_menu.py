from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
MENU_CPP = ROOT / "Source" / "WroclawTheGame" / "UI" / "PlayerMenuWidget.cpp"
MENU_H = ROOT / "Source" / "WroclawTheGame" / "UI" / "PlayerMenuWidget.h"
PERF_H = ROOT / "Source" / "WroclawTheGame" / "UI" / "PerformanceSettings.h"
AUDIO_CPP = ROOT / "Source" / "WroclawTheGame" / "Audio" / "SliceAudio.cpp"
AUDIO_SETTINGS_H = ROOT / "Source" / "WroclawTheGame" / "Audio" / "WTGAudioSettings.h"
ASSETS = ROOT / "Scripts" / "make_source_assets.py"


class PlayerMenuRegressionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cpp = MENU_CPP.read_text(encoding="utf-8")
        cls.header = MENU_H.read_text(encoding="utf-8")
        cls.perf = PERF_H.read_text(encoding="utf-8")
        cls.audio = AUDIO_CPP.read_text(encoding="utf-8")
        cls.audio_settings = AUDIO_SETTINGS_H.read_text(encoding="utf-8")
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

    def test_footer_shows_project_version_and_build_configuration(self):
        for token in (
            "ProjectVersionLabel()",
            "BuildLabel()",
            "ProjectVersion",
            "GGameIni",
            "UE_BUILD_SHIPPING",
            "UE_BUILD_DEVELOPMENT",
        ):
            self.assertIn(token, self.cpp)

    def test_menu_content_respects_platform_safe_zone(self):
        for token in (
            '#include "Components/SafeZone.h"',
            "ConstructWidget<USafeZone>()",
            "SafeArea->AddChild(Scale)",
        ):
            self.assertIn(token, self.cpp)

    def test_modern_visual_shell_is_not_removed(self):
        for token in (
            "FSlateRoundedBoxBrush", "UBackgroundBlur", "SetBlurStrength",
            "AmbientGlowA", "THE GAME  /  PRZEBUDZENIE",
            "SetWidthOverride(1600.0f)", "SetHeightOverride(900.0f)",
        ):
            self.assertIn(token, self.cpp)

    def test_menu_shell_has_reduced_motion_safe_entry_animation(self):
        for token in (
            "ShellLayout = WidgetTree->ConstructWidget<UVerticalBox>()",
            "ShellAnimationTime",
            "SetRenderOpacity",
            "SetRenderTranslation",
            "SetRenderScale",
            "bReduceShellMotion",
            "0.985f",
        ):
            self.assertIn(token, self.cpp)
        self.assertIn("TObjectPtr<class UVerticalBox> ShellLayout", self.header)

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

    def test_context_chrome_has_page_badge_session_state_and_live_clock(self):
        for token in (
            'TEXT("01 / 07")',
            'TEXT("%02d / 07")',
            "PageCounter",
            "SessionStateDot",
            "ClockText",
            'FDateTime::Now().ToString(TEXT("%d.%m.%Y  •  %H:%M"))',
            "ClockRefreshAccumulator",
            "FooterDivider",
        ):
            self.assertIn(token, self.cpp)
        for member in ("PageCounter", "ClockText", "SessionStateDot"):
            self.assertIn(member, self.header)

    def test_three_column_layout_has_dynamic_panel_headers(self):
        for token in (
            "ActionPanelLabel",
            "CenterPanelLabel",
            "RightPanelLabel",
            "UpdatePanelLabels()",
            'TEXT("STEROWANIE")',
            'TEXT("PODGLĄD")',
            'TEXT("BIEŻĄCE WARTOŚCI")',
            "LeftDividerSize->SetHeightOverride(1.0f)",
            "CenterDividerSize->SetHeightOverride(1.0f)",
            "RightDividerSize->SetHeightOverride(1.0f)",
        ):
            self.assertIn(token, self.cpp)
        for member in ("ActionPanelLabel", "CenterPanelLabel", "RightPanelLabel"):
            self.assertIn(member, self.header)

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

    def test_footer_uses_compact_keycap_shortcut_legend(self):
        for token in (
            "MakeKeycap(const FString& Label)",
            "AddFooterShortcut",
            'AddFooterShortcut(TEXT("ESC"), TEXT("WRÓĆ"))',
            'AddFooterShortcut(TEXT("1–7"), TEXT("ZAKŁADKI"))',
            'AddFooterShortcut(TEXT("ENTER / A"), TEXT("WYBIERZ"))',
        ):
            self.assertIn(token, self.cpp)
        self.assertIn("class UBorder* MakeKeycap(const FString& Label);", self.header)

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

    def test_information_cards_use_modern_accent_rails(self):
        for token in (
            "auto* Content = WidgetTree->ConstructWidget<UHorizontalBox>()",
            "auto* Rail = WidgetTree->ConstructWidget<UBorder>()",
            "Rail->SetBrush(RoundedBrush(bHighlighted ? Accent : Divider, 2.0f))",
            "RailSize->SetWidthOverride(3.0f)",
        ):
            self.assertIn(token, self.cpp)

    def test_player_status_is_a_visual_dashboard_not_raw_bitmask_text(self):
        for token in (
            "HealthValue",
            "StaminaValue",
            'FString::Printf(TEXT("%.0f%%"), HealthValue)',
            "AchievementCount",
            'TEXT("AKTYWNOŚCI DZIELNIC")',
            'TEXT("OSTATNI ZAPIS")',
        ):
            self.assertIn(token, self.cpp)
        self.assertNotIn('TEXT("Osiągnięcia       %d', self.cpp)

    def test_game_tab_has_real_campaign_hero(self):
        self.assertIn("UPlayerMenuWidget::AddGameHero()", self.cpp)
        self.assertIn("Mission->State.QuestComplete(Quest)", self.cpp)
        self.assertIn("Mission->HasSave()", self.cpp)
        self.assertIn("POSTĘP ROZDZIAŁU", self.cpp)
        self.assertIn("Progress->SetPercent", self.cpp)

    def test_journal_uses_city_progress_and_visual_bars(self):
        for token in (
            "City->TrackableActivityCount()",
            "City->CompletedActivityCount()",
            "City->EventCount()",
            "City->CompletedEventCount()",
            'TEXT("AKTYWNOŚCI DZIELNIC")',
            'TEXT("ZDARZENIA AMBIENTOWE")',
            "MainProgress->SetPercent",
            "SideProgress->SetPercent",
        ):
            self.assertIn(token, self.cpp)

    def test_character_and_wardrobe_use_player_facing_catalog_names(self):
        for token in (
            "Creator->Catalog->ClothingDefinitions",
            "Definition->DisplayName.ToString()",
            "Creator->Catalog->HairStyleDefinitions",
            "VoiceProfiles.FindByPredicate",
            'TEXT("FRYZURA")',
            'TEXT("PROFIL GŁOSU")',
            'TEXT("SMUKŁA")',
            'TEXT("STANDARDOWA")',
            'TEXT("ATLETYCZNA")',
            'TEXT("MASYWNA")',
        ):
            self.assertIn(token, self.cpp)
        self.assertNotIn('TEXT("PRESET")', self.cpp)
        self.assertNotIn('TEXT("SEED WYGLĄDU")', self.cpp)

    def test_inventory_has_stable_sorting_counts_and_empty_states(self):
        for token in (
            'TEXT("GARDEROBA  •  %d")',
            'TEXT("PRZEDMIOTY  •  %d")',
            "Clothing.Sort",
            "DisplayItems.Sort",
            'TEXT("PUSTO")',
            'TEXT("BRAK PRZEDMIOTÓW")',
        ):
            self.assertIn(token, self.cpp)

    def test_secondary_tabs_remain_data_driven_dashboards(self):
        for token in (
            "State.inventory", "OwnedClothing", "State.evidence",
            "State.QuestComplete", "WorldState.discoveries", "UCanvasPanel",
            "State.history", "LifetimeAchievements", "MakeInfoRow",
        ):
            self.assertIn(token, self.cpp)

    def test_settings_show_modern_saved_toast_feedback(self):
        for token in (
            "ShowToast(const FString& Message)",
            'TEXT("USTAWIENIA ZAPISANE")',
            'TEXT("USTAWIENIA OBRAZU ZAPISANE")',
            'TEXT("USTAWIENIA DOMYŚLNE PRZYWRÓCONE")',
            "ToastTimeRemaining = 1.8f",
            "ToastCard->SetRenderOpacity",
            "ToastCard->RemoveFromParent",
        ):
            self.assertIn(token, self.cpp)
        self.assertIn("TObjectPtr<class UBorder> ToastCard", self.header)

    def test_settings_toast_has_accent_rail_and_reduced_motion_safe_entrance(self):
        for token in (
            "ToastRail",
            "ToastRailSize->SetWidthOverride(3.0f)",
            "ToastAnimationTime",
            "ToastAnimationTime < 0.18f",
            "FMath::Lerp(18.0f, 0.0f, Ease)",
            "bReduceMotion ? FVector2D::ZeroVector",
        ):
            self.assertIn(token, self.cpp)
        self.assertIn("float ToastAnimationTime = 0.18f", self.header)

    def test_confirmation_modal_blurs_background(self):
        for token in (
            "ModalBlur = WidgetTree->ConstructWidget<UBackgroundBlur>()",
            "ModalBlur->SetBlurStrength(8.0f)",
            "ModalBlur->SetBlurRadius(12)",
            "ModalBlur->AddChild(Center)",
        ):
            self.assertIn(token, self.cpp)

    def test_confirmation_modal_has_countdown_progress_and_input_hint(self):
        for token in (
            "ConfirmationProgress",
            "ConfirmationSecondsTotal",
            "ConfirmationSecondsRemaining / ConfirmationSecondsTotal",
            'MakeKeycap(TEXT("ENTER / A"))',
            'MakeKeycap(TEXT("ESC / B"))',
            'MakeText(TEXT("WYBIERZ"), 8, true, Muted)',
            'TEXT("ANULUJ")',
        ):
            self.assertIn(token, self.cpp)
        self.assertIn("TObjectPtr<class UProgressBar> ConfirmationProgress", self.header)

    def test_video_mode_has_confirmation_and_automatic_revert(self):
        self.assertIn("ConfirmVideoMode()", self.cpp)
        self.assertIn("RevertVideoMode()", self.cpp)
        self.assertIn("ConfirmationSecondsTotal = Action == 3 ? 15.0f : 0.0f", self.cpp)\n        self.assertIn("ConfirmationSecondsRemaining = ConfirmationSecondsTotal", self.cpp)
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

    def test_context_hint_changes_with_active_menu_tab(self):
        for token in (
            "UpdateContextHint()",
            'TEXT("PPM  OBRÓT',
            'TEXT("C  NASTĘPNY CEL',
            'TEXT("MAPA ODKRYĆ',
            'TEXT("ENTER / A  ZMIEŃ WARTOŚĆ',
        ):
            self.assertIn(token, self.cpp)
        self.assertIn("TObjectPtr<class UTextBlock> ContextHint", self.header)

    def test_direct_tab_shortcuts_and_dpad_navigation(self):
        for token in (
            "DirectTabKeys",
            "EKeys::One",
            "EKeys::Seven",
            "EKeys::Home",
            "EKeys::End",
            "EKeys::Gamepad_DPad_Left",
            "EKeys::Gamepad_DPad_Right",
            'TEXT("Q/E  •  L1/R1  ZMIEŃ ZAKŁADKĘ")',
            'AddFooterShortcut(TEXT("1–7"), TEXT("ZAKŁADKI"))',
        ):
            self.assertIn(token, self.cpp)

    def test_tab_navigation_has_numbered_labels_and_active_hierarchy(self):
        for token in (
            'FString::Printf(TEXT("%02d  %s"), Index + 1, Labels[Index])',
            "const bool bActive = Index == ActiveTab",
            "bActive ? 1.008f : 1.0f",
            "(bFocused || bActive) ? 1.0f : 0.92f",
        ):
            self.assertIn(token, self.cpp)

    def test_keyboard_and_gamepad_focus_has_visible_feedback(self):
        for token in (
            "FocusTint",
            "Button->SetBackgroundColor(bFocused ? FocusTint : FLinearColor::White)",
            "UpdateFocusPresentation()",
            "HasAnyUserFocus()",
            "HasKeyboardFocus()",
            "SetRenderScale",
            "SetRenderOpacity",
            "1.022f",
        ):
            self.assertIn(token, self.cpp)
        self.assertIn("void UpdateFocusPresentation();", self.header)

    def test_settings_refresh_preserves_focus_on_active_category(self):
        for token in (
            "ActiveTab == 6 && ActionButtons.IsValidIndex(SettingsSection)",
            "UButton* ActiveSettingsSection = ActionButtons[SettingsSection]",
            "ActiveSettingsSection->SetUserFocus",
            "ActiveSettingsSection->SetKeyboardFocus",
        ):
            self.assertIn(token, self.cpp)

    def test_gamepad_dpad_adjusts_focused_audio_slider_instead_of_switching_tabs(self):
        for token in (
            "bAudioSliderFocused",
            "SFXVolumeSlider->HasAnyUserFocus()",
            "UIVolumeSlider->HasAnyUserFocus()",
            "Key == EKeys::Gamepad_DPad_Left && !bAudioSliderFocused",
            "Key == EKeys::Gamepad_DPad_Right && !bAudioSliderFocused",
        ):
            self.assertIn(token, self.cpp)
        for member in ("SFXVolumeSlider", "UIVolumeSlider"):
            self.assertIn(member, self.header)

    def test_keyboard_and_gamepad_navigation_remains_available(self):
        for token in (
            "Gamepad_LeftShoulder", "Gamepad_RightShoulder",
            "Gamepad_FaceButton_Right", "Gamepad_Special_Right",
            "NativeOnPreviewKeyDown", "FocusPrimaryAction",
        ):
            self.assertIn(token, self.cpp)

    def test_performance_settings_have_direct_fps_presets(self):
        for token in (
            'TEXT("SZYBKI LIMIT FPS")',
            "AddFPSPreset",
            "&UPlayerMenuWidget::FPS60",
            "&UPlayerMenuWidget::FPS120",
            "&UPlayerMenuWidget::FPS144",
            "&UPlayerMenuWidget::FPS165",
            "&UPlayerMenuWidget::FPSUnlimited",
            'TEXT("∞")',
        ):
            self.assertIn(token, self.cpp)

    def test_project_audio_settings_header_does_not_collide_with_engine_header(self):
        self.assertEqual(AUDIO_SETTINGS_H.name, "WTGAudioSettings.h")
        self.assertFalse(
            (ROOT / "Source" / "WroclawTheGame" / "Audio" / "AudioSettings.h").exists()
        )
        self.assertIn('#include "WTGAudioSettings.generated.h"', self.audio_settings)
        self.assertIn('#include "Audio/WTGAudioSettings.h"', self.audio)

    def test_audio_settings_have_precise_interactive_sliders(self):
        for token in (
            '#include "Components/Slider.h"',
            "ConstructWidget<USlider>()",
            "SFXSlider->SetValue(SFXVolume)",
            "UISlider->SetValue(UIVolume)",
            "SetStepSize(0.05f)",
            "SetSliderBarColor",
            "SetSliderHandleColor(Accent)",
            "SetMinValue(0.0f)",
            "SetMaxValue(1.0f)",
            "SetSFXVolumeFromSlider(float Volume)",
            "SetUIVolumeFromSlider(float Volume)",
            "OnMouseCaptureEnd.AddDynamic",
            "OnControllerCaptureEnd.AddDynamic",
            "CommitAudioSliderChange()",
            "Settings->SFXVolume = FMath::Clamp(Volume, 0.0f, 1.0f)",
            "Settings->UIVolume = FMath::Clamp(Volume, 0.0f, 1.0f)",
            "Settings->SaveConfig()",
            'TEXT("SUWAK: PRECYZYJNA REGULACJA  •  PRZYCISK: SKOK CO 25%")',
        ):
            self.assertIn(token, self.cpp)
        for token in (
            "SFXVolumeButton",
            "UIVolumeButton",
            "SFXVolumeMeter",
            "UIVolumeMeter",
        ):
            self.assertIn(token, self.header)

    def test_audio_settings_have_visual_level_meters(self):
        for token in (
            "SFXMeter->SetPercent(SFXVolume)",
            "UIVolumeMeter->SetPercent(UIVolume)",
            "SetFillColorAndOpacity(Accent)",
            'TEXT("SUWAK: PRECYZYJNA REGULACJA  •  PRZYCISK: SKOK CO 25%")',
        ):
            self.assertIn(token, self.cpp)

    def test_audio_settings_are_real_and_runtime_backed(self):
        for token in (
            'TEXT("DŹWIĘK")',
            "SettingsAudio()",
            "CycleSFXVolume()",
            "CycleUIVolume()",
            'TEXT("EFEKTY ŚWIATA  •  %d%%")',
            'TEXT("INTERFEJS  •  %d%%")',
        ):
            self.assertIn(token, self.cpp)
        for token in ("SFXVolume", "UIVolume", "SetSFXVolume", "SetUIVolume"):
            self.assertIn(token, self.audio_settings)
        self.assertIn("Settings->SFXVolume", self.audio)
        self.assertIn("Settings->UIVolume", self.audio)
        self.assertIn("EffectiveVolume", self.audio)

    def test_display_quality_details_use_visual_metric_cards(self):
        for token in (
            'TEXT("SZCZEGÓŁY JAKOŚCI")',
            "AddQualityRow",
            'TEXT("WIDOCZNOŚĆ")',
            'TEXT("CIENIE")',
            'TEXT("TEKSTURY")',
            'TEXT("GLOBALNE OŚWIETLENIE")',
            'TEXT("ODBICIA")',
            'TEXT("CIENIOWANIE")',
        ):
            self.assertIn(token, self.cpp)
        self.assertNotIn("const FString QualityDetails", self.cpp)

    def test_settings_navigation_has_numbered_sections_and_progress(self):
        for token in (
            'TEXT("KATEGORIE  •  %d / 4")',
            "SettingsNavProgress",
            "SettingsNavProgress->SetPercent",
            "SettingsNavProgressSize->SetHeightOverride(3.0f)",
            'TEXT("01  OBRAZ")',
            'TEXT("02  WYDAJNOŚĆ")',
            'TEXT("03  INTERFEJS")',
            'TEXT("04  DŹWIĘK")',
        ):
            self.assertIn(token, self.cpp)

    def test_settings_have_safe_restore_defaults_flow(self):
        for token in (
            'TEXT("PRZYWRÓĆ DOMYŚLNE")',
            "ResetSettings()",
            "Preferences->ResetToDefaults()",
            "SetOverallScalabilityLevel(3)",
            "SetResolutionScaleValueEx(100.0f)",
            "Tryb ekranu i rozdzielczość pozostaną bez zmian.",
        ):
            self.assertIn(token, self.cpp)
        self.assertIn("void ResetToDefaults();", self.perf)

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

    def test_player_facing_copy_avoids_implementation_jargon(self):
        for token in (
            'TEXT("OTWARTY ŚWIAT / WROCŁAW")',
            'TEXT("MAPA MIASTA / SEKTORY")',
            'TEXT("WROCŁAW / OTWARTY ŚWIAT")',
            'TEXT("WSPÓLNY MIKS")',
            'TEXT("MUZYKA")',
        ):
            self.assertIn(token, self.cpp)
        self.assertNotIn('TEXT("OTWARTY ŚWIAT / GIS")', self.cpp)
        self.assertNotIn('TEXT("MAPA GIS / SEKTORY")', self.cpp)
        self.assertNotIn("Projekt nie ma jeszcze osobnego kanału muzyki", self.cpp)
        self.assertNotIn("obsługiwane przez runtime", self.cpp)

    def test_game_landing_is_context_aware_for_city_mode(self):
        self.assertIn("City->IsActive()", self.cpp)
        self.assertIn("CompletedActivityCount()", self.cpp)
        self.assertIn("TrackableActivityCount()", self.cpp)
        self.assertIn('TEXT("OTWARTY ŚWIAT")', self.cpp)
        self.assertIn('TEXT("POSTĘP MIASTA")', self.cpp)
        self.assertIn("City->NearbyObjective()", self.cpp)
        self.assertIn("City->IsWriteBlocked()", self.cpp)

    def test_campaign_statistics_have_visual_progress_and_achievement_summary(self):
        for token in (
            "CampaignProgress",
            "CampaignProgressBar->SetPercent",
            'TEXT("POSTĘP KAMPANII")',
            "UnlockedAchievements",
            'TEXT("ODBLOKOWANE")',
        ):
            self.assertIn(token, self.cpp)

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

    def test_journal_map_and_stats_avoid_raw_debug_style_status_text(self):
        for token in (
            'TEXT("CZAS SESJI")',
            'TEXT("POZIOM ZAGROŻENIA")',
            'MakeKeycap(TEXT("C"))',
            'MakeKeycap(TEXT("BACKSPACE"))',
            'TEXT("RĘCZNA")',
            'TEXT("NAWIGACJA")',
        ):
            self.assertIn(token, self.cpp)
        self.assertNotIn('TEXT("C — następny sektor\\nBACKSPACE — usuń cel")', self.cpp)
        self.assertNotIn('TEXT("GPS"), TEXT("NAWIGACJA W ROZWOJU")', self.cpp)
        self.assertNotIn('TEXT("AKTUALNY HEAT")', self.cpp)

    def test_map_has_compass_and_modern_map_chrome(self):
        for token in (
            "AddCompassLabel",
            'AddCompassLabel(TEXT("N")',
            'AddCompassLabel(TEXT("E")',
            'AddCompassLabel(TEXT("S")',
            'AddCompassLabel(TEXT("W")',
            'TEXT("WROCŁAW  •  MAPA 2D")',
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

    def test_character_preview_shows_current_camera_and_lighting_state(self):
        for token in (
            "PreviewViewStatus",
            "PreviewLightingStatus",
            "UpdatePreviewStatus()",
            'TEXT("KADR  •  %s")',
            'TEXT("ŚWIATŁO  •  %s")',
            'PreviewViewLabel = TEXT("TWARZ")',
            'PreviewLightingLabel = TEXT("NOC")',
        ):
            self.assertIn(token, self.cpp)

    def test_character_preview_uses_consistent_keycap_hints(self):
        for token in (
            'MakeKeycap(TEXT("PPM"))',
            'MakeKeycap(TEXT("KÓŁKO"))',
            'MakeText(TEXT("OBRÓT"), 8, true, Muted)',
            'MakeText(TEXT("ZOOM"), 8, true, Muted)',
        ):
            self.assertIn(token, self.cpp)
        self.assertNotIn('TEXT("PPM — OBRÓT   •   KÓŁKO — ZOOM")', self.cpp)

    def test_character_preview_controls_show_real_selected_state(self):
        for token in (
            'PreviewViewLabel == TEXT("CAŁA SYLWETKA")',
            'PreviewViewLabel == TEXT("GÓRNA CZĘŚĆ")',
            'PreviewViewLabel == TEXT("TWARZ")',
            'PreviewLightingLabel == TEXT("STUDIO")',
            'PreviewLightingLabel == TEXT("DZIEŃ")',
            'PreviewLightingLabel == TEXT("NOC")',
            "PreviewViewButtons.Add",
            "PreviewLightingButtons.Add",
            "UpdatePreviewControlStyles()",
            "ApplySelectedStyle",
        ):
            self.assertIn(token, self.cpp)
        for member in ("PreviewViewButtons", "PreviewLightingButtons"):
            self.assertIn(member, self.header)

    def test_character_preview_mouse_controls_are_scoped_to_center_panel(self):
        for token in (
            "CenterScroll->GetCachedGeometry().IsUnderLocation(Event.GetScreenSpacePosition())",
            "Studio && bPreviewPanelHovered",
            "Studio->Zoom(-Event.GetWheelDelta() * 18.0f)",
            "bRotatingPreview = true",
        ):
            self.assertIn(token, self.cpp)

    def test_unavailable_states_are_player_facing_not_engine_debug(self):
        for token in (
            'TEXT("PODGLĄD NIEDOSTĘPNY")',
            "Pozostałe informacje profilu są nadal dostępne.",
            "Nie udało się wczytać ustawień obrazu.",
            "Bieżące ustawienia gry nie zostały zmienione.",
        ):
            self.assertIn(token, self.cpp)
        self.assertNotIn("Nie udało się pobrać UGameUserSettings.", self.cpp)

    def test_character_preview_keeps_camera_and_lighting_controls(self):
        for token in (
            "PreviewFullBody", "PreviewUpperBody", "PreviewFace",
            "PreviewLightingModern", "PreviewLightingDaylight",
            "PreviewLightingNight", "PreviewReset",
        ):
            self.assertIn(token, self.cpp)


if __name__ == "__main__":
    unittest.main()
