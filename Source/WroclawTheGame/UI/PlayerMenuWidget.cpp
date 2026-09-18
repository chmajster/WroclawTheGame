#include "UI/PlayerMenuWidget.h"

#include "UI/SliceController.h"
#include "UI/PerformanceSettings.h"
#include "Audio/SliceAudio.h"
#include "Character/CharacterCreator.h"
#include "Character/CharacterCreatorSubsystem.h"
#include "Character/CharacterAppearanceComponent.h"
#include "Character/SliceCharacter.h"
#include "Components/GameplayComponents.h"
#include "Mission/SliceMission.h"
#include "Systems/CityGameplaySubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
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
#include "Brushes/SlateRoundedBoxBrush.h"

namespace
{
const FLinearColor Background(0.005f, 0.009f, 0.016f, 0.79f);
const FLinearColor Panel(0.020f, 0.030f, 0.044f, 0.93f);
const FLinearColor PanelSoft(0.034f, 0.049f, 0.069f, 0.91f);
const FLinearColor PanelHover(0.055f, 0.078f, 0.105f, 1.0f);
const FLinearColor Accent(0.18f, 0.79f, 0.96f, 1.0f);
const FLinearColor AccentHover(0.32f, 0.86f, 1.0f, 1.0f);
const FLinearColor AccentPressed(0.10f, 0.61f, 0.78f, 1.0f);
const FLinearColor TextPrimary(0.95f, 0.97f, 1.0f, 1.0f);
const FLinearColor Muted(0.57f, 0.64f, 0.72f, 1.0f);
const FLinearColor Divider(0.11f, 0.16f, 0.21f, 1.0f);
const FLinearColor Disabled(0.028f, 0.038f, 0.050f, 0.72f);

FSlateRoundedBoxBrush RoundedBrush(const FLinearColor& Color, float Radius)
{
    return FSlateRoundedBoxBrush(Color, Radius, FVector2f(64.0f, 64.0f));
}

FString WindowModeLabel(EWindowMode::Type Mode)
{
    switch (Mode)
    {
        case EWindowMode::Fullscreen: return TEXT("PEŁNY EKRAN");
        case EWindowMode::WindowedFullscreen: return TEXT("BEZ RAMKI");
        case EWindowMode::Windowed: return TEXT("OKNO");
        default: return TEXT("NIEZNANY");
    }
}

FString QualityLabel(int32 Level)
{
    switch (Level)
    {
        case 0: return TEXT("NISKA");
        case 1: return TEXT("ŚREDNIA");
        case 2: return TEXT("WYSOKA");
        case 3: return TEXT("EPICKA");
        case 4: return TEXT("KINOWA");
        default: return TEXT("NIESTANDARDOWA");
    }
}
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
    Button->SetIsFocusable(true);
    Button->OnHovered.AddDynamic(this, &UPlayerMenuWidget::PlayUIHover);
    Button->OnClicked.AddDynamic(this, &UPlayerMenuWidget::PlayUIClick);
    Button->SetBackgroundColor(FLinearColor::White);
    Button->SetColorAndOpacity(FLinearColor::White);

    FButtonStyle Style = Button->GetStyle();
    Style.Normal = RoundedBrush(bAccent ? Accent : PanelSoft, 9.0f);
    Style.Hovered = RoundedBrush(bAccent ? AccentHover : PanelHover, 9.0f);
    Style.Pressed = RoundedBrush(bAccent ? AccentPressed : Panel, 9.0f);
    Style.Disabled = RoundedBrush(Disabled, 9.0f);
    Style.NormalPadding = FMargin(15.0f, 11.0f, 15.0f, 11.0f);
    Style.PressedPadding = FMargin(15.0f, 12.0f, 15.0f, 10.0f);
    Button->SetStyle(Style);

    auto* Text = MakeText(Label, 13, true, bAccent ? Background : TextPrimary);
    Text->SetJustification(ETextJustify::Center);
    Text->SetLineHeightPercentage(1.0f);
    Button->AddChild(Text);
    if (bCollectActionButtons)
        ActionButtons.Add(Button);
    return Button;
}

UBorder* UPlayerMenuWidget::MakeCard(const FMargin& Padding)
{
    auto* Border = WidgetTree->ConstructWidget<UBorder>();
    Border->SetBrush(RoundedBrush(Panel, 16.0f));
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

UBorder* UPlayerMenuWidget::MakeInfoRow(const FString& Title, const FString& Subtitle, bool bHighlighted)
{
    auto* Row = WidgetTree->ConstructWidget<UBorder>();
    Row->SetBrush(RoundedBrush(
        bHighlighted ? FLinearColor(0.025f, 0.105f, 0.135f, 0.98f) : PanelSoft, 10.0f));
    Row->SetPadding(FMargin(12, 10, 12, 10));

    auto* Column = WidgetTree->ConstructWidget<UVerticalBox>();
    Row->AddChild(Column);

    Column->AddChildToVerticalBox(MakeText(Title, 13, true, TextPrimary));
    if (!Subtitle.IsEmpty())
        Column->AddChildToVerticalBox(MakeText(Subtitle, 9, true, bHighlighted ? Accent : Muted))
            ->SetPadding(FMargin(0, 3, 0, 0));

    return Row;
}

void UPlayerMenuWidget::BuildShell()
{
    RootOverlay = WidgetTree->ConstructWidget<UOverlay>();
    WidgetTree->RootWidget = RootOverlay;

    auto* WorldBlur = WidgetTree->ConstructWidget<UBackgroundBlur>();
    WorldBlur->SetBlurStrength(12.0f);
    WorldBlur->SetBlurRadius(18);
    WorldBlur->SetApplyAlphaToBlur(false);
    auto* BlurFill = WidgetTree->ConstructWidget<UBorder>();
    BlurFill->SetBrushColor(FLinearColor(0, 0, 0, 0.01f));
    WorldBlur->AddChild(BlurFill);
    auto* BlurSlot = RootOverlay->AddChildToOverlay(WorldBlur);
    BlurSlot->SetHorizontalAlignment(HAlign_Fill);
    BlurSlot->SetVerticalAlignment(VAlign_Fill);

    auto* Backdrop = WidgetTree->ConstructWidget<UBorder>();
    Backdrop->SetBrushColor(Background);
    auto* BackdropSlot = RootOverlay->AddChildToOverlay(Backdrop);
    BackdropSlot->SetHorizontalAlignment(HAlign_Fill);
    BackdropSlot->SetVerticalAlignment(VAlign_Fill);

    AmbientGlowA = WidgetTree->ConstructWidget<UBorder>();
    AmbientGlowA->SetBrush(RoundedBrush(FLinearColor(0.10f, 0.65f, 0.82f, 0.055f), 240.0f));
    auto* GlowASize = WidgetTree->ConstructWidget<USizeBox>();
    GlowASize->SetWidthOverride(480.0f);
    GlowASize->SetHeightOverride(480.0f);
    GlowASize->AddChild(AmbientGlowA);
    auto* GlowASlot = RootOverlay->AddChildToOverlay(GlowASize);
    GlowASlot->SetHorizontalAlignment(HAlign_Right);
    GlowASlot->SetVerticalAlignment(VAlign_Top);
    GlowASlot->SetPadding(FMargin(0, -140, -120, 0));

    AmbientGlowB = WidgetTree->ConstructWidget<UBorder>();
    AmbientGlowB->SetBrush(RoundedBrush(FLinearColor(0.16f, 0.35f, 0.50f, 0.045f), 190.0f));
    auto* GlowBSize = WidgetTree->ConstructWidget<USizeBox>();
    GlowBSize->SetWidthOverride(380.0f);
    GlowBSize->SetHeightOverride(380.0f);
    GlowBSize->AddChild(AmbientGlowB);
    auto* GlowBSlot = RootOverlay->AddChildToOverlay(GlowBSize);
    GlowBSlot->SetHorizontalAlignment(HAlign_Left);
    GlowBSlot->SetVerticalAlignment(VAlign_Bottom);
    GlowBSlot->SetPadding(FMargin(-110, 0, 0, -120));

    auto* TopAccent = WidgetTree->ConstructWidget<UBorder>();
    TopAccent->SetBrushColor(FLinearColor(Accent.R, Accent.G, Accent.B, 0.62f));
    auto* TopAccentSize = WidgetTree->ConstructWidget<USizeBox>();
    TopAccentSize->SetHeightOverride(3.0f);
    TopAccentSize->AddChild(TopAccent);
    auto* AccentSlot = RootOverlay->AddChildToOverlay(TopAccentSize);
    AccentSlot->SetHorizontalAlignment(HAlign_Fill);
    AccentSlot->SetVerticalAlignment(VAlign_Top);

    // Fixed design canvas scaled as one unit keeps spacing and typography stable
    // from 1280x720 up to ultrawide/4K while the backdrop still fills the screen.
    auto* Scale = WidgetTree->ConstructWidget<UScaleBox>();
    Scale->SetStretch(EStretch::ScaleToFit);
    Scale->SetStretchDirection(EStretchDirection::Both);
    auto* ScaleSlot = RootOverlay->AddChildToOverlay(Scale);
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

    auto* ContextHint = MakeText(TEXT("Q/E  L1/R1  ZAKŁADKI    •    ENTER/A  WYBIERZ    •    ESC/B  WSTECZ"), 10, true, Muted);
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

    auto* FooterHint = MakeText(TEXT("MYSZ  •  KLAWIATURA  •  GAMEPAD"), 10, true, Accent);
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
    ActionButtons.Reset();
    UpdateTabStyle();

    bCollectActionButtons = true;
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
    bCollectActionButtons = false;

    PageAnimationTime = 0.0f;
    ActionColumn->SetRenderOpacity(0.0f);
    CenterColumn->SetRenderOpacity(0.0f);
    RightColumn->SetRenderOpacity(0.0f);
    ActionColumn->SetRenderTranslation(FVector2D(-12.0f, 0.0f));
    CenterColumn->SetRenderTranslation(FVector2D(0.0f, 8.0f));
    RightColumn->SetRenderTranslation(FVector2D(12.0f, 0.0f));

    FocusPrimaryAction();
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
        Style.Normal = RoundedBrush(bActive ? Accent : PanelSoft, 9.0f);
        Style.Hovered = RoundedBrush(bActive ? AccentHover : PanelHover, 9.0f);
        Style.Pressed = RoundedBrush(bActive ? AccentPressed : Panel, 9.0f);
        Button->SetStyle(Style);

        if (auto* Text = Cast<UTextBlock>(Button->GetContent()))
            Text->SetColorAndOpacity(FSlateColor(bActive ? Background : TextPrimary));
    }
}

