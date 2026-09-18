#include "UI/PlayerMenuWidget.h"

#include "UI/SliceController.h"
#include "UI/PerformanceSettings.h"
#include "Character/CharacterCreator.h"
#include "Character/CharacterCreatorSubsystem.h"
#include "Character/CharacterAppearanceComponent.h"
#include "Character/SliceCharacter.h"
#include "Components/GameplayComponents.h"
#include "Mission/SliceMission.h"
#include "Systems/CityGameplaySubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "Input/Reply.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
const FLinearColor Background(0.008f, 0.012f, 0.018f, 0.985f);
const FLinearColor Panel(0.022f, 0.031f, 0.043f, 0.96f);
const FLinearColor PanelSoft(0.035f, 0.046f, 0.061f, 0.92f);
const FLinearColor Accent(0.95f, 0.72f, 0.16f, 1.0f);
const FLinearColor Muted(0.63f, 0.68f, 0.74f, 1.0f);
const FLinearColor Divider(0.12f, 0.14f, 0.17f, 1.0f);
}

UTextBlock* UPlayerMenuWidget::MakeText(const FString& Value, int32 Size, bool bBold, FLinearColor Color)
{
    auto* Text = WidgetTree->ConstructWidget<UTextBlock>();
    Text->SetText(FText::FromString(Value));
    auto Font = Text->GetFont();
    Font.Size = Size;
    if (bBold)
        Font.TypefaceFontName = TEXT("Bold");
    Text->SetFont(Font);
    Text->SetColorAndOpacity(FSlateColor(Color));
    Text->SetAutoWrapText(true);
    return Text;
}

UButton* UPlayerMenuWidget::MakeButton(const FString& Label, bool bAccent)
{
    auto* Button = WidgetTree->ConstructWidget<UButton>();
    Button->SetBackgroundColor(bAccent ? Accent : PanelSoft);
    Button->SetColorAndOpacity(FLinearColor::White);
    auto* Text = MakeText(Label, 14, true, bAccent ? FLinearColor(0.05f, 0.05f, 0.05f) : FLinearColor::White);
    Text->SetJustification(ETextJustify::Center);
    Button->AddChild(Text);
    return Button;
}

UBorder* UPlayerMenuWidget::MakeCard(const FMargin& Padding)
{
    auto* Border = WidgetTree->ConstructWidget<UBorder>();
    Border->SetBrushColor(Panel);
    Border->SetPadding(Padding);
    return Border;
}

void UPlayerMenuWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(true);

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* PreviewClass = LoadClass<AWTG_CharacterCreator>(
        nullptr, TEXT("/Game/CharacterCreator/BP_WTG_CharacterCreator.BP_WTG_CharacterCreator_C"));
    Studio = GetWorld()->SpawnActor<AWTG_CharacterCreator>(
        PreviewClass ? PreviewClass : AWTG_CharacterCreator::StaticClass(),
        FVector(0, 0, -50000), FRotator::ZeroRotator, Params);

    if (Studio)
    {
        Studio->SetActorEnableCollision(false);
        Studio->SetView(TEXT("FullBody"));
        Studio->SetLighting(TEXT("Neutral"));
        if (auto* Creator = GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>())
            Studio->Appearance->ApplyAppearance(Creator->Committed.PlayerAppearanceData);
    }

    BuildShell();
    Refresh();
}

void UPlayerMenuWidget::NativeDestruct()
{
    if (Studio)
        Studio->Destroy();
    Studio = nullptr;
    Super::NativeDestruct();
}

