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
#include "Styling/SlateRoundedBoxBrush.h"

namespace
{
const FLinearColor Background(0.005f, 0.009f, 0.016f, 0.995f);
const FLinearColor Panel(0.020f, 0.030f, 0.044f, 0.985f);
const FLinearColor PanelSoft(0.034f, 0.049f, 0.069f, 0.98f);
const FLinearColor PanelHover(0.055f, 0.078f, 0.105f, 1.0f);
const FLinearColor Accent(0.18f, 0.79f, 0.96f, 1.0f);
const FLinearColor AccentHover(0.32f, 0.86f, 1.0f, 1.0f);
const FLinearColor AccentPressed(0.10f, 0.61f, 0.78f, 1.0f);
const FLinearColor AccentWarm(0.96f, 0.66f, 0.22f, 1.0f);
const FLinearColor TextPrimary(0.95f, 0.97f, 1.0f, 1.0f);
const FLinearColor Muted(0.57f, 0.64f, 0.72f, 1.0f);
const FLinearColor Divider(0.11f, 0.16f, 0.21f, 1.0f);
const FLinearColor Disabled(0.028f, 0.038f, 0.050f, 0.72f);
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
    Button->SetBackgroundColor(FLinearColor::White);
    Button->SetColorAndOpacity(FLinearColor::White);

    FButtonStyle Style = Button->GetStyle();
    Style.Normal = FSlateRoundedBoxBrush(bAccent ? Accent : PanelSoft, 9.0f);
    Style.Hovered = FSlateRoundedBoxBrush(bAccent ? AccentHover : PanelHover, 9.0f);
    Style.Pressed = FSlateRoundedBoxBrush(bAccent ? AccentPressed : Panel, 9.0f);
    Style.Disabled = FSlateRoundedBoxBrush(Disabled, 9.0f);
    Style.NormalPadding = FMargin(15.0f, 11.0f, 15.0f, 11.0f);
    Style.PressedPadding = FMargin(15.0f, 12.0f, 15.0f, 10.0f);
    Button->SetStyle(Style);

    auto* Text = MakeText(Label, 13, true, bAccent ? Background : TextPrimary);
    Text->SetJustification(ETextJustify::Center);
    Text->SetLineHeightPercentage(1.0f);
    Button->AddChild(Text);
    return Button;
}