void UPlayerMenuWidget::FocusPrimaryAction()
{
    if (PendingConfirmation != 0)
        return;

    for (UButton* Button : ActionButtons)
    {
        if (Button && Button->GetIsEnabled() && Button->GetVisibility() == ESlateVisibility::Visible)
        {
            if (APlayerController* PlayerController = GetOwningPlayer())
                Button->SetUserFocus(PlayerController);
            else
                Button->SetKeyboardFocus();
            return;
        }
    }

    if (TabButtons.IsValidIndex(ActiveTab) && TabButtons[ActiveTab])
    {
        if (APlayerController* PlayerController = GetOwningPlayer())
            TabButtons[ActiveTab]->SetUserFocus(PlayerController);
        else
            TabButtons[ActiveTab]->SetKeyboardFocus();
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
    StageBg->SetBrush(RoundedBrush(FLinearColor(0.008f, 0.014f, 0.024f, 1.0f), 14.0f));
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
    TopBadge->SetBrush(RoundedBrush(FLinearColor(0.015f, 0.024f, 0.036f, 0.92f), 8.0f));
    TopBadge->SetPadding(FMargin(11, 6, 11, 6));
    auto* TopBadgeSlot = Stage->AddChildToOverlay(TopBadge);
    TopBadgeSlot->SetHorizontalAlignment(HAlign_Left);
    TopBadgeSlot->SetVerticalAlignment(VAlign_Top);
    TopBadgeSlot->SetPadding(FMargin(18));
    TopBadge->AddChild(MakeText(TEXT("PODGLĄD NA ŻYWO"), 9, true, Accent));

    auto* CaptionCard = WidgetTree->ConstructWidget<UBorder>();
    CaptionCard->SetBrush(RoundedBrush(FLinearColor(0.005f, 0.009f, 0.015f, 0.88f), 10.0f));
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
        ObjectiveCard->SetBrush(RoundedBrush(PanelSoft, 11.0f));
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
    PageTitle->SetText(FText::FromString(TEXT("POSTAĆ  /  PODGLĄD")));

    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("PODGLĄD"), 20, true, TextPrimary))
        ->SetPadding(FMargin(2, 1, 2, 2));
    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("KAMERA I ŚWIATŁO"), 9, true, Accent))
        ->SetPadding(FMargin(2, 0, 2, 14));

    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("KADR"), 9, true, Muted))
        ->SetPadding(FMargin(2, 0, 2, 7));

    auto* Full = MakeButton(TEXT("CAŁA SYLWETKA"), true);
    Full->OnClicked.AddDynamic(this, &UPlayerMenuWidget::PreviewFullBody);
    ActionColumn->AddChildToVerticalBox(Full)->SetPadding(FMargin(0, 0, 0, 7));

    auto* Upper = MakeButton(TEXT("GÓRNA CZĘŚĆ"));
    Upper->OnClicked.AddDynamic(this, &UPlayerMenuWidget::PreviewUpperBody);
    ActionColumn->AddChildToVerticalBox(Upper)->SetPadding(FMargin(0, 0, 0, 7));

    auto* Face = MakeButton(TEXT("TWARZ"));
    Face->OnClicked.AddDynamic(this, &UPlayerMenuWidget::PreviewFace);
    ActionColumn->AddChildToVerticalBox(Face)->SetPadding(FMargin(0, 0, 0, 14));

    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("ŚWIATŁO"), 9, true, Muted))
        ->SetPadding(FMargin(2, 0, 2, 7));

    auto* Modern = MakeButton(TEXT("STUDIO"), true);
    Modern->OnClicked.AddDynamic(this, &UPlayerMenuWidget::PreviewLightingModern);
    ActionColumn->AddChildToVerticalBox(Modern)->SetPadding(FMargin(0, 0, 0, 7));

    auto* Day = MakeButton(TEXT("DZIEŃ"));
    Day->OnClicked.AddDynamic(this, &UPlayerMenuWidget::PreviewLightingDaylight);
    ActionColumn->AddChildToVerticalBox(Day)->SetPadding(FMargin(0, 0, 0, 7));

    auto* Night = MakeButton(TEXT("NOC"));
    Night->OnClicked.AddDynamic(this, &UPlayerMenuWidget::PreviewLightingNight);
    ActionColumn->AddChildToVerticalBox(Night)->SetPadding(FMargin(0, 0, 0, 14));

    auto* Reset = MakeButton(TEXT("RESET PODGLĄDU"));
    Reset->OnClicked.AddDynamic(this, &UPlayerMenuWidget::PreviewReset);
    ActionColumn->AddChildToVerticalBox(Reset);

    AddPreview();

    auto* Creator = GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>();
    if (!Creator)
        return;

    const auto& A = Creator->Committed.PlayerAppearanceData;
    const FString Sex = A.Sex == EAppearanceSex::Female ? TEXT("KOBIETA") : TEXT("MĘŻCZYZNA");
    const FString Name = A.Name.IsEmpty() ? TEXT("GRACZ") : A.Name.ToUpper();

    RightColumn->AddChildToVerticalBox(MakeText(TEXT("PROFIL POSTACI"), 9, true, Accent))
        ->SetPadding(FMargin(0, 0, 0, 4));
    RightColumn->AddChildToVerticalBox(MakeText(Name, 23, true, TextPrimary))
        ->SetPadding(FMargin(0, 0, 0, 14));

    RightColumn->AddChildToVerticalBox(MakeInfoRow(Sex, TEXT("PŁEĆ"), true))
        ->SetPadding(FMargin(0, 0, 0, 6));
    RightColumn->AddChildToVerticalBox(MakeInfoRow(
        FString::Printf(TEXT("%.0f CM"), A.Height), TEXT("WZROST")))
        ->SetPadding(FMargin(0, 0, 0, 6));
    RightColumn->AddChildToVerticalBox(MakeInfoRow(
        FString::Printf(TEXT("%.0f"), A.VisualAge), TEXT("WIEK WIZUALNY")))
        ->SetPadding(FMargin(0, 0, 0, 6));
    RightColumn->AddChildToVerticalBox(MakeInfoRow(
        A.BodyBuild.ToString().ToUpper(), TEXT("SYLWETKA")))
        ->SetPadding(FMargin(0, 0, 0, 6));
    RightColumn->AddChildToVerticalBox(MakeInfoRow(
        A.VoiceProfileID.ToString().ToUpper(), TEXT("PROFIL GŁOSU")))
        ->SetPadding(FMargin(0, 0, 0, 6));
    RightColumn->AddChildToVerticalBox(MakeInfoRow(
        A.PresetID.ToString().ToUpper(), TEXT("PRESET")))
        ->SetPadding(FMargin(0, 0, 0, 6));
    RightColumn->AddChildToVerticalBox(MakeInfoRow(
        FString::Printf(TEXT("%d"), A.RandomSeed), TEXT("SEED WYGLĄDU")));
}