void UPlayerMenuWidget::BuildShell()
{
    auto* Root = WidgetTree->ConstructWidget<UOverlay>();
    WidgetTree->RootWidget = Root;

    auto* Backdrop = WidgetTree->ConstructWidget<UBorder>();
    Backdrop->SetBrushColor(Background);
    Root->AddChildToOverlay(Backdrop);

    auto* Layout = WidgetTree->ConstructWidget<UVerticalBox>();
    auto* LayoutSlot = Root->AddChildToOverlay(Layout);
    LayoutSlot->SetPadding(FMargin(24, 18, 24, 16));

    auto* Top = WidgetTree->ConstructWidget<UHorizontalBox>();
    auto* TopSlot = Layout->AddChildToVerticalBox(Top);
    TopSlot->SetPadding(FMargin(0, 0, 0, 12));

    auto* Brand = WidgetTree->ConstructWidget<USizeBox>();
    Brand->SetWidthOverride(265);
    Brand->AddChild(MakeText(TEXT("WROCŁAW THE GAME"), 18, true, Accent));
    Top->AddChildToHorizontalBox(Brand);

    const TCHAR* Labels[] = {TEXT("GRA"), TEXT("POSTAĆ"), TEXT("EKWIPUNEK"),
                             TEXT("DZIENNIK"), TEXT("MAPA"), TEXT("STATYSTYKI"), TEXT("USTAWIENIA")};
    for (int32 Index = 0; Index < 7; ++Index)
    {
        auto* Button = MakeButton(Labels[Index]);
        auto* Slot = Top->AddChildToHorizontalBox(Button);
        Slot->SetPadding(FMargin(3, 0));
        Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        TabButtons.Add(Button);
    }

    TabButtons[0]->OnClicked.AddDynamic(this, &UPlayerMenuWidget::TabGame);
    TabButtons[1]->OnClicked.AddDynamic(this, &UPlayerMenuWidget::TabCharacter);
    TabButtons[2]->OnClicked.AddDynamic(this, &UPlayerMenuWidget::TabInventory);
    TabButtons[3]->OnClicked.AddDynamic(this, &UPlayerMenuWidget::TabJournal);
    TabButtons[4]->OnClicked.AddDynamic(this, &UPlayerMenuWidget::TabMap);
    TabButtons[5]->OnClicked.AddDynamic(this, &UPlayerMenuWidget::TabStats);
    TabButtons[6]->OnClicked.AddDynamic(this, &UPlayerMenuWidget::TabSettings);

    auto* Line = WidgetTree->ConstructWidget<UBorder>();
    Line->SetBrushColor(Divider);
    auto* LineSlot = Layout->AddChildToVerticalBox(Line);
    LineSlot->SetPadding(FMargin(0, 0, 0, 12));
    LineSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

    auto* Body = WidgetTree->ConstructWidget<UHorizontalBox>();
    auto* BodySlot = Layout->AddChildToVerticalBox(Body);
    BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    auto* LeftCard = MakeCard(FMargin(14));
    auto* LeftSize = WidgetTree->ConstructWidget<USizeBox>();
    LeftSize->SetWidthOverride(255);
    LeftSize->AddChild(LeftCard);
    Body->AddChildToHorizontalBox(LeftSize)->SetPadding(FMargin(0, 0, 12, 0));
    ActionColumn = WidgetTree->ConstructWidget<UVerticalBox>();
    LeftCard->AddChild(ActionColumn);

    auto* CenterCard = MakeCard(FMargin(12));
    auto* CenterSlot = Body->AddChildToHorizontalBox(CenterCard);
    CenterSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CenterSlot->SetPadding(FMargin(0, 0, 12, 0));
    CenterColumn = WidgetTree->ConstructWidget<UVerticalBox>();
    CenterCard->AddChild(CenterColumn);

    auto* RightCard = MakeCard(FMargin(16));
    auto* RightSize = WidgetTree->ConstructWidget<USizeBox>();
    RightSize->SetWidthOverride(330);
    RightSize->AddChild(RightCard);
    Body->AddChildToHorizontalBox(RightSize);
    RightColumn = WidgetTree->ConstructWidget<UVerticalBox>();
    RightCard->AddChild(RightColumn);

    auto* Footer = WidgetTree->ConstructWidget<UHorizontalBox>();
    auto* FooterSlot = Layout->AddChildToVerticalBox(Footer);
    FooterSlot->SetPadding(FMargin(0, 12, 0, 0));

    PageTitle = MakeText(TEXT(""), 13, true, Muted);
    Footer->AddChildToHorizontalBox(PageTitle)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* Hint = MakeText(TEXT("ESC — wróć do gry   •   PPM — obrót postaci   •   kółko — zoom"), 12, false, Muted);
    Hint->SetJustification(ETextJustify::Right);
    Footer->AddChildToHorizontalBox(Hint);
}

void UPlayerMenuWidget::Refresh()
{
    if (!ActionColumn || !CenterColumn || !RightColumn)
        return;

    ActionColumn->ClearChildren();
    CenterColumn->ClearChildren();
    RightColumn->ClearChildren();
    UpdateTabStyle();

    switch (ActiveTab)
    {
        case 0: BuildGameTab(); break;
        case 1: BuildCharacterTab(); break;
        case 2: BuildInventoryTab(); break;
        case 3: BuildJournalTab(); break;
        case 4: BuildMapTab(); break;
        case 5: BuildStatsTab(); break;
        case 6: BuildSettingsTab(); break;
        default: ActiveTab = 0; BuildGameTab(); break;
    }
}