UBorder* UPlayerMenuWidget::MakeCard(const FMargin& Padding)
{
    auto* Border = WidgetTree->ConstructWidget<UBorder>();
    Border->SetBrush(FSlateRoundedBoxBrush(Panel, 16.0f));
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
        Studio->SetLighting(TEXT("Modern"));
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

    // Fixed design canvas scaled as one unit keeps spacing and typography stable
    // from 1280x720 up to ultrawide/4K while the backdrop still fills the screen.
    auto* Scale = WidgetTree->ConstructWidget<UScaleBox>();
    Scale->SetStretch(EStretch::ScaleToFit);
    Scale->SetStretchDirection(EStretchDirection::Both);
    auto* ScaleSlot = Root->AddChildToOverlay(Scale);
    ScaleSlot->SetHorizontalAlignment(HAlign_Fill);
    ScaleSlot->SetVerticalAlignment(VAlign_Fill);

    auto* Frame = WidgetTree->ConstructWidget<USizeBox>();
    Frame->SetWidthOverride(1600.0f);
    Frame->SetHeightOverride(900.0f);
    Scale->AddChild(Frame);

    auto* Layout = WidgetTree->ConstructWidget<UVerticalBox>();
    Frame->AddChild(Layout);
    Layout->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

    auto* Header = MakeCard(FMargin(18, 14, 18, 14));
    auto* HeaderSlot = Layout->AddChildToVerticalBox(Header);
    HeaderSlot->SetPadding(FMargin(30, 24, 30, 0));

    auto* Top = WidgetTree->ConstructWidget<UHorizontalBox>();
    Header->AddChild(Top);

    auto* BrandSize = WidgetTree->ConstructWidget<USizeBox>();
    BrandSize->SetWidthOverride(325.0f);
    auto* Brand = WidgetTree->ConstructWidget<UVerticalBox>();
    BrandSize->AddChild(Brand);
    Top->AddChildToHorizontalBox(BrandSize)->SetPadding(FMargin(4, 0, 22, 0));

    auto* BrandTitle = MakeText(TEXT("WROCŁAW"), 27, true, TextPrimary);
    BrandTitle->SetLineHeightPercentage(0.9f);
    Brand->AddChildToVerticalBox(BrandTitle);
    Brand->AddChildToVerticalBox(MakeText(TEXT("THE GAME  /  PRZEBUDZENIE"), 10, true, Accent))
        ->SetPadding(FMargin(1, 3, 0, 0));

    const TCHAR* Labels[] = {TEXT("GRA"), TEXT("POSTAĆ"), TEXT("EKWIPUNEK"),
                             TEXT("DZIENNIK"), TEXT("MAPA"), TEXT("STATYSTYKI"), TEXT("USTAWIENIA")};
    for (int32 Index = 0; Index < 7; ++Index)
    {
        auto* Button = MakeButton(Labels[Index]);
        auto* Slot = Top->AddChildToHorizontalBox(Button);
        Slot->SetPadding(FMargin(3, 1, 3, 1));
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

    auto* ContextBar = WidgetTree->ConstructWidget<UHorizontalBox>();
    auto* ContextSlot = Layout->AddChildToVerticalBox(ContextBar);
    ContextSlot->SetPadding(FMargin(34, 13, 34, 12));

    PageTitle = MakeText(TEXT(""), 11, true, Accent);
    ContextBar->AddChildToHorizontalBox(PageTitle)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    auto* ContextHint = MakeText(TEXT("ESC  WRÓĆ DO GRY    •    PPM  OBRÓT    •    KÓŁKO  ZOOM"), 10, true, Muted);
    ContextHint->SetJustification(ETextJustify::Right);
    ContextBar->AddChildToHorizontalBox(ContextHint);

    auto* Body = WidgetTree->ConstructWidget<UHorizontalBox>();
    auto* BodySlot = Layout->AddChildToVerticalBox(Body);
    BodySlot->SetPadding(FMargin(30, 0, 30, 0));
    BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    auto* LeftCard = MakeCard(FMargin(18, 18, 18, 18));
    auto* LeftSize = WidgetTree->ConstructWidget<USizeBox>();
    LeftSize->SetWidthOverride(285.0f);
    LeftSize->AddChild(LeftCard);
    Body->AddChildToHorizontalBox(LeftSize)->SetPadding(FMargin(0, 0, 16, 0));
    ActionColumn = WidgetTree->ConstructWidget<UVerticalBox>();
    LeftCard->AddChild(ActionColumn);

    auto* CenterCard = MakeCard(FMargin(10, 10, 10, 10));
    auto* CenterSlot = Body->AddChildToHorizontalBox(CenterCard);
    CenterSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CenterSlot->SetPadding(FMargin(0, 0, 16, 0));
    CenterColumn = WidgetTree->ConstructWidget<UVerticalBox>();
    CenterCard->AddChild(CenterColumn);

    auto* RightCard = MakeCard(FMargin(20, 20, 20, 20));
    auto* RightSize = WidgetTree->ConstructWidget<USizeBox>();
    RightSize->SetWidthOverride(320.0f);
    RightSize->AddChild(RightCard);
    Body->AddChildToHorizontalBox(RightSize);
    RightColumn = WidgetTree->ConstructWidget<UVerticalBox>();
    RightCard->AddChild(RightColumn);

    auto* Footer = WidgetTree->ConstructWidget<UHorizontalBox>();
    auto* FooterSlot = Layout->AddChildToVerticalBox(Footer);
    FooterSlot->SetPadding(FMargin(34, 12, 34, 20));

    auto* Location = MakeText(TEXT("WROCŁAW  /  DOLNY ŚLĄSK"), 10, true, Muted);
    Footer->AddChildToHorizontalBox(Location)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    auto* FooterHint = MakeText(TEXT("CENTRUM GRACZA"), 10, true, Accent);
    FooterHint->SetJustification(ETextJustify::Right);
    Footer->AddChildToHorizontalBox(FooterHint);
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
    {
        auto* Button = TabButtons[Index];
        if (!Button)
            continue;

        const bool bActive = Index == ActiveTab;
        FButtonStyle Style = Button->GetStyle();
        Style.Normal = FSlateRoundedBoxBrush(bActive ? Accent : PanelSoft, 9.0f);
        Style.Hovered = FSlateRoundedBoxBrush(bActive ? AccentHover : PanelHover, 9.0f);
        Style.Pressed = FSlateRoundedBoxBrush(bActive ? AccentPressed : Panel, 9.0f);
        Button->SetStyle(Style);

        if (auto* Text = Cast<UTextBlock>(Button->GetContent()))
            Text->SetColorAndOpacity(FSlateColor(bActive ? Background : TextPrimary));
    }
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
    StageBg->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.008f, 0.014f, 0.024f, 1.0f), 14.0f));
    Stage->AddChildToOverlay(StageBg);

    auto* Scale = WidgetTree->ConstructWidget<UScaleBox>();
    Scale->SetStretch(EStretch::ScaleToFit);
    Scale->SetStretchDirection(EStretchDirection::Both);
    auto* ScaleSlot = Stage->AddChildToOverlay(Scale);
    ScaleSlot->SetPadding(FMargin(22, 20, 22, 70));
    ScaleSlot->SetHorizontalAlignment(HAlign_Center);
    ScaleSlot->SetVerticalAlignment(VAlign_Center);

    auto* Image = WidgetTree->ConstructWidget<UImage>();
    FSlateBrush Brush;
    Brush.SetResourceObject(Studio->RenderTarget);
    Brush.ImageSize = FVector2D(720, 1000);
    Image->SetBrush(Brush);
    Scale->AddChild(Image);

    auto* TopBadge = WidgetTree->ConstructWidget<UBorder>();
    TopBadge->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.015f, 0.024f, 0.036f, 0.92f), 8.0f));
    TopBadge->SetPadding(FMargin(11, 6, 11, 6));
    auto* TopBadgeSlot = Stage->AddChildToOverlay(TopBadge);
    TopBadgeSlot->SetHorizontalAlignment(HAlign_Left);
    TopBadgeSlot->SetVerticalAlignment(VAlign_Top);
    TopBadgeSlot->SetPadding(FMargin(18));
    TopBadge->AddChild(MakeText(TEXT("PODGLĄD NA ŻYWO"), 9, true, Accent));

    auto* CaptionCard = WidgetTree->ConstructWidget<UBorder>();
    CaptionCard->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.005f, 0.009f, 0.015f, 0.88f), 10.0f));
    CaptionCard->SetPadding(FMargin(16, 11, 16, 11));
    auto* CaptionSlot = Stage->AddChildToOverlay(CaptionCard);
    CaptionSlot->SetHorizontalAlignment(HAlign_Fill);
    CaptionSlot->SetVerticalAlignment(VAlign_Bottom);
    CaptionSlot->SetPadding(FMargin(18));

    auto* CaptionBox = WidgetTree->ConstructWidget<UVerticalBox>();
    CaptionCard->AddChild(CaptionBox);
    auto* Caption = MakeText(TEXT("AKTYWNA POSTAĆ"), 13, true, TextPrimary);
    Caption->SetJustification(ETextJustify::Center);
    CaptionBox->AddChildToVerticalBox(Caption);
    auto* CaptionSub = MakeText(TEXT("PPM — OBRÓT   •   KÓŁKO — ZOOM"), 9, true, Muted);
    CaptionSub->SetJustification(ETextJustify::Center);
    CaptionBox->AddChildToVerticalBox(CaptionSub)->SetPadding(FMargin(0, 3, 0, 0));
}