void UPlayerMenuWidget::BuildInventoryTab()
{
    PageTitle->SetText(FText::FromString(TEXT("EKWIPUNEK  /  ZASOBY")));

    auto* Mission = GetGameInstance()->GetSubsystem<USliceMission>();
    auto* Creator = GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>();

    int32 ItemTypes = 0;
    int32 ItemCount = 0;
    if (Mission)
    {
        for (const auto& Pair : Mission->State.inventory)
            if (Pair.second > 0)
            {
                ++ItemTypes;
                ItemCount += Pair.second;
            }
    }
    const int32 ClothingCount = Creator ? Creator->Committed.OwnedClothing.Num() : 0;

    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("EKWIPUNEK"), 20, true, TextPrimary))
        ->SetPadding(FMargin(2, 1, 2, 2));
    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("AKTUALNE ZASOBY"), 9, true, Accent))
        ->SetPadding(FMargin(2, 0, 2, 14));
    ActionColumn->AddChildToVerticalBox(MakeInfoRow(
        FString::Printf(TEXT("%d"), ItemCount), TEXT("PRZEDMIOTÓW ŁĄCZNIE"), true))
        ->SetPadding(FMargin(0, 0, 0, 7));
    ActionColumn->AddChildToVerticalBox(MakeInfoRow(
        FString::Printf(TEXT("%d"), ItemTypes), TEXT("TYPÓW PRZEDMIOTÓW")))
        ->SetPadding(FMargin(0, 0, 0, 7));
    ActionColumn->AddChildToVerticalBox(MakeInfoRow(
        FString::Printf(TEXT("%d"), ClothingCount), TEXT("ELEMENTÓW GARDEROBY")));

    auto* Header = MakeText(TEXT("ZAWARTOŚĆ"), 26, true, TextPrimary);
    CenterColumn->AddChildToVerticalBox(Header)->SetPadding(FMargin(8, 7, 8, 12));

    auto* Columns = WidgetTree->ConstructWidget<UHorizontalBox>();
    auto* ColumnsSlot = CenterColumn->AddChildToVerticalBox(Columns);
    ColumnsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    auto* Wardrobe = MakeCard(FMargin(14));
    auto* WardrobeSlot = Columns->AddChildToHorizontalBox(Wardrobe);
    WardrobeSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    WardrobeSlot->SetPadding(FMargin(0, 0, 6, 0));
    auto* WardrobeBox = WidgetTree->ConstructWidget<UVerticalBox>();
    Wardrobe->AddChild(WardrobeBox);
    WardrobeBox->AddChildToVerticalBox(MakeText(TEXT("GARDEROBA"), 10, true, Accent))
        ->SetPadding(FMargin(0, 0, 0, 9));
    auto* ClothesScroll = WidgetTree->ConstructWidget<UScrollBox>();
    WardrobeBox->AddChildToVerticalBox(ClothesScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    if (Creator && !Creator->Committed.OwnedClothing.IsEmpty())
    {
        for (const FName& Id : Creator->Committed.OwnedClothing)
            ClothesScroll->AddChild(MakeInfoRow(Id.ToString().ToUpper(), TEXT("ELEMENT UBIORU")));
    }
    else
        ClothesScroll->AddChild(MakeText(TEXT("Brak zapisanych elementów garderoby."), 11, false, Muted));

    auto* Items = MakeCard(FMargin(14));
    auto* ItemsSlot = Columns->AddChildToHorizontalBox(Items);
    ItemsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ItemsSlot->SetPadding(FMargin(6, 0, 0, 0));
    auto* ItemsBox = WidgetTree->ConstructWidget<UVerticalBox>();
    Items->AddChild(ItemsBox);
    ItemsBox->AddChildToVerticalBox(MakeText(TEXT("PRZEDMIOTY"), 10, true, Accent))
        ->SetPadding(FMargin(0, 0, 0, 9));
    auto* ItemsScroll = WidgetTree->ConstructWidget<UScrollBox>();
    ItemsBox->AddChildToVerticalBox(ItemsScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    bool bAnyItem = false;
    if (Mission)
    {
        for (const auto& Pair : Mission->State.inventory)
        {
            if (Pair.second <= 0)
                continue;
            bAnyItem = true;
            const auto* Item = Wroclaw::Progress::Item(Pair.first);
            const FString Name = Item ? UTF8_TO_TCHAR(Item->name.c_str()) : UTF8_TO_TCHAR(Pair.first.c_str());
            ItemsScroll->AddChild(MakeInfoRow(
                Name.ToUpper(), FString::Printf(TEXT("ILOŚĆ  × %d"), Pair.second), Pair.second > 1));
        }
    }
    if (!bAnyItem)
        ItemsScroll->AddChild(MakeText(TEXT("Brak przedmiotów w ekwipunku."), 11, false, Muted));

    AddPlayerStatus();
}

void UPlayerMenuWidget::BuildJournalTab()
{
    PageTitle->SetText(FText::FromString(TEXT("DZIENNIK  /  POSTĘP")));

    auto* Mission = GetGameInstance()->GetSubsystem<USliceMission>();
    auto* City = GetWorld()->GetSubsystem<UCityGameplaySubsystem>();

    const int32 TotalMain = static_cast<int32>(Wroclaw::Quests().size());
    int32 MainDone = 0;
    int32 SideDone = 0;
    if (Mission)
    {
        for (const auto& Quest : Wroclaw::Quests())
            if (Mission->State.QuestComplete(Quest))
                ++MainDone;
        Wroclaw::QuestFramework Framework;
        for (const auto& Quest : Wroclaw::SideQuests())
            if (Framework.Complete(Quest, Mission->State))
                ++SideDone;
    }

    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("DZIENNIK"), 20, true, TextPrimary))
        ->SetPadding(FMargin(2, 1, 2, 2));
    ActionColumn->AddChildToVerticalBox(MakeText(
        City->IsActive() ? TEXT("TRYB MIASTA") : TEXT("KAMPANIA"), 9, true, Accent))
        ->SetPadding(FMargin(2, 0, 2, 14));
    ActionColumn->AddChildToVerticalBox(MakeInfoRow(
        FString::Printf(TEXT("%d / %d"), MainDone, TotalMain), TEXT("ETAPY GŁÓWNE"), true))
        ->SetPadding(FMargin(0, 0, 0, 7));
    ActionColumn->AddChildToVerticalBox(MakeInfoRow(
        FString::Printf(TEXT("%d / %d"), SideDone, static_cast<int32>(Wroclaw::SideQuests().size())),
        TEXT("ZADANIA POBOCZNE")))
        ->SetPadding(FMargin(0, 0, 0, 7));
    ActionColumn->AddChildToVerticalBox(MakeInfoRow(
        Mission ? FString::Printf(TEXT("%d"), static_cast<int32>(Mission->State.evidence.size())) : TEXT("0"),
        TEXT("ZEBRANE DOWODY")));

    CenterColumn->AddChildToVerticalBox(MakeText(TEXT("POSTĘP FABULARNY"), 26, true, TextPrimary))
        ->SetPadding(FMargin(8, 7, 8, 8));

    if (Mission)
    {
        CenterColumn->AddChildToVerticalBox(MakeInfoRow(
            Mission->ObjectiveText(), TEXT("AKTUALNY CEL"), true))
            ->SetPadding(FMargin(8, 0, 8, 12));
    }

    auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
    auto* ScrollSlot = CenterColumn->AddChildToVerticalBox(Scroll);
    ScrollSlot->SetPadding(FMargin(8, 0, 8, 8));
    ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    if (City->IsActive())
    {
        Scroll->AddChild(MakeInfoRow(TEXT("AKTYWNOŚCI DZIELNIC"), City->Journal(), true));
    }
    else if (Mission)
    {
        int32 Index = 0;
        const int32 Current = Mission->State.Current();
        for (const auto& Quest : Wroclaw::Quests())
        {
            const bool bDone = Mission->State.QuestComplete(Quest);
            const bool bCurrent = !bDone && Index == Current;
            Scroll->AddChild(MakeInfoRow(
                FString::Printf(TEXT("%02d  %s"), Index + 1, UTF8_TO_TCHAR(Quest.title.c_str())),
                bDone ? TEXT("UKOŃCZONO") : (bCurrent ? TEXT("AKTUALNY ETAP") : TEXT("DO WYKONANIA")),
                bCurrent));
            ++Index;
        }

        Scroll->AddChild(MakeText(TEXT("ZADANIA POBOCZNE"), 10, true, Accent))
            ->SetPadding(FMargin(2, 16, 2, 8));

        Wroclaw::QuestFramework Framework;
        for (const auto& Quest : Wroclaw::SideQuests())
        {
            const bool bDone = Framework.Complete(Quest, Mission->State);
            Scroll->AddChild(MakeInfoRow(
                UTF8_TO_TCHAR(Quest.title.c_str()),
                bDone ? TEXT("UKOŃCZONO") : TEXT("OPCJONALNE")));
        }
    }

    RightColumn->AddChildToVerticalBox(MakeText(TEXT("PODPOWIEDŹ"), 9, true, Accent))
        ->SetPadding(FMargin(0, 0, 0, 7));
    RightColumn->AddChildToVerticalBox(MakeInfoRow(
        Mission ? Mission->HintText() : TEXT("Brak aktywnej kampanii."),
        TEXT("BEZ UJAWNIANIA ROZWIĄZANIA")))
        ->SetPadding(FMargin(0, 0, 0, 14));

    if (Mission)
    {
        RightColumn->AddChildToVerticalBox(MakeText(TEXT("STATUS"), 9, true, Accent))
            ->SetPadding(FMargin(0, 0, 0, 7));
        RightColumn->AddChildToVerticalBox(MakeText(
            FString::Printf(TEXT("Czas sesji: %.0f min\nZagrożenie: %d / 5\nZapis: %s"),
                Mission->State.elapsed / 60.0,
                Mission->WorldState.HeatLevel(),
                Mission->bLastSaveSucceeded ? TEXT("OK") : TEXT("BŁĄD")),
            11, false, Muted));
    }
}