void UPlayerMenuWidget::SelectTab(int32 Index)
{
    ActiveTab = FMath::Clamp(Index, 0, 6);
    Refresh();
}

void UPlayerMenuWidget::UpdateTabStyle()
{
    for (int32 Index = 0; Index < TabButtons.Num(); ++Index)
        if (TabButtons[Index])
            TabButtons[Index]->SetBackgroundColor(Index == ActiveTab ? Accent : PanelSoft);
}

void UPlayerMenuWidget::AddPreview()
{
    if (!Studio || !Studio->RenderTarget)
    {
        auto* Fallback = MakeText(TEXT("Podgląd postaci jest chwilowo niedostępny."), 18, true, Muted);
        Fallback->SetJustification(ETextJustify::Center);
        CenterColumn->AddChildToVerticalBox(Fallback)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        return;
    }

    auto* Stage = WidgetTree->ConstructWidget<UOverlay>();
    auto* StageSlot = CenterColumn->AddChildToVerticalBox(Stage);
    StageSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    auto* StageBg = WidgetTree->ConstructWidget<UBorder>();
    StageBg->SetBrushColor(FLinearColor(0.01f, 0.014f, 0.02f, 1.0f));
    Stage->AddChildToOverlay(StageBg);

    auto* Scale = WidgetTree->ConstructWidget<UScaleBox>();
    Scale->SetStretch(EStretch::ScaleToFit);
    Scale->SetStretchDirection(EStretchDirection::Both);
    auto* ScaleSlot = Stage->AddChildToOverlay(Scale);
    ScaleSlot->SetPadding(FMargin(8));
    ScaleSlot->SetHorizontalAlignment(HAlign_Center);
    ScaleSlot->SetVerticalAlignment(VAlign_Center);

    auto* Image = WidgetTree->ConstructWidget<UImage>();
    FSlateBrush Brush;
    Brush.SetResourceObject(Studio->RenderTarget);
    Brush.ImageSize = FVector2D(720, 1000);
    Image->SetBrush(Brush);
    Scale->AddChild(Image);

    auto* CaptionCard = WidgetTree->ConstructWidget<UBorder>();
    CaptionCard->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.62f));
    CaptionCard->SetPadding(FMargin(12, 8));
    auto* CaptionSlot = Stage->AddChildToOverlay(CaptionCard);
    CaptionSlot->SetHorizontalAlignment(HAlign_Fill);
    CaptionSlot->SetVerticalAlignment(VAlign_Bottom);
    CaptionSlot->SetPadding(FMargin(14));
    auto* Caption = MakeText(TEXT("AKTYWNA POSTAĆ  •  PODGLĄD NA ŻYWO"), 12, true, Accent);
    Caption->SetJustification(ETextJustify::Center);
    CaptionCard->AddChild(Caption);
}

void UPlayerMenuWidget::AddTextPage(const FString& Heading, const FString& Body)
{
    auto* Header = MakeText(Heading, 22, true, Accent);
    CenterColumn->AddChildToVerticalBox(Header)->SetPadding(FMargin(6, 4, 6, 14));

    auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
    auto* ScrollSlot = CenterColumn->AddChildToVerticalBox(Scroll);
    ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    auto* BodyText = MakeText(Body.IsEmpty() ? TEXT("Brak danych.") : Body, 15, false, FLinearColor(0.9f, 0.92f, 0.95f));
    BodyText->SetLineHeightPercentage(1.15f);
    Scroll->AddChild(BodyText);
}