void UPlayerMenuWidget::AddTextPage(const FString& Heading, const FString& Body)
{
    auto* Eyebrow = MakeText(TEXT("CENTRUM INFORMACJI"), 9, true, Accent);
    CenterColumn->AddChildToVerticalBox(Eyebrow)->SetPadding(FMargin(10, 8, 10, 3));

    auto* Header = MakeText(Heading, 27, true, TextPrimary);
    CenterColumn->AddChildToVerticalBox(Header)->SetPadding(FMargin(10, 0, 10, 10));

    auto* Line = WidgetTree->ConstructWidget<UBorder>();
    Line->SetBrushColor(Divider);
    CenterColumn->AddChildToVerticalBox(Line)->SetPadding(FMargin(10, 0, 10, 14));

    auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
    auto* ScrollSlot = CenterColumn->AddChildToVerticalBox(Scroll);
    ScrollSlot->SetPadding(FMargin(10, 0, 10, 8));
    ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    auto* BodyText = MakeText(Body.IsEmpty() ? TEXT("Brak danych.") : Body, 14, false, TextPrimary);
    BodyText->SetLineHeightPercentage(1.22f);
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

    RightColumn->AddChildToVerticalBox(MakeText(TEXT("PROFIL"), 9, true, Accent))
        ->SetPadding(FMargin(0, 0, 0, 4));
    RightColumn->AddChildToVerticalBox(MakeText(Name.ToUpper(), 23, true, TextPrimary))
        ->SetPadding(FMargin(0, 0, 0, 2));
    RightColumn->AddChildToVerticalBox(MakeText(TEXT("WROCŁAW  /  DOLNY ŚLĄSK"), 10, true, Muted))
        ->SetPadding(FMargin(0, 0, 0, 18));

    if (Player && Player->HealthState && Player->StaminaState)
    {
        RightColumn->AddChildToVerticalBox(MakeText(TEXT("ZDROWIE"), 9, true, Muted));
        auto* Health = WidgetTree->ConstructWidget<UProgressBar>();
        Health->SetPercent(FMath::Clamp(Player->HealthState->Value / 100.0f, 0.0f, 1.0f));
        Health->SetFillColorAndOpacity(FLinearColor(0.91f, 0.29f, 0.28f, 1.0f));
        RightColumn->AddChildToVerticalBox(Health)->SetPadding(FMargin(0, 5, 0, 12));

        RightColumn->AddChildToVerticalBox(MakeText(TEXT("KONDYCJA"), 9, true, Muted));
        auto* Stamina = WidgetTree->ConstructWidget<UProgressBar>();
        Stamina->SetPercent(FMath::Clamp(Player->StaminaState->Value / 100.0f, 0.0f, 1.0f));
        Stamina->SetFillColorAndOpacity(Accent);
        RightColumn->AddChildToVerticalBox(Stamina)->SetPadding(FMargin(0, 5, 0, 18));
    }

    if (Mission)
    {
        auto* ObjectiveCard = WidgetTree->ConstructWidget<UBorder>();
        ObjectiveCard->SetBrush(FSlateRoundedBoxBrush(PanelSoft, 11.0f));
        ObjectiveCard->SetPadding(FMargin(13, 12, 13, 12));
        RightColumn->AddChildToVerticalBox(ObjectiveCard)->SetPadding(FMargin(0, 0, 0, 14));

        auto* ObjectiveBox = WidgetTree->ConstructWidget<UVerticalBox>();
        ObjectiveCard->AddChild(ObjectiveBox);
        ObjectiveBox->AddChildToVerticalBox(MakeText(TEXT("AKTUALNY CEL"), 9, true, Accent))
            ->SetPadding(FMargin(0, 0, 0, 6));
        ObjectiveBox->AddChildToVerticalBox(
            MakeText(Mission->ObjectiveText(), 13, true, TextPrimary));

        RightColumn->AddChildToVerticalBox(MakeText(TEXT("SESJA"), 9, true, Accent))
            ->SetPadding(FMargin(0, 0, 0, 7));
        const FString Meta = FString::Printf(
            TEXT("Zagrożenie        %d / 5\nCzas rozgrywki    %.0f min\nOsiągnięcia       %d\nZapis             %s"),
            Mission->WorldState.HeatLevel(),
            Mission->State.elapsed / 60.0,
            Mission->LifetimeAchievements,
            Mission->bLastSaveSucceeded ? TEXT("OK") : TEXT("BŁĄD"));
        auto* MetaText = MakeText(Meta, 11, false, Muted);
        MetaText->SetLineHeightPercentage(1.35f);
        RightColumn->AddChildToVerticalBox(MetaText);
    }
}