void UPlayerMenuWidget::BuildMapTab()
{
    PageTitle->SetText(FText::FromString(TEXT("MAPA  /  WROCŁAW")));

    auto* Mission = GetGameInstance()->GetSubsystem<USliceMission>();
    const auto& Locations = Wroclaw::Locations();

    int32 Discovered = 0;
    for (const auto& Location : Locations)
        if (Mission && Mission->WorldState.discoveries.count(Location.id))
            ++Discovered;

    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("WROCŁAW"), 20, true, TextPrimary))
        ->SetPadding(FMargin(2, 1, 2, 2));
    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("ODKRYWANIE ŚWIATA"), 9, true, Accent))
        ->SetPadding(FMargin(2, 0, 2, 14));
    ActionColumn->AddChildToVerticalBox(MakeInfoRow(
        FString::Printf(TEXT("%d / %d"), Discovered, static_cast<int32>(Locations.size())),
        TEXT("ODKRYTE MIEJSCA"), true))
        ->SetPadding(FMargin(0, 0, 0, 7));
    ActionColumn->AddChildToVerticalBox(MakeInfoRow(TEXT("WYŁĄCZONA"), TEXT("SZYBKA PODRÓŻ")))
        ->SetPadding(FMargin(0, 0, 0, 7));
    ActionColumn->AddChildToVerticalBox(MakeInfoRow(TEXT("GPS"), TEXT("NAWIGACJA W ROZWOJU")));

    CenterColumn->AddChildToVerticalBox(MakeText(TEXT("MAPA ODKRYĆ"), 26, true, TextPrimary))
        ->SetPadding(FMargin(8, 7, 8, 8));

    constexpr float MapWidth = 760.0f;
    constexpr float MapHeight = 470.0f;

    auto* MapSize = WidgetTree->ConstructWidget<USizeBox>();
    MapSize->SetWidthOverride(MapWidth);
    MapSize->SetHeightOverride(MapHeight);
    auto* MapSizeSlot = CenterColumn->AddChildToVerticalBox(MapSize);
    MapSizeSlot->SetHorizontalAlignment(HAlign_Center);
    MapSizeSlot->SetVerticalAlignment(VAlign_Center);
    MapSizeSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    auto* MapFrame = WidgetTree->ConstructWidget<UBorder>();
    MapFrame->SetBrush(RoundedBrush(FLinearColor(0.008f, 0.018f, 0.027f, 1.0f), 14.0f));
    MapFrame->SetPadding(FMargin(12));
    MapSize->AddChild(MapFrame);

    auto* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    MapFrame->AddChild(Canvas);

    for (int32 I = 1; I < 6; ++I)
    {
        auto* Vertical = WidgetTree->ConstructWidget<UBorder>();
        Vertical->SetBrushColor(FLinearColor(0.12f, 0.18f, 0.23f, 0.28f));
        auto* VSlot = Canvas->AddChildToCanvas(Vertical);
        VSlot->SetPosition(FVector2D(MapWidth * I / 6.0f, 0.0f));
        VSlot->SetSize(FVector2D(1.0f, MapHeight));

        auto* Horizontal = WidgetTree->ConstructWidget<UBorder>();
        Horizontal->SetBrushColor(FLinearColor(0.12f, 0.18f, 0.23f, 0.28f));
        auto* HSlot = Canvas->AddChildToCanvas(Horizontal);
        HSlot->SetPosition(FVector2D(0.0f, MapHeight * I / 6.0f));
        HSlot->SetSize(FVector2D(MapWidth, 1.0f));
    }

    if (!Locations.empty())
    {
        double MinX = Locations.front().position[0];
        double MaxX = MinX;
        double MinY = Locations.front().position[1];
        double MaxY = MinY;
        for (const auto& Location : Locations)
        {
            MinX = FMath::Min(MinX, static_cast<double>(Location.position[0]));
            MaxX = FMath::Max(MaxX, static_cast<double>(Location.position[0]));
            MinY = FMath::Min(MinY, static_cast<double>(Location.position[1]));
            MaxY = FMath::Max(MaxY, static_cast<double>(Location.position[1]));
        }
        const double SpanX = FMath::Max(MaxX - MinX, 1.0);
        const double SpanY = FMath::Max(MaxY - MinY, 1.0);

        for (const auto& Location : Locations)
        {
            if (!Mission || !Mission->WorldState.discoveries.count(Location.id))
                continue;

            const float X = 28.0f + static_cast<float>((Location.position[0] - MinX) / SpanX) * (MapWidth - 80.0f);
            const float Y = 24.0f + (1.0f - static_cast<float>((Location.position[1] - MinY) / SpanY)) * (MapHeight - 60.0f);

            auto* Marker = WidgetTree->ConstructWidget<UBorder>();
            Marker->SetBrush(RoundedBrush(
                Location.safehouse ? FLinearColor(0.06f, 0.30f, 0.34f, 0.98f)
                                   : FLinearColor(0.04f, 0.08f, 0.11f, 0.95f), 7.0f));
            Marker->SetPadding(FMargin(8, 5, 8, 5));
            Marker->AddChild(MakeText(
                UTF8_TO_TCHAR(Location.name.c_str()), 9, true,
                Location.safehouse ? Accent : TextPrimary));

            auto* MarkerSlot = Canvas->AddChildToCanvas(Marker);
            MarkerSlot->SetAutoSize(true);
            MarkerSlot->SetAlignment(FVector2D(0.5f, 0.5f));
            MarkerSlot->SetPosition(FVector2D(X, Y));
        }
    }

    RightColumn->AddChildToVerticalBox(MakeText(TEXT("LEGENDA"), 9, true, Accent))
        ->SetPadding(FMargin(0, 0, 0, 7));
    RightColumn->AddChildToVerticalBox(MakeInfoRow(TEXT("TURKUSOWY"), TEXT("BEZPIECZNY PUNKT"), true))
        ->SetPadding(FMargin(0, 0, 0, 7));
    RightColumn->AddChildToVerticalBox(MakeInfoRow(TEXT("JASNY"), TEXT("ODKRYTA LOKACJA")))
        ->SetPadding(FMargin(0, 0, 0, 14));

    if (Mission)
    {
        RightColumn->AddChildToVerticalBox(MakeText(TEXT("AKTUALNY CEL"), 9, true, Accent))
            ->SetPadding(FMargin(0, 0, 0, 7));
        RightColumn->AddChildToVerticalBox(MakeInfoRow(Mission->ObjectiveText(), TEXT("KAMPANIA")));
    }
}