void UPlayerMenuWidget::AddPlayerStatus()
{
    auto* Mission = GetGameInstance()->GetSubsystem<USliceMission>();
    auto* Creator = GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>();
    auto* Player = Cast<ASliceCharacter>(GetOwningPlayerPawn());

    const FString Name = Creator && !Creator->Committed.PlayerAppearanceData.Name.IsEmpty()
                             ? Creator->Committed.PlayerAppearanceData.Name
                             : TEXT("Gracz");
    RightColumn->AddChildToVerticalBox(MakeText(Name.ToUpper(), 20, true, FLinearColor::White))
        ->SetPadding(FMargin(0, 0, 0, 3));
    RightColumn->AddChildToVerticalBox(MakeText(TEXT("WROCŁAW • DOLNY ŚLĄSK"), 11, true, Accent))
        ->SetPadding(FMargin(0, 0, 0, 16));

    if (Player && Player->HealthState && Player->StaminaState)
    {
        RightColumn->AddChildToVerticalBox(MakeText(TEXT("ZDROWIE"), 11, true, Muted));
        auto* Health = WidgetTree->ConstructWidget<UProgressBar>();
        Health->SetPercent(FMath::Clamp(Player->HealthState->Value / 100.0f, 0.0f, 1.0f));
        Health->SetFillColorAndOpacity(FLinearColor(0.72f, 0.18f, 0.16f, 1));
        RightColumn->AddChildToVerticalBox(Health)->SetPadding(FMargin(0, 4, 0, 12));

        RightColumn->AddChildToVerticalBox(MakeText(TEXT("KONDYCJA"), 11, true, Muted));
        auto* Stamina = WidgetTree->ConstructWidget<UProgressBar>();
        Stamina->SetPercent(FMath::Clamp(Player->StaminaState->Value / 100.0f, 0.0f, 1.0f));
        Stamina->SetFillColorAndOpacity(FLinearColor(0.18f, 0.68f, 0.58f, 1));
        RightColumn->AddChildToVerticalBox(Stamina)->SetPadding(FMargin(0, 4, 0, 18));
    }

    if (Mission)
    {
        RightColumn->AddChildToVerticalBox(MakeText(TEXT("AKTUALNY CEL"), 11, true, Muted))
            ->SetPadding(FMargin(0, 0, 0, 5));
        auto* Objective = MakeText(Mission->ObjectiveText(), 14, true, FLinearColor(0.94f, 0.95f, 0.97f));
        RightColumn->AddChildToVerticalBox(Objective)->SetPadding(FMargin(0, 0, 0, 16));

        const FString Meta = FString::Printf(
            TEXT("Zagrożenie: %d / 5\nCzas rozgrywki: %.0f min\nOsiągnięcia: %d\nZapis: %s"),
            Mission->WorldState.HeatLevel(),
            Mission->State.elapsed / 60.0,
            Mission->LifetimeAchievements,
            Mission->bLastSaveSucceeded ? TEXT("OK") : TEXT("BŁĄD"));
        RightColumn->AddChildToVerticalBox(MakeText(Meta, 12, false, Muted));
    }
}

void UPlayerMenuWidget::BuildGameTab()
{
    PageTitle->SetText(FText::FromString(TEXT("GRA / CENTRUM GRACZA")));

    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("GRA"), 16, true, Accent))
        ->SetPadding(FMargin(3, 0, 3, 12));

    auto* Mission = GetGameInstance()->GetSubsystem<USliceMission>();

    auto* ResumeButton = MakeButton(TEXT("WZNÓW"), true);
    ResumeButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::Resume);
    ResumeButton->SetIsEnabled(Mission && Mission->bInGame);
    ActionColumn->AddChildToVerticalBox(ResumeButton)->SetPadding(FMargin(0, 0, 0, 8));

    auto* NewButton = MakeButton(TEXT("NOWA GRA"));
    NewButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::NewGame);
    ActionColumn->AddChildToVerticalBox(NewButton)->SetPadding(FMargin(0, 0, 0, 8));

    auto* LoadButton = MakeButton(TEXT("WCZYTAJ OSTATNI ZAPIS"));
    LoadButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::LoadGame);
    const bool CityActive = GetWorld()->GetSubsystem<UCityGameplaySubsystem>()->IsActive();
    LoadButton->SetIsEnabled(CityActive || (Mission && Mission->HasSave()));
    ActionColumn->AddChildToVerticalBox(LoadButton)->SetPadding(FMargin(0, 0, 0, 8));

    auto* CharacterButton = MakeButton(TEXT("POSTAĆ"));
    CharacterButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::TabCharacter);
    ActionColumn->AddChildToVerticalBox(CharacterButton)->SetPadding(FMargin(0, 8, 0, 8));

    auto* QuitButton = MakeButton(TEXT("WYJDŹ DO PULPITU"));
    QuitButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::QuitGame);
    ActionColumn->AddChildToVerticalBox(QuitButton)->SetPadding(FMargin(0, 18, 0, 0));

    AddPreview();
    AddPlayerStatus();
}