void UPlayerMenuWidget::BuildGameTab()
{
    PageTitle->SetText(FText::FromString(TEXT("CENTRUM GRACZA  /  GRA")));

    auto* Mission = GetGameInstance()->GetSubsystem<USliceMission>();
    const bool bInGame = Mission && Mission->bInGame;

    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("PRZEBUDZENIE"), 21, true, TextPrimary))
        ->SetPadding(FMargin(2, 1, 2, 2));
    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("KAMPANIA FABULARNA"), 9, true, Accent))
        ->SetPadding(FMargin(2, 0, 2, 12));
    ActionColumn->AddChildToVerticalBox(MakeText(
        bInGame ? TEXT("Wróć do bieżącej sesji albo zarządzaj zapisem.")
                : TEXT("Rozpocznij nową historię albo wczytaj istniejący zapis."),
        11, false, Muted))->SetPadding(FMargin(2, 0, 2, 18));

    auto* ResumeButton = MakeButton(bInGame ? TEXT("KONTYNUUJ") : TEXT("BRAK AKTYWNEJ SESJI"), bInGame);
    ResumeButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::Resume);
    ResumeButton->SetIsEnabled(bInGame);
    ActionColumn->AddChildToVerticalBox(ResumeButton)->SetPadding(FMargin(0, 0, 0, 8));

    auto* NewButton = MakeButton(TEXT("NOWA GRA"), !bInGame);
    NewButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::NewGame);
    ActionColumn->AddChildToVerticalBox(NewButton)->SetPadding(FMargin(0, 0, 0, 8));

    auto* LoadButton = MakeButton(TEXT("WCZYTAJ ZAPIS"));
    LoadButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::LoadGame);
    const bool CityActive = GetWorld()->GetSubsystem<UCityGameplaySubsystem>()->IsActive();
    LoadButton->SetIsEnabled(CityActive || (Mission && Mission->HasSave()));
    ActionColumn->AddChildToVerticalBox(LoadButton)->SetPadding(FMargin(0, 0, 0, 18));

    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("CENTRUM GRACZA"), 9, true, Muted))
        ->SetPadding(FMargin(2, 0, 2, 8));

    auto* CharacterButton = MakeButton(TEXT("POSTAĆ I PROFIL"));
    CharacterButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::TabCharacter);
    ActionColumn->AddChildToVerticalBox(CharacterButton)->SetPadding(FMargin(0, 0, 0, 8));

    auto* SettingsButton = MakeButton(TEXT("USTAWIENIA"));
    SettingsButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::TabSettings);
    ActionColumn->AddChildToVerticalBox(SettingsButton)->SetPadding(FMargin(0, 0, 0, 8));

    auto* QuitButton = MakeButton(TEXT("WYJDŹ Z GRY"));
    QuitButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::QuitGame);
    ActionColumn->AddChildToVerticalBox(QuitButton)->SetPadding(FMargin(0, 14, 0, 0));

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