void UPlayerMenuWidget::BuildStatsTab()
{
    PageTitle->SetText(FText::FromString(TEXT("STATYSTYKI  /  SESJA")));

    auto* Mission = GetGameInstance()->GetSubsystem<USliceMission>();

    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("SESJA"), 20, true, TextPrimary))
        ->SetPadding(FMargin(2, 1, 2, 2));
    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("PODSUMOWANIE"), 9, true, Accent))
        ->SetPadding(FMargin(2, 0, 2, 14));

    if (!Mission)
    {
        ActionColumn->AddChildToVerticalBox(MakeInfoRow(TEXT("BRAK"), TEXT("AKTYWNEJ SESJI")));
        AddTextPage(TEXT("STATYSTYKI"), TEXT("Brak danych."));
        return;
    }

    int32 MainDone = 0;
    for (const auto& Quest : Wroclaw::Quests())
        if (Mission->State.QuestComplete(Quest))
            ++MainDone;

    ActionColumn->AddChildToVerticalBox(MakeInfoRow(
        Mission->bInGame ? TEXT("AKTYWNA") : TEXT("MENU"), TEXT("STATUS"), Mission->bInGame))
        ->SetPadding(FMargin(0, 0, 0, 7));
    ActionColumn->AddChildToVerticalBox(MakeInfoRow(
        Mission->bLastSaveSucceeded ? TEXT("POPRAWNY") : TEXT("BŁĄD"), TEXT("OSTATNI ZAPIS"),
        Mission->bLastSaveSucceeded))
        ->SetPadding(FMargin(0, 0, 0, 7));
    ActionColumn->AddChildToVerticalBox(MakeInfoRow(
        FString::Printf(TEXT("%d / 5"), Mission->WorldState.HeatLevel()), TEXT("ZAGROŻENIE")));

    CenterColumn->AddChildToVerticalBox(MakeText(TEXT("METRYKI"), 26, true, TextPrimary))
        ->SetPadding(FMargin(8, 7, 8, 10));

    auto* Metrics = WidgetTree->ConstructWidget<UVerticalBox>();
    CenterColumn->AddChildToVerticalBox(Metrics)->SetPadding(FMargin(8, 0, 8, 8));

    Metrics->AddChildToVerticalBox(MakeInfoRow(
        FString::Printf(TEXT("%.0f MIN"), Mission->State.elapsed / 60.0), TEXT("CZAS ROZGRYWKI"), true))
        ->SetPadding(FMargin(0, 0, 0, 7));
    Metrics->AddChildToVerticalBox(MakeInfoRow(
        FString::Printf(TEXT("%d / %d"), MainDone, static_cast<int32>(Wroclaw::Quests().size())),
        TEXT("POSTĘP GŁÓWNY")))
        ->SetPadding(FMargin(0, 0, 0, 7));
    Metrics->AddChildToVerticalBox(MakeInfoRow(
        FString::Printf(TEXT("%d"), static_cast<int32>(Mission->State.history.size())),
        TEXT("UKOŃCZONE AKCJE")))
        ->SetPadding(FMargin(0, 0, 0, 7));
    Metrics->AddChildToVerticalBox(MakeInfoRow(
        FString::Printf(TEXT("%d"), static_cast<int32>(Mission->State.evidence.size())),
        TEXT("ZEBRANE DOWODY")))
        ->SetPadding(FMargin(0, 0, 0, 7));
    Metrics->AddChildToVerticalBox(MakeInfoRow(
        FString::Printf(TEXT("%d"), Mission->State.kills), TEXT("NEUTRALIZACJE")));

    RightColumn->AddChildToVerticalBox(MakeText(TEXT("OSIĄGNIĘCIA"), 9, true, Accent))
        ->SetPadding(FMargin(0, 0, 0, 8));

    const TCHAR* Names[] = {
        TEXT("Pierwsze kroki"), TEXT("Escape Artist"), TEXT("Bez śladu"),
        TEXT("Detektyw"), TEXT("Pacyfista"), TEXT("Szybkie myślenie")
    };
    for (int32 Index = 0; Index < 6; ++Index)
    {
        const bool bUnlocked = (Mission->LifetimeAchievements & (1 << Index)) != 0;
        RightColumn->AddChildToVerticalBox(MakeInfoRow(
            Names[Index], bUnlocked ? TEXT("ZDOBYTE") : TEXT("ZABLOKOWANE"), bUnlocked))
            ->SetPadding(FMargin(0, 0, 0, 6));
    }
}