void UPlayerMenuWidget::BuildCharacterTab()
{
    PageTitle->SetText(FText::FromString(TEXT("POSTAĆ / PODGLĄD")));

    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("KAMERA"), 16, true, Accent))
        ->SetPadding(FMargin(3, 0, 3, 12));

    auto* Full = MakeButton(TEXT("CAŁA SYLWETKA"), true);
    Full->OnClicked.AddDynamic(this, &UPlayerMenuWidget::PreviewFullBody);
    ActionColumn->AddChildToVerticalBox(Full)->SetPadding(FMargin(0, 0, 0, 8));

    auto* Upper = MakeButton(TEXT("GÓRNA CZĘŚĆ"));
    Upper->OnClicked.AddDynamic(this, &UPlayerMenuWidget::PreviewUpperBody);
    ActionColumn->AddChildToVerticalBox(Upper)->SetPadding(FMargin(0, 0, 0, 8));

    auto* Face = MakeButton(TEXT("TWARZ"));
    Face->OnClicked.AddDynamic(this, &UPlayerMenuWidget::PreviewFace);
    ActionColumn->AddChildToVerticalBox(Face)->SetPadding(FMargin(0, 0, 0, 16));

    auto* Left = MakeButton(TEXT("OBRÓĆ W LEWO"));
    Left->OnClicked.AddDynamic(this, &UPlayerMenuWidget::PreviewRotateLeft);
    ActionColumn->AddChildToVerticalBox(Left)->SetPadding(FMargin(0, 0, 0, 8));

    auto* Right = MakeButton(TEXT("OBRÓĆ W PRAWO"));
    Right->OnClicked.AddDynamic(this, &UPlayerMenuWidget::PreviewRotateRight);
    ActionColumn->AddChildToVerticalBox(Right);

    AddPreview();

    auto* Creator = GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>();
    if (Creator)
    {
        const auto& A = Creator->Committed.PlayerAppearanceData;
        const FString Sex = A.Sex == EAppearanceSex::Female ? TEXT("Kobieta") : TEXT("Mężczyzna");
        const FString Details = FString::Printf(
            TEXT("%s\n\nPłeć: %s\nWzrost: %.0f cm\nWiek wizualny: %.0f\nSylwetka: %s\nProfil głosu: %s\nPreset: %s\nSeed: %d"),
            *A.Name.ToUpper(), *Sex, A.Height, A.VisualAge, *A.BodyBuild.ToString(),
            *A.VoiceProfileID.ToString(), *A.PresetID.ToString(), A.RandomSeed);
        RightColumn->AddChildToVerticalBox(MakeText(TEXT("PROFIL POSTACI"), 13, true, Accent))
            ->SetPadding(FMargin(0, 0, 0, 10));
        RightColumn->AddChildToVerticalBox(MakeText(Details, 14, false, FLinearColor(0.9f, 0.92f, 0.95f)));
    }
}

void UPlayerMenuWidget::BuildInventoryTab()
{
    PageTitle->SetText(FText::FromString(TEXT("EKWIPUNEK")));
    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("EKWIPUNEK"), 16, true, Accent));
    ActionColumn->AddChildToVerticalBox(MakeText(
        TEXT("Przedmioty fabularne, użytkowe i zasoby zapisane w aktualnym stanie kampanii."),
        12, false, Muted))->SetPadding(FMargin(0, 10, 0, 0));

    auto* Mission = GetGameInstance()->GetSubsystem<USliceMission>();
    AddTextPage(TEXT("ZAWARTOŚĆ"), Mission ? Mission->InventoryText() : TEXT("Brak danych."));
    AddPlayerStatus();
}

void UPlayerMenuWidget::BuildJournalTab()
{
    PageTitle->SetText(FText::FromString(TEXT("DZIENNIK / POSTĘP")));
    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("DZIENNIK"), 16, true, Accent));
    ActionColumn->AddChildToVerticalBox(MakeText(
        TEXT("Bieżące zadania, tropy i postęp dzielnic. Dane są czytane z aktywnego zapisu."),
        12, false, Muted))->SetPadding(FMargin(0, 10, 0, 0));

    auto* Mission = GetGameInstance()->GetSubsystem<USliceMission>();
    auto* City = GetWorld()->GetSubsystem<UCityGameplaySubsystem>();
    const FString Body = City->IsActive() ? City->Journal() : (Mission ? Mission->QuestLogText() : TEXT(""));
    AddTextPage(TEXT("AKTUALNE WPISY"), Body);
    AddPlayerStatus();
}