void UPlayerMenuWidget::BuildSettingsTab()
{
    PageTitle->SetText(FText::FromString(TEXT("USTAWIENIA  /  OBRAZ I WYDAJNOŚĆ")));

    auto* Preferences = UWTGPerformanceSettings::Get();
    UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr;

    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("OBRAZ"), 20, true, TextPrimary))
        ->SetPadding(FMargin(2, 1, 2, 2));
    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("USTAWIENIA NATYCHMIASTOWE"), 9, true, Accent))
        ->SetPadding(FMargin(2, 0, 2, 14));

    if (!UserSettings)
    {
        ActionColumn->AddChildToVerticalBox(MakeText(
            TEXT("Ustawienia silnika są chwilowo niedostępne."), 11, false, Muted));
        AddTextPage(TEXT("WYŚWIETLANIE"), TEXT("Nie udało się pobrać UGameUserSettings."));
        return;
    }

    const bool bFPSVisible = Preferences && Preferences->bShowFPS;
    const bool bVSync = UserSettings->IsVSyncEnabled();
    const bool bDynamicResolution = UserSettings->IsDynamicResolutionEnabled();
    const int32 Limit = Preferences ? Preferences->FPSLimit : 60;
    const FIntPoint Resolution = UserSettings->GetScreenResolution();
    const EWindowMode::Type WindowMode = UserSettings->GetFullscreenMode();
    const int32 Quality = UserSettings->GetOverallScalabilityLevel();

    float ScaleNormalized = 0.0f;
    float ScaleValue = 100.0f;
    float MinScale = 0.0f;
    float MaxScale = 100.0f;
    UserSettings->GetResolutionScaleInformationEx(ScaleNormalized, ScaleValue, MinScale, MaxScale);

    auto* ModeButton = MakeButton(
        FString::Printf(TEXT("TRYB: %s"), *WindowModeLabel(WindowMode)), true);
    ModeButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::CycleWindowMode);
    ActionColumn->AddChildToVerticalBox(ModeButton)->SetPadding(FMargin(0, 0, 0, 7));

    auto* ResolutionButton = MakeButton(
        FString::Printf(TEXT("ROZDZIELCZOŚĆ: %d × %d"), Resolution.X, Resolution.Y));
    ResolutionButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::CycleResolution);
    ActionColumn->AddChildToVerticalBox(ResolutionButton)->SetPadding(FMargin(0, 0, 0, 7));

    auto* QualityButton = MakeButton(
        FString::Printf(TEXT("JAKOŚĆ: %s"), *QualityLabel(Quality)));
    QualityButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::CycleQuality);
    ActionColumn->AddChildToVerticalBox(QualityButton)->SetPadding(FMargin(0, 0, 0, 7));

    auto* ScaleButton = MakeButton(
        FString::Printf(TEXT("SKALA RENDERU: %.0f%%"), ScaleValue));
    ScaleButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::CycleResolutionScale);
    ActionColumn->AddChildToVerticalBox(ScaleButton)->SetPadding(FMargin(0, 0, 0, 7));

    auto* VSyncButton = MakeButton(
        FString::Printf(TEXT("VSYNC: %s"), bVSync ? TEXT("WŁ.") : TEXT("WYŁ.")),
        bVSync);
    VSyncButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::ToggleVSync);
    ActionColumn->AddChildToVerticalBox(VSyncButton)->SetPadding(FMargin(0, 0, 0, 7));

    auto* DynamicButton = MakeButton(
        FString::Printf(TEXT("DYNAMICZNA ROZDZ.: %s"), bDynamicResolution ? TEXT("WŁ.") : TEXT("WYŁ.")),
        bDynamicResolution);
    DynamicButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::ToggleDynamicResolution);
    ActionColumn->AddChildToVerticalBox(DynamicButton)->SetPadding(FMargin(0, 0, 0, 16));

    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("PŁYNNOŚĆ"), 9, true, Muted))
        ->SetPadding(FMargin(2, 0, 2, 7));

    auto* FPSButton = MakeButton(
        FString::Printf(TEXT("LICZNIK FPS: %s"), bFPSVisible ? TEXT("WŁ.") : TEXT("WYŁ.")),
        bFPSVisible);
    FPSButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::ToggleFPSCounter);
    ActionColumn->AddChildToVerticalBox(FPSButton)->SetPadding(FMargin(0, 0, 0, 7));

    const FString LimitLabel = Limit == 0 ? TEXT("BEZ LIMITU") : FString::Printf(TEXT("%d FPS"), Limit);
    auto* LimitButton = MakeButton(FString::Printf(TEXT("LIMIT: %s"), *LimitLabel));
    LimitButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::CycleFPSLimit);
    ActionColumn->AddChildToVerticalBox(LimitButton);

    const FString Description = FString::Printf(
        TEXT("TRYB EKRANU\n%s\n\nROZDZIELCZOŚĆ\n%d × %d\n\nSKALA RENDERU\n%.0f%%  (zakres %.0f–%.0f%%)\n\n")
        TEXT("PRESET JAKOŚCI\n%s\n\nVSYNC\n%s\n\nDYNAMICZNA ROZDZIELCZOŚĆ\n%s\n\n")
        TEXT("LIMIT KLATEK\n%s\n\nLICZNIK FPS\n%s\n\n")
        TEXT("Zmiany są stosowane od razu i zapisywane w GameUserSettings. ")
        TEXT("Tryb bez ramki może używać rozdzielczości pulpitu niezależnie od wybranego presetu."),
        *WindowModeLabel(WindowMode),
        Resolution.X, Resolution.Y,
        ScaleValue, MinScale, MaxScale,
        *QualityLabel(Quality),
        bVSync ? TEXT("Włączony") : TEXT("Wyłączony"),
        bDynamicResolution ? TEXT("Włączona") : TEXT("Wyłączona"),
        *LimitLabel,
        bFPSVisible ? TEXT("Włączony") : TEXT("Wyłączony"));
    AddTextPage(TEXT("WYŚWIETLANIE"), Description);

    RightColumn->AddChildToVerticalBox(MakeText(TEXT("SZCZEGÓŁY JAKOŚCI"), 9, true, Accent))
        ->SetPadding(FMargin(0, 0, 0, 8));

    const FString QualityDetails = FString::Printf(
        TEXT("Widoczność       %d / 4\nCienie            %d / 4\nTekstury           %d / 4\nAntyaliasing       %d / 4\n")
        TEXT("Efekty            %d / 4\nPost-processing    %d / 4\nRoślinność         %d / 4\nGlobal illumination %d / 4\n")
        TEXT("Odbicia            %d / 4\nShading            %d / 4"),
        UserSettings->GetViewDistanceQuality(),
        UserSettings->GetShadowQuality(),
        UserSettings->GetTextureQuality(),
        UserSettings->GetAntiAliasingQuality(),
        UserSettings->GetVisualEffectQuality(),
        UserSettings->GetPostProcessingQuality(),
        UserSettings->GetFoliageQuality(),
        UserSettings->GetGlobalIlluminationQuality(),
        UserSettings->GetReflectionQuality(),
        UserSettings->GetShadingQuality());
    auto* Details = MakeText(QualityDetails, 11, false, Muted);
    Details->SetLineHeightPercentage(1.35f);
    RightColumn->AddChildToVerticalBox(Details);
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
    ShowConfirmation(
        1,
        TEXT("ROZPOCZĄĆ NOWĄ GRĘ?"),
        TEXT("Rozpoczęcie nowej gry przejdzie do wyboru postaci. Bieżący postęp powinien być zapisany przed kontynuacją."),
        TEXT("NOWA GRA"));
}

void UPlayerMenuWidget::LoadGame()
{
    if (auto* Controller = Cast<ASliceController>(GetOwningPlayer()))
        Controller->LoadGame();
}

void UPlayerMenuWidget::QuitGame()
{
    ShowConfirmation(
        2,
        TEXT("WYJŚĆ Z GRY?"),
        TEXT("Gra zostanie zamknięta. Upewnij się, że ostatni zapis został wykonany poprawnie."),
        TEXT("WYJDŹ"));
}

void UPlayerMenuWidget::PlayUIHover()
{
    USliceAudio::PlayUI(this, TEXT("UIHover"), 0.16f);
}

void UPlayerMenuWidget::PlayUIClick()
{
    USliceAudio::PlayUI(this, TEXT("UIClick"), 0.26f);
}

void UPlayerMenuWidget::ShowConfirmation(
    int32 Action,
    const FString& Title,
    const FString& Body,
    const FString& ConfirmLabel)
{
    if (!RootOverlay)
        return;

    ClearConfirmation();
    PendingConfirmation = Action;
    ConfirmationSecondsRemaining = Action == 3 ? 15.0f : 0.0f;

    ConfirmationOverlay = WidgetTree->ConstructWidget<UBorder>();
    ConfirmationOverlay->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.78f));
    auto* OverlaySlot = RootOverlay->AddChildToOverlay(ConfirmationOverlay);
    OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
    OverlaySlot->SetVerticalAlignment(VAlign_Fill);

    auto* Center = WidgetTree->ConstructWidget<UOverlay>();
    ConfirmationOverlay->AddChild(Center);

    auto* CardSize = WidgetTree->ConstructWidget<USizeBox>();
    CardSize->SetWidthOverride(560.0f);
    auto* CardSlot = Center->AddChildToOverlay(CardSize);
    CardSlot->SetHorizontalAlignment(HAlign_Center);
    CardSlot->SetVerticalAlignment(VAlign_Center);

    auto* Card = MakeCard(FMargin(28, 26, 28, 24));
    CardSize->AddChild(Card);

    auto* Column = WidgetTree->ConstructWidget<UVerticalBox>();
    Card->AddChild(Column);

    Column->AddChildToVerticalBox(MakeText(TEXT("POTWIERDZENIE"), 9, true, Accent))
        ->SetPadding(FMargin(0, 0, 0, 5));
    Column->AddChildToVerticalBox(MakeText(Title, 24, true, TextPrimary))
        ->SetPadding(FMargin(0, 0, 0, 12));

    auto* Description = MakeText(Body, 12, false, Muted);
    Description->SetLineHeightPercentage(1.25f);
    Column->AddChildToVerticalBox(Description)->SetPadding(FMargin(0, 0, 0, 18));

    if (Action == 3)
    {
        ConfirmationCountdown = MakeText(TEXT("AUTOMATYCZNE COFNIĘCIE ZA 15 S"), 10, true, Accent);
        Column->AddChildToVerticalBox(ConfirmationCountdown)->SetPadding(FMargin(0, 0, 0, 12));
    }

    auto* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>();
    Column->AddChildToVerticalBox(Buttons);

    auto* Cancel = MakeButton(Action == 3 ? TEXT("COFNIJ") : TEXT("ANULUJ"));
    Cancel->OnClicked.AddDynamic(this, &UPlayerMenuWidget::CancelConfirmation);
    auto* CancelSlot = Buttons->AddChildToHorizontalBox(Cancel);
    CancelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CancelSlot->SetPadding(FMargin(0, 0, 5, 0));

    auto* Confirm = MakeButton(ConfirmLabel, true);
    Confirm->OnClicked.AddDynamic(this, &UPlayerMenuWidget::ConfirmPendingAction);
    auto* ConfirmSlot = Buttons->AddChildToHorizontalBox(Confirm);
    ConfirmSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ConfirmSlot->SetPadding(FMargin(5, 0, 0, 0));

    if (APlayerController* PlayerController = GetOwningPlayer())
        Confirm->SetUserFocus(PlayerController);
    else
        Confirm->SetKeyboardFocus();
}

void UPlayerMenuWidget::ClearConfirmation()
{
    if (ConfirmationOverlay)
        ConfirmationOverlay->RemoveFromParent();
    ConfirmationOverlay = nullptr;
    ConfirmationCountdown = nullptr;
    PendingConfirmation = 0;
    ConfirmationSecondsRemaining = 0.0f;
}