void UPlayerMenuWidget::BuildMapTab()
{
    PageTitle->SetText(FText::FromString(TEXT("MAPA / WROCŁAW")));
    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("MAPA"), 16, true, Accent));
    ActionColumn->AddChildToVerticalBox(MakeText(
        TEXT("Widok tekstowy aktywnej mapy świata. Punkty i sektory pochodzą z bieżącego stanu gry."),
        12, false, Muted))->SetPadding(FMargin(0, 10, 0, 0));

    auto* Mission = GetGameInstance()->GetSubsystem<USliceMission>();
    AddTextPage(TEXT("WROCŁAW"), Mission ? Mission->WorldMapText() : TEXT("Brak danych mapy."));
    AddPlayerStatus();
}

void UPlayerMenuWidget::BuildStatsTab()
{
    PageTitle->SetText(FText::FromString(TEXT("STATYSTYKI / SESJA")));
    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("STATYSTYKI"), 16, true, Accent));
    ActionColumn->AddChildToVerticalBox(MakeText(
        TEXT("Podsumowanie aktywnej sesji i stanu zapisu."),
        12, false, Muted))->SetPadding(FMargin(0, 10, 0, 0));

    auto* Mission = GetGameInstance()->GetSubsystem<USliceMission>();
    FString Body = TEXT("Brak aktywnej sesji.");
    if (Mission)
    {
        Body = FString::Printf(
            TEXT("CZAS ROZGRYWKI\n%.0f minut\n\nPOZIOM ZAGROŻENIA\n%d / 5\n\nOSIĄGNIĘCIA\n%d\n\nSTAN ZAPISU\n%s\n\nSTATUS KAMPANII\n%s"),
            Mission->State.elapsed / 60.0,
            Mission->WorldState.HeatLevel(),
            Mission->LifetimeAchievements,
            Mission->bLastSaveSucceeded ? TEXT("Poprawny") : TEXT("Błąd zapisu"),
            Mission->State.Finished() ? TEXT("Rozdział ukończony") :
            (Mission->bInGame ? TEXT("W toku") : TEXT("Menu główne")));
    }
    AddTextPage(TEXT("SESJA"), Body);

    RightColumn->AddChildToVerticalBox(MakeText(TEXT("OSIĄGNIĘCIA"), 13, true, Accent))
        ->SetPadding(FMargin(0, 0, 0, 10));
    RightColumn->AddChildToVerticalBox(
        MakeText(Mission ? Mission->AchievementsText() : TEXT("Brak danych."), 13, false,
                 FLinearColor(0.9f, 0.92f, 0.95f)));
}