void UPlayerMenuWidget::ConfirmPendingAction()
{
    const int32 Action = PendingConfirmation;

    if (Action == 3)
    {
        if (GEngine)
        {
            if (UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
            {
                UserSettings->ConfirmVideoMode();
                UserSettings->SaveSettings();
            }
        }
        ClearConfirmation();
        Refresh();
        return;
    }

    ClearConfirmation();

    if (auto* Controller = Cast<ASliceController>(GetOwningPlayer()))
    {
        if (Action == 1)
            Controller->NewGame();
        else if (Action == 2)
            Controller->Quit();
    }
}

void UPlayerMenuWidget::CancelConfirmation()
{
    const int32 Action = PendingConfirmation;
    ClearConfirmation();

    if (Action == 3 && GEngine)
    {
        if (UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
        {
            UserSettings->RevertVideoMode();
            UserSettings->ApplyResolutionSettings(false);
            UserSettings->SaveSettings();
        }
    }

    Refresh();
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

void UPlayerMenuWidget::ToggleDynamicResolution()
{
    if (GEngine)
    {
        if (UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
        {
            UserSettings->SetDynamicResolutionEnabled(!UserSettings->IsDynamicResolutionEnabled());
            UserSettings->ApplySettings(false);
        }
    }
    Refresh();
}

void UPlayerMenuWidget::CycleWindowMode()
{
    if (!GEngine)
        return;
    if (UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
    {
        EWindowMode::Type Next = EWindowMode::Fullscreen;
        switch (UserSettings->GetFullscreenMode())
        {
            case EWindowMode::Fullscreen: Next = EWindowMode::WindowedFullscreen; break;
            case EWindowMode::WindowedFullscreen: Next = EWindowMode::Windowed; break;
            default: Next = EWindowMode::Fullscreen; break;
        }
        UserSettings->SetFullscreenMode(Next);
        UserSettings->ApplyResolutionSettings(false);
        ShowConfirmation(
            3,
            TEXT("ZACHOWAĆ TRYB EKRANU?"),
            TEXT("Jeżeli nowy tryb nie działa poprawnie, wybierz COFNIJ. Bez potwierdzenia zmiana zostanie automatycznie cofnięta."),
            TEXT("ZACHOWAJ"));
        return;
    }
    Refresh();
}

void UPlayerMenuWidget::CycleResolution()
{
    if (!GEngine)
        return;
    if (UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
    {
        static const FIntPoint Presets[] = {
            FIntPoint(1280, 720), FIntPoint(1600, 900), FIntPoint(1920, 1080),
            FIntPoint(2560, 1440), FIntPoint(3840, 2160)
        };
        const FIntPoint Current = UserSettings->GetScreenResolution();
        const FIntPoint Desktop = UserSettings->GetDesktopResolution();

        TArray<FIntPoint> Supported;
        for (const FIntPoint& Preset : Presets)
            if ((Preset.X <= Desktop.X && Preset.Y <= Desktop.Y) || Preset == Current)
                Supported.Add(Preset);
        if (Supported.IsEmpty())
            Supported.Add(Current);

        int32 NextIndex = Supported.IndexOfByKey(Current);
        if (NextIndex != INDEX_NONE)
            NextIndex = (NextIndex + 1) % Supported.Num();
        else
        {
            NextIndex = 0;
            const int64 CurrentPixels = static_cast<int64>(Current.X) * Current.Y;
            for (int32 Index = 0; Index < Supported.Num(); ++Index)
            {
                const int64 PresetPixels = static_cast<int64>(Supported[Index].X) * Supported[Index].Y;
                if (PresetPixels > CurrentPixels)
                {
                    NextIndex = Index;
                    break;
                }
            }
        }

        UserSettings->SetScreenResolution(Supported[NextIndex]);
        UserSettings->ApplyResolutionSettings(false);
        ShowConfirmation(
            3,
            TEXT("ZACHOWAĆ ROZDZIELCZOŚĆ?"),
            TEXT("Jeżeli obraz jest nieczytelny lub monitor nie obsługuje ustawienia, wybierz COFNIJ. Bez potwierdzenia zmiana zostanie automatycznie cofnięta."),
            TEXT("ZACHOWAJ"));
        return;
    }
    Refresh();
}

void UPlayerMenuWidget::CycleQuality()
{
    if (!GEngine)
        return;
    if (UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
    {
        const int32 Current = UserSettings->GetOverallScalabilityLevel();
        const int32 Next = Current < 0 ? 3 : ((Current + 1) % 5);
        UserSettings->SetOverallScalabilityLevel(Next);
        UserSettings->ApplySettings(false);
    }
    Refresh();
}

void UPlayerMenuWidget::CycleResolutionScale()
{
    if (!GEngine)
        return;
    if (UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
    {
        float Normalized = 0.0f;
        float Current = 100.0f;
        float MinValue = 0.0f;
        float MaxValue = 100.0f;
        UserSettings->GetResolutionScaleInformationEx(Normalized, Current, MinValue, MaxValue);

        static const float Presets[] = {50.0f, 67.0f, 75.0f, 85.0f, 100.0f};
        float Next = FMath::Clamp(Presets[0], MinValue, MaxValue);
        for (float Preset : Presets)
        {
            const float Candidate = FMath::Clamp(Preset, MinValue, MaxValue);
            if (Candidate > Current + 0.5f)
            {
                Next = Candidate;
                break;
            }
        }

        UserSettings->SetResolutionScaleValueEx(Next);
        UserSettings->ApplySettings(false);
    }
    Refresh();
}

void UPlayerMenuWidget::CycleFPSLimit()
{
    auto* Preferences = UWTGPerformanceSettings::Get();
    if (!Preferences)
        return;

    static const int32 Limits[] = {0, 30, 60, 90, 120, 144, 165, 240};
    int32 Next = Limits[0];
    bool bMatched = false;
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Limits); ++Index)
    {
        if (Limits[Index] == Preferences->FPSLimit)
        {
            Next = Limits[(Index + 1) % UE_ARRAY_COUNT(Limits)];
            bMatched = true;
            break;
        }
    }
    if (!bMatched)
        Next = 60;

    Preferences->SetFPSLimit(Next);
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

void UPlayerMenuWidget::PreviewLightingModern()
{
    if (Studio) Studio->SetLighting(TEXT("Modern"));
}

void UPlayerMenuWidget::PreviewLightingDaylight()
{
    if (Studio) Studio->SetLighting(TEXT("Daylight"));
}

void UPlayerMenuWidget::PreviewLightingNight()
{
    if (Studio) Studio->SetLighting(TEXT("Night"));
}

void UPlayerMenuWidget::PreviewReset()
{
    if (Studio) Studio->ResetPresentation();
}

FReply UPlayerMenuWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();

    if (PendingConfirmation != 0)
    {
        if (Key == EKeys::Escape || Key == EKeys::BackSpace || Key == EKeys::Gamepad_FaceButton_Right)
        {
            CancelConfirmation();
            return FReply::Handled();
        }
        return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
    }

    if (Key == EKeys::Q || Key == EKeys::Gamepad_LeftShoulder)
    {
        SelectTab((ActiveTab + TabButtons.Num() - 1) % TabButtons.Num());
        return FReply::Handled();
    }

    if (Key == EKeys::E || Key == EKeys::Gamepad_RightShoulder)
    {
        SelectTab((ActiveTab + 1) % TabButtons.Num());
        return FReply::Handled();
    }

    if (Key == EKeys::Gamepad_FaceButton_Right)
    {
        auto* Mission = GetGameInstance()->GetSubsystem<USliceMission>();
        if (Mission && Mission->bInGame)
        {
            Resume();
            return FReply::Handled();
        }
    }

    if (Key == EKeys::Gamepad_Special_Right)
    {
        auto* Mission = GetGameInstance()->GetSubsystem<USliceMission>();
        if (Mission && Mission->bInGame)
        {
            Resume();
            return FReply::Handled();
        }
    }

    return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

void UPlayerMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    AmbientAnimationTime += InDeltaTime;
    if (AmbientGlowA)
    {
        AmbientGlowA->SetRenderTranslation(FVector2D(
            FMath::Sin(AmbientAnimationTime * 0.22f) * 18.0f,
            FMath::Cos(AmbientAnimationTime * 0.17f) * 12.0f));
        AmbientGlowA->SetRenderOpacity(0.78f + FMath::Sin(AmbientAnimationTime * 0.31f) * 0.12f);
    }
    if (AmbientGlowB)
    {
        AmbientGlowB->SetRenderTranslation(FVector2D(
            FMath::Cos(AmbientAnimationTime * 0.19f) * 14.0f,
            FMath::Sin(AmbientAnimationTime * 0.15f) * 10.0f));
        AmbientGlowB->SetRenderOpacity(0.72f + FMath::Cos(AmbientAnimationTime * 0.27f) * 0.10f);
    }

    if (PageAnimationTime < 0.22f && ActionColumn && CenterColumn && RightColumn)
    {
        PageAnimationTime = FMath::Min(0.22f, PageAnimationTime + InDeltaTime);
        const float T = FMath::Clamp(PageAnimationTime / 0.22f, 0.0f, 1.0f);
        const float Ease = 1.0f - FMath::Pow(1.0f - T, 3.0f);

        ActionColumn->SetRenderOpacity(Ease);
        CenterColumn->SetRenderOpacity(Ease);
        RightColumn->SetRenderOpacity(Ease);
        ActionColumn->SetRenderTranslation(FVector2D(FMath::Lerp(-12.0f, 0.0f, Ease), 0.0f));
        CenterColumn->SetRenderTranslation(FVector2D(0.0f, FMath::Lerp(8.0f, 0.0f, Ease)));
        RightColumn->SetRenderTranslation(FVector2D(FMath::Lerp(12.0f, 0.0f, Ease), 0.0f));
    }

    if (PendingConfirmation != 3 || !ConfirmationOverlay)
        return;

    ConfirmationSecondsRemaining = FMath::Max(0.0f, ConfirmationSecondsRemaining - InDeltaTime);
    if (ConfirmationCountdown)
    {
        ConfirmationCountdown->SetText(FText::FromString(FString::Printf(
            TEXT("AUTOMATYCZNE COFNIĘCIE ZA %d S"),
            FMath::Max(0, FMath::CeilToInt(ConfirmationSecondsRemaining)))));
    }

    if (ConfirmationSecondsRemaining <= 0.0f)
        CancelConfirmation();
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