void UPlayerMenuWidget::BuildSettingsTab()
{
    PageTitle->SetText(FText::FromString(TEXT("USTAWIENIA / WYŚWIETLANIE")));

    auto* Preferences = UWTGPerformanceSettings::Get();
    UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr;

    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("WYŚWIETLANIE"), 16, true, Accent))
        ->SetPadding(FMargin(3, 0, 3, 12));

    const bool bFPSVisible = Preferences && Preferences->bShowFPS;
    auto* FPSButton = MakeButton(
        FString::Printf(TEXT("LICZNIK FPS: %s"), bFPSVisible ? TEXT("WŁ.") : TEXT("WYŁ.")),
        bFPSVisible);
    FPSButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::ToggleFPSCounter);
    ActionColumn->AddChildToVerticalBox(FPSButton)->SetPadding(FMargin(0, 0, 0, 8));

    const bool bVSync = UserSettings && UserSettings->IsVSyncEnabled();
    auto* VSyncButton = MakeButton(
        FString::Printf(TEXT("VSYNC: %s"), bVSync ? TEXT("WŁ.") : TEXT("WYŁ.")),
        bVSync);
    VSyncButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::ToggleVSync);
    ActionColumn->AddChildToVerticalBox(VSyncButton)->SetPadding(FMargin(0, 0, 0, 16));

    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("LIMIT FPS"), 12, true, Muted))
        ->SetPadding(FMargin(2, 0, 2, 8));

    const int32 Limit = Preferences ? Preferences->FPSLimit : 60;

    auto* Unlimited = MakeButton(TEXT("BEZ LIMITU"), Limit == 0);
    Unlimited->OnClicked.AddDynamic(this, &UPlayerMenuWidget::FPSUnlimited);
    ActionColumn->AddChildToVerticalBox(Unlimited)->SetPadding(FMargin(0, 0, 0, 5));

    auto* B30 = MakeButton(TEXT("30 FPS"), Limit == 30);
    B30->OnClicked.AddDynamic(this, &UPlayerMenuWidget::FPS30);
    ActionColumn->AddChildToVerticalBox(B30)->SetPadding(FMargin(0, 0, 0, 5));

    auto* B60 = MakeButton(TEXT("60 FPS"), Limit == 60);
    B60->OnClicked.AddDynamic(this, &UPlayerMenuWidget::FPS60);
    ActionColumn->AddChildToVerticalBox(B60)->SetPadding(FMargin(0, 0, 0, 5));

    auto* B90 = MakeButton(TEXT("90 FPS"), Limit == 90);
    B90->OnClicked.AddDynamic(this, &UPlayerMenuWidget::FPS90);
    ActionColumn->AddChildToVerticalBox(B90)->SetPadding(FMargin(0, 0, 0, 5));

    auto* B120 = MakeButton(TEXT("120 FPS"), Limit == 120);
    B120->OnClicked.AddDynamic(this, &UPlayerMenuWidget::FPS120);
    ActionColumn->AddChildToVerticalBox(B120)->SetPadding(FMargin(0, 0, 0, 5));

    auto* B144 = MakeButton(TEXT("144 FPS"), Limit == 144);
    B144->OnClicked.AddDynamic(this, &UPlayerMenuWidget::FPS144);
    ActionColumn->AddChildToVerticalBox(B144)->SetPadding(FMargin(0, 0, 0, 5));

    auto* B165 = MakeButton(TEXT("165 FPS"), Limit == 165);
    B165->OnClicked.AddDynamic(this, &UPlayerMenuWidget::FPS165);
    ActionColumn->AddChildToVerticalBox(B165)->SetPadding(FMargin(0, 0, 0, 5));

    auto* B240 = MakeButton(TEXT("240 FPS"), Limit == 240);
    B240->OnClicked.AddDynamic(this, &UPlayerMenuWidget::FPS240);
    ActionColumn->AddChildToVerticalBox(B240);

    const FString LimitText = Limit == 0 ? TEXT("bez limitu") : FString::Printf(TEXT("%d FPS"), Limit);
    const FString Description = FString::Printf(
        TEXT("LICZNIK FPS\n%s\n\nLIMIT KLATEK\n%s\n\nVSYNC\n%s\n\n")
        TEXT("Limit jest zapisywany w ustawieniach użytkownika i stosowany przy następnym uruchomieniu. ")
        TEXT("Przy włączonym VSync rzeczywista liczba FPS może być dodatkowo ograniczona częstotliwością monitora."),
        bFPSVisible ? TEXT("Włączony") : TEXT("Wyłączony"),
        *LimitText,
        bVSync ? TEXT("Włączony") : TEXT("Wyłączony"));
    AddTextPage(TEXT("WYDAJNOŚĆ"), Description);

    RightColumn->AddChildToVerticalBox(MakeText(TEXT("AKTYWNY PROFIL"), 13, true, Accent))
        ->SetPadding(FMargin(0, 0, 0, 10));

    if (UserSettings)
    {
        const FIntPoint Resolution = UserSettings->GetScreenResolution();
        const FString Status = FString::Printf(
            TEXT("Rozdzielczość: %d × %d\nTryb ekranu: %d\nVSync: %s\nLimit: %s\nLicznik FPS: %s"),
            Resolution.X, Resolution.Y,
            static_cast<int32>(UserSettings->GetFullscreenMode()),
            bVSync ? TEXT("Wł.") : TEXT("Wył."),
            *LimitText,
            bFPSVisible ? TEXT("Wł.") : TEXT("Wył."));
        RightColumn->AddChildToVerticalBox(MakeText(Status, 13, false, FLinearColor(0.9f, 0.92f, 0.95f)));
    }
}

void UPlayerMenuWidget::TabGame() { SelectTab(0); }
void UPlayerMenuWidget::TabCharacter() { SelectTab(1); }
void UPlayerMenuWidget::TabInventory() { SelectTab(2); }
void UPlayerMenuWidget::TabJournal() { SelectTab(3); }
void UPlayerMenuWidget::TabMap() { SelectTab(4); }
void UPlayerMenuWidget::TabStats() { SelectTab(5); }
void UPlayerMenuWidget::TabSettings() { SelectTab(6); }

void UPlayerMenuWidget::Resume()
{
    if (auto* Controller = Cast<ASliceController>(GetOwningPlayer()))
        Controller->ResumeGame();
}

void UPlayerMenuWidget::NewGame()
{
    if (auto* Controller = Cast<ASliceController>(GetOwningPlayer()))
        Controller->NewGame();
}

void UPlayerMenuWidget::LoadGame()
{
    if (auto* Controller = Cast<ASliceController>(GetOwningPlayer()))
        Controller->LoadGame();
}

void UPlayerMenuWidget::QuitGame()
{
    if (auto* Controller = Cast<ASliceController>(GetOwningPlayer()))
        Controller->Quit();
}

void UPlayerMenuWidget::ToggleFPSCounter()
{
    auto* Preferences = UWTGPerformanceSettings::Get();
    if (!Preferences)
        return;
    Preferences->SetShowFPS(!Preferences->bShowFPS);
    Refresh();
}

void UPlayerMenuWidget::ToggleVSync()
{
    if (GEngine)
    {
        if (UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
        {
            UserSettings->SetVSyncEnabled(!UserSettings->IsVSyncEnabled());
            UserSettings->ApplySettings(false);
            UserSettings->SaveSettings();
        }
    }
    Refresh();
}

void UPlayerMenuWidget::SetFPSLimit(int32 Limit)
{
    if (auto* Preferences = UWTGPerformanceSettings::Get())
        Preferences->SetFPSLimit(Limit);
    Refresh();
}

void UPlayerMenuWidget::FPSUnlimited() { SetFPSLimit(0); }
void UPlayerMenuWidget::FPS30() { SetFPSLimit(30); }
void UPlayerMenuWidget::FPS60() { SetFPSLimit(60); }
void UPlayerMenuWidget::FPS90() { SetFPSLimit(90); }
void UPlayerMenuWidget::FPS120() { SetFPSLimit(120); }
void UPlayerMenuWidget::FPS144() { SetFPSLimit(144); }
void UPlayerMenuWidget::FPS165() { SetFPSLimit(165); }
void UPlayerMenuWidget::FPS240() { SetFPSLimit(240); }

void UPlayerMenuWidget::PreviewFullBody()
{
    if (Studio) Studio->SetView(TEXT("FullBody"));
}

void UPlayerMenuWidget::PreviewUpperBody()
{
    if (Studio) Studio->SetView(TEXT("UpperBody"));
}

void UPlayerMenuWidget::PreviewFace()
{
    if (Studio) Studio->SetView(TEXT("Face"));
}

void UPlayerMenuWidget::PreviewRotateLeft()
{
    if (Studio) Studio->Rotate(-20);
}

void UPlayerMenuWidget::PreviewRotateRight()
{
    if (Studio) Studio->Rotate(20);
}

FReply UPlayerMenuWidget::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if ((ActiveTab == 0 || ActiveTab == 1) && Studio && Event.GetEffectingButton() == EKeys::RightMouseButton)
    {
        bRotatingPreview = true;
        return FReply::Handled().CaptureMouse(TakeWidget());
    }
    return Super::NativeOnMouseButtonDown(Geometry, Event);
}

FReply UPlayerMenuWidget::NativeOnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if (bRotatingPreview && Event.GetEffectingButton() == EKeys::RightMouseButton)
    {
        bRotatingPreview = false;
        return FReply::Handled().ReleaseMouseCapture();
    }
    return Super::NativeOnMouseButtonUp(Geometry, Event);
}

FReply UPlayerMenuWidget::NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if (bRotatingPreview && Studio)
    {
        Studio->Rotate(Event.GetCursorDelta().X * 0.35f);
        return FReply::Handled();
    }
    return Super::NativeOnMouseMove(Geometry, Event);
}

FReply UPlayerMenuWidget::NativeOnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if ((ActiveTab == 0 || ActiveTab == 1) && Studio)
    {
        Studio->Zoom(-Event.GetWheelDelta() * 18.0f);
        return FReply::Handled();
    }
    return Super::NativeOnMouseWheel(Geometry, Event);
}
