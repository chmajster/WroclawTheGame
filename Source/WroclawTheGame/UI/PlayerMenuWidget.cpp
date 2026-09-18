#include "UI/PlayerMenuWidget.h"

#include "UI/SliceController.h"
#include "UI/PerformanceSettings.h"
#include "Audio/SliceAudio.h"
#include "Audio/AudioSettings.h"
#include "Character/CharacterCreator.h"
#include "Character/CharacterCreatorSubsystem.h"
#include "Character/CharacterAppearanceComponent.h"
#include "Character/SliceCharacter.h"
#include "Components/GameplayComponents.h"
#include "Mission/SliceMission.h"
#include "Systems/CityGameplaySubsystem.h"
#include "Systems/WroclawMapSubsystem.h"

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
#include "Components/SafeZone.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/Pawn.h"
#include "Input/Reply.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/DateTime.h"

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
const FLinearColor Danger(0.88f, 0.20f, 0.18f, 1.0f);
const FLinearColor DangerHover(0.97f, 0.29f, 0.25f, 1.0f);
const FLinearColor DangerPressed(0.67f, 0.12f, 0.11f, 1.0f);

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

FString BuildLabel()
{
#if UE_BUILD_SHIPPING
    return TEXT("SHIPPING");
#elif UE_BUILD_TEST
    return TEXT("TEST");
#elif UE_BUILD_DEBUG
    return TEXT("DEBUG");
#elif UE_BUILD_DEVELOPMENT
    return TEXT("DEVELOPMENT");
#else
    return TEXT("UNKNOWN");
#endif
}

FString ProjectVersionLabel()
{
    FString Version = TEXT("0.0.0");
    if (GConfig)
        GConfig->GetString(
            TEXT("/Script/EngineSettings.GeneralProjectSettings"),
            TEXT("ProjectVersion"), Version, GGameIni);
    return FString::Printf(TEXT("v%s  •  %s"), *Version, *BuildLabel());
}

float NextAudioVolume(float Current)
{
    constexpr float Levels[] = {0.0f, 0.25f, 0.50f, 0.75f, 1.0f};
    for (float Level : Levels)
        if (Level > Current + 0.01f)
            return Level;
    return 0.0f;
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
    Button->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

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

UBorder* UPlayerMenuWidget::MakeKeycap(const FString& Label)
{
    auto* Keycap = WidgetTree->ConstructWidget<UBorder>();
    Keycap->SetBrush(RoundedBrush(FLinearColor(0.055f, 0.075f, 0.098f, 0.96f), 6.0f));
    Keycap->SetPadding(FMargin(7, 3, 7, 3));

    auto* LabelText = MakeText(Label, 8, true, TextPrimary);
    LabelText->SetJustification(ETextJustify::Center);
    Keycap->AddChild(LabelText);
    return Keycap;
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
    Row->SetPadding(FMargin(10, 9, 12, 9));

    auto* Content = WidgetTree->ConstructWidget<UHorizontalBox>();
    Row->AddChild(Content);

    auto* Rail = WidgetTree->ConstructWidget<UBorder>();
    Rail->SetBrush(RoundedBrush(bHighlighted ? Accent : Divider, 2.0f));
    auto* RailSize = WidgetTree->ConstructWidget<USizeBox>();
    RailSize->SetWidthOverride(3.0f);
    RailSize->AddChild(Rail);
    Content->AddChildToHorizontalBox(RailSize)->SetPadding(FMargin(0, 1, 10, 1));

    auto* Column = WidgetTree->ConstructWidget<UVerticalBox>();
    auto* ColumnSlot = Content->AddChildToHorizontalBox(Column);
    ColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

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

    MenuBackgroundBlur = WidgetTree->ConstructWidget<UBackgroundBlur>();
    const auto* UISettings = UWTGPerformanceSettings::Get();
    const bool bBlurEnabled = !UISettings || UISettings->bMenuBackgroundBlur;
    MenuBackgroundBlur->SetBlurStrength(bBlurEnabled ? 12.0f : 0.0f);
    MenuBackgroundBlur->SetBlurRadius(bBlurEnabled ? 18 : 0);
    MenuBackgroundBlur->SetApplyAlphaToBlur(false);
    auto* BlurFill = WidgetTree->ConstructWidget<UBorder>();
    BlurFill->SetBrushColor(FLinearColor(0, 0, 0, 0.01f));
    MenuBackgroundBlur->AddChild(BlurFill);
    auto* BlurSlot = RootOverlay->AddChildToOverlay(MenuBackgroundBlur);
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
    auto* SafeArea = WidgetTree->ConstructWidget<USafeZone>();
    auto* SafeAreaSlot = RootOverlay->AddChildToOverlay(SafeArea);
    SafeAreaSlot->SetHorizontalAlignment(HAlign_Fill);
    SafeAreaSlot->SetVerticalAlignment(VAlign_Fill);

    auto* Scale = WidgetTree->ConstructWidget<UScaleBox>();
    Scale->SetStretch(EStretch::ScaleToFit);
    Scale->SetStretchDirection(EStretchDirection::Both);
    SafeArea->AddChild(Scale);

    auto* Frame = WidgetTree->ConstructWidget<USizeBox>();
    Frame->SetWidthOverride(1600.0f);
    Frame->SetHeightOverride(900.0f);
    Scale->AddChild(Frame);

    ShellLayout = WidgetTree->ConstructWidget<UVerticalBox>();
    Frame->AddChild(ShellLayout);
    ShellLayout->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
    const bool bReduceShellMotion = UISettings && UISettings->bReduceUIMotion;
    ShellAnimationTime = bReduceShellMotion ? 0.28f : 0.0f;
    ShellLayout->SetRenderOpacity(bReduceShellMotion ? 1.0f : 0.0f);
    ShellLayout->SetRenderTranslation(
        bReduceShellMotion ? FVector2D::ZeroVector : FVector2D(0.0f, 10.0f));
    ShellLayout->SetRenderScale(
        bReduceShellMotion ? FVector2D(1.0f, 1.0f) : FVector2D(0.985f, 0.985f));

    auto* Layout = ShellLayout;

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
        auto* Tab = WidgetTree->ConstructWidget<UVerticalBox>();
        auto* Slot = Top->AddChildToHorizontalBox(Tab);
        Slot->SetPadding(FMargin(3, 1, 3, 1));
        Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

        auto* Button = MakeButton(FString::Printf(TEXT("%02d  %s"), Index + 1, Labels[Index]));
        auto* ButtonSlot = Tab->AddChildToVerticalBox(Button);
        ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

        auto* IndicatorSize = WidgetTree->ConstructWidget<USizeBox>();
        IndicatorSize->SetHeightOverride(2.0f);
        auto* Indicator = WidgetTree->ConstructWidget<UBorder>();
        Indicator->SetBrushColor(Accent);
        IndicatorSize->AddChild(Indicator);
        Tab->AddChildToVerticalBox(IndicatorSize)->SetPadding(FMargin(8, 4, 8, 0));

        TabButtons.Add(Button);
        TabIndicators.Add(Indicator);
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

    auto* PageChip = WidgetTree->ConstructWidget<UBorder>();
    PageChip->SetBrush(RoundedBrush(FLinearColor(Accent.R, Accent.G, Accent.B, 0.10f), 7.0f));
    PageChip->SetPadding(FMargin(9, 4, 9, 4));
    PageCounter = MakeText(TEXT("01 / 07"), 9, true, Accent);
    PageCounter->SetJustification(ETextJustify::Center);
    PageChip->AddChild(PageCounter);
    ContextBar->AddChildToHorizontalBox(PageChip)->SetPadding(FMargin(0, 0, 12, 0));

    PageTitle = MakeText(TEXT(""), 11, true, Accent);
    ContextBar->AddChildToHorizontalBox(PageTitle)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    auto* DotSize = WidgetTree->ConstructWidget<USizeBox>();
    DotSize->SetWidthOverride(8.0f);
    DotSize->SetHeightOverride(8.0f);
    SessionStateDot = WidgetTree->ConstructWidget<UBorder>();
    SessionStateDot->SetBrush(RoundedBrush(Muted, 4.0f));
    DotSize->AddChild(SessionStateDot);
    ContextBar->AddChildToHorizontalBox(DotSize)->SetPadding(FMargin(0, 5, 8, 0));

    ContextStatus = MakeText(TEXT(""), 9, true, Muted);
    ContextStatus->SetJustification(ETextJustify::Right);
    ContextBar->AddChildToHorizontalBox(ContextStatus)->SetPadding(FMargin(0, 0, 18, 0));

    ContextHint = MakeText(TEXT(""), 10, true, Muted);
    ContextHint->SetJustification(ETextJustify::Right);
    ContextBar->AddChildToHorizontalBox(ContextHint);

    auto* Body = WidgetTree->ConstructWidget<UHorizontalBox>();
    auto* BodySlot = Layout->AddChildToVerticalBox(Body);
    BodySlot->SetPadding(FMargin(30, 0, 30, 0));
    BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    auto* LeftCard = MakeCard(FMargin(14, 12, 14, 14));
    auto* LeftSize = WidgetTree->ConstructWidget<USizeBox>();
    LeftSize->SetWidthOverride(285.0f);
    LeftSize->AddChild(LeftCard);
    Body->AddChildToHorizontalBox(LeftSize)->SetPadding(FMargin(0, 0, 16, 0));
    auto* LeftPanel = WidgetTree->ConstructWidget<UVerticalBox>();
    LeftCard->AddChild(LeftPanel);
    ActionPanelLabel = MakeText(TEXT("AKCJE"), 8, true, Muted);
    LeftPanel->AddChildToVerticalBox(ActionPanelLabel)->SetPadding(FMargin(4, 0, 4, 8));
    auto* LeftDivider = WidgetTree->ConstructWidget<UBorder>();
    LeftDivider->SetBrushColor(FLinearColor(Divider.R, Divider.G, Divider.B, 0.72f));
    auto* LeftDividerSize = WidgetTree->ConstructWidget<USizeBox>();
    LeftDividerSize->SetHeightOverride(1.0f);
    LeftDividerSize->AddChild(LeftDivider);
    LeftPanel->AddChildToVerticalBox(LeftDividerSize)->SetPadding(FMargin(4, 0, 4, 10));
    ActionScroll = WidgetTree->ConstructWidget<UScrollBox>();
    ActionScroll->SetScrollBarVisibility(ESlateVisibility::Hidden);
    ActionScroll->SetAnimateWheelScrolling(true);
    ActionScroll->SetWheelScrollMultiplier(42.0f);
    LeftPanel->AddChildToVerticalBox(ActionScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ActionColumn = WidgetTree->ConstructWidget<UVerticalBox>();
    ActionScroll->AddChild(ActionColumn);

    auto* CenterCard = MakeCard(FMargin(10, 10, 10, 10));
    auto* CenterSlot = Body->AddChildToHorizontalBox(CenterCard);
    CenterSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CenterSlot->SetPadding(FMargin(0, 0, 16, 0));
    auto* CenterPanel = WidgetTree->ConstructWidget<UVerticalBox>();
    CenterCard->AddChild(CenterPanel);
    CenterPanelLabel = MakeText(TEXT("ZAWARTOŚĆ"), 8, true, Accent);
    CenterPanel->AddChildToVerticalBox(CenterPanelLabel)->SetPadding(FMargin(10, 2, 10, 8));
    auto* CenterDivider = WidgetTree->ConstructWidget<UBorder>();
    CenterDivider->SetBrushColor(FLinearColor(Divider.R, Divider.G, Divider.B, 0.72f));
    auto* CenterDividerSize = WidgetTree->ConstructWidget<USizeBox>();
    CenterDividerSize->SetHeightOverride(1.0f);
    CenterDividerSize->AddChild(CenterDivider);
    CenterPanel->AddChildToVerticalBox(CenterDividerSize)->SetPadding(FMargin(10, 0, 10, 8));
    CenterScroll = WidgetTree->ConstructWidget<UScrollBox>();
    CenterScroll->SetScrollBarVisibility(ESlateVisibility::Hidden);
    CenterScroll->SetAnimateWheelScrolling(true);
    CenterScroll->SetWheelScrollMultiplier(42.0f);
    CenterPanel->AddChildToVerticalBox(CenterScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CenterColumn = WidgetTree->ConstructWidget<UVerticalBox>();
    CenterScroll->AddChild(CenterColumn);

    auto* RightCard = MakeCard(FMargin(16, 14, 16, 16));
    auto* RightSize = WidgetTree->ConstructWidget<USizeBox>();
    RightSize->SetWidthOverride(320.0f);
    RightSize->AddChild(RightCard);
    Body->AddChildToHorizontalBox(RightSize);
    auto* RightPanel = WidgetTree->ConstructWidget<UVerticalBox>();
    RightCard->AddChild(RightPanel);
    RightPanelLabel = MakeText(TEXT("KONTEKST"), 8, true, Muted);
    RightPanel->AddChildToVerticalBox(RightPanelLabel)->SetPadding(FMargin(4, 0, 4, 8));
    auto* RightDivider = WidgetTree->ConstructWidget<UBorder>();
    RightDivider->SetBrushColor(FLinearColor(Divider.R, Divider.G, Divider.B, 0.72f));
    auto* RightDividerSize = WidgetTree->ConstructWidget<USizeBox>();
    RightDividerSize->SetHeightOverride(1.0f);
    RightDividerSize->AddChild(RightDivider);
    RightPanel->AddChildToVerticalBox(RightDividerSize)->SetPadding(FMargin(4, 0, 4, 10));
    RightScroll = WidgetTree->ConstructWidget<UScrollBox>();
    RightScroll->SetScrollBarVisibility(ESlateVisibility::Hidden);
    RightScroll->SetAnimateWheelScrolling(true);
    RightScroll->SetWheelScrollMultiplier(42.0f);
    RightPanel->AddChildToVerticalBox(RightScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    RightColumn = WidgetTree->ConstructWidget<UVerticalBox>();
    RightScroll->AddChild(RightColumn);

    auto* FooterDivider = WidgetTree->ConstructWidget<UBorder>();
    FooterDivider->SetBrushColor(FLinearColor(Divider.R, Divider.G, Divider.B, 0.72f));
    auto* FooterDividerSize = WidgetTree->ConstructWidget<USizeBox>();
    FooterDividerSize->SetHeightOverride(1.0f);
    FooterDividerSize->AddChild(FooterDivider);
    Layout->AddChildToVerticalBox(FooterDividerSize)->SetPadding(FMargin(34, 10, 34, 0));

    auto* Footer = WidgetTree->ConstructWidget<UHorizontalBox>();
    auto* FooterSlot = Layout->AddChildToVerticalBox(Footer);
    FooterSlot->SetPadding(FMargin(34, 10, 34, 20));

    auto* Location = MakeText(TEXT("WROCŁAW  /  DOLNY ŚLĄSK"), 10, true, Muted);
    Footer->AddChildToHorizontalBox(Location)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    ClockText = MakeText(FDateTime::Now().ToString(TEXT("%d.%m.%Y  •  %H:%M")), 9, true, Muted);
    ClockText->SetJustification(ETextJustify::Center);
    Footer->AddChildToHorizontalBox(ClockText)->SetPadding(FMargin(16, 0, 16, 0));

    auto* Build = MakeText(ProjectVersionLabel(), 9, true, Muted);
    Build->SetJustification(ETextJustify::Center);
    auto* BuildSlot = Footer->AddChildToHorizontalBox(Build);
    BuildSlot->SetPadding(FMargin(16, 0, 16, 0));

    auto* InputLegend = WidgetTree->ConstructWidget<UHorizontalBox>();
    auto* InputLegendSlot = Footer->AddChildToHorizontalBox(InputLegend);
    InputLegendSlot->SetPadding(FMargin(10, 0, 0, 0));

    auto AddFooterShortcut = [&](const FString& Key, const FString& Label)
    {
        InputLegend->AddChildToHorizontalBox(MakeKeycap(Key))->SetPadding(FMargin(0, 0, 6, 0));
        auto* Hint = MakeText(Label, 8, true, Muted);
        InputLegend->AddChildToHorizontalBox(Hint)->SetPadding(FMargin(0, 3, 12, 0));
    };

    AddFooterShortcut(TEXT("ESC"), TEXT("WRÓĆ"));
    AddFooterShortcut(TEXT("1–7"), TEXT("ZAKŁADKI"));
    AddFooterShortcut(TEXT("ENTER / A"), TEXT("WYBIERZ"));
}

void UPlayerMenuWidget::Refresh()
{
    if (!ActionColumn || !CenterColumn || !RightColumn)
        return;

    ActionColumn->ClearChildren();
    CenterColumn->ClearChildren();
    RightColumn->ClearChildren();
    if (ActionScroll) ActionScroll->ScrollToStart();
    if (CenterScroll) CenterScroll->ScrollToStart();
    if (RightScroll) RightScroll->ScrollToStart();
    ActionButtons.Reset();
    SFXVolumeButton = nullptr;
    UIVolumeButton = nullptr;
    SFXVolumeMeter = nullptr;
    UIVolumeMeter = nullptr;
    PreviewViewButtons.Reset();
    PreviewLightingButtons.Reset();
    UpdateTabStyle();
    UpdateContextStatus();
    UpdateContextHint();
    UpdatePanelLabels();

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

    const auto* UISettings = UWTGPerformanceSettings::Get();
    const bool bReduceMotion = UISettings && UISettings->bReduceUIMotion;
    PageAnimationTime = bReduceMotion ? 0.22f : 0.0f;
    ActionColumn->SetRenderOpacity(bReduceMotion ? 1.0f : 0.0f);
    CenterColumn->SetRenderOpacity(bReduceMotion ? 1.0f : 0.0f);
    RightColumn->SetRenderOpacity(bReduceMotion ? 1.0f : 0.0f);
    ActionColumn->SetRenderTranslation(bReduceMotion ? FVector2D::ZeroVector : FVector2D(-12.0f, 0.0f));
    CenterColumn->SetRenderTranslation(bReduceMotion ? FVector2D::ZeroVector : FVector2D(0.0f, 8.0f));
    RightColumn->SetRenderTranslation(bReduceMotion ? FVector2D::ZeroVector : FVector2D(12.0f, 0.0f));

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

        if (TabIndicators.IsValidIndex(Index) && TabIndicators[Index])
        {
            TabIndicators[Index]->SetBrushColor(bActive ? Accent : Divider);
            TabIndicators[Index]->SetRenderOpacity(bActive ? 1.0f : 0.24f);
        }
    }
}

void UPlayerMenuWidget::UpdateContextStatus()
{
    if (PageCounter)
        PageCounter->SetText(FText::FromString(FString::Printf(TEXT("%02d / 07"), ActiveTab + 1)));

    if (!ContextStatus)
        return;

    auto* Mission = GetGameInstance()->GetSubsystem<USliceMission>();
    auto* City = GetWorld()->GetSubsystem<UCityGameplaySubsystem>();
    const bool bCity = City && City->IsActive();
    const bool bInGame = Mission && Mission->bInGame;
    const bool bHasSave = bCity || (Mission && Mission->HasSave());

    const FString Mode = bCity ? TEXT("OTWARTY ŚWIAT") : TEXT("KAMPANIA");
    const FString Session = bInGame ? TEXT("SESJA AKTYWNA") : TEXT("MENU GŁÓWNE");
    const FString Save = bHasSave ? TEXT("ZAPIS DOSTĘPNY") : TEXT("BRAK ZAPISU");

    ContextStatus->SetText(FText::FromString(FString::Printf(
        TEXT("%s  •  %s  •  %s"), *Mode, *Session, *Save)));
    ContextStatus->SetColorAndOpacity(FSlateColor(bHasSave ? Accent : Muted));
    if (SessionStateDot)
        SessionStateDot->SetBrush(RoundedBrush(bInGame ? Accent : Muted, 4.0f));
}

void UPlayerMenuWidget::UpdateContextHint()
{
    if (!ContextHint)
        return;

    FString Hint = TEXT("Q/E  •  L1/R1  ZMIEŃ ZAKŁADKĘ");

    if (ActiveTab == 1)
    {
        Hint = TEXT("PPM  OBRÓT    •    KÓŁKO  ZOOM");
    }
    else if (ActiveTab == 4)
    {
        auto* CityGameplay = GetWorld()->GetSubsystem<UCityGameplaySubsystem>();
        auto* MapSubsystem = GetWorld()->GetSubsystem<UWroclawMapSubsystem>();
        Hint = CityGameplay && CityGameplay->IsActive() && MapSubsystem
            ? TEXT("C  NASTĘPNY CEL    •    BACKSPACE  USUŃ CEL")
            : TEXT("MAPA ODKRYĆ");
    }
    else if (ActiveTab == 6)
    {
        Hint = TEXT("ENTER / A  ZMIEŃ WARTOŚĆ");
    }

    ContextHint->SetText(FText::FromString(Hint));
}

void UPlayerMenuWidget::UpdatePanelLabels()
{
    static const TCHAR* LeftLabels[] = {
        TEXT("AKCJE"), TEXT("STEROWANIE"), TEXT("FILTRY"),
        TEXT("NAWIGACJA"), TEXT("NAWIGACJA"), TEXT("NAWIGACJA"), TEXT("KATEGORIE")
    };
    static const TCHAR* CenterLabels[] = {
        TEXT("SESJA"), TEXT("PODGLĄD"), TEXT("EKWIPUNEK"),
        TEXT("POSTĘP"), TEXT("MAPA"), TEXT("METRYKI"), TEXT("OPCJE")
    };
    static const TCHAR* RightLabels[] = {
        TEXT("STATUS"), TEXT("PROFIL"), TEXT("PODSUMOWANIE"),
        TEXT("SZCZEGÓŁY"), TEXT("CEL"), TEXT("PODSUMOWANIE"), TEXT("BIEŻĄCE WARTOŚCI")
    };

    const int32 Index = FMath::Clamp(ActiveTab, 0, 6);
    if (ActionPanelLabel)
        ActionPanelLabel->SetText(FText::FromString(LeftLabels[Index]));
    if (CenterPanelLabel)
        CenterPanelLabel->SetText(FText::FromString(CenterLabels[Index]));
    if (RightPanelLabel)
        RightPanelLabel->SetText(FText::FromString(RightLabels[Index]));
}

void UPlayerMenuWidget::ShowToast(const FString& Message)
{
    if (!RootOverlay)
        return;

    if (ToastCard)
        ToastCard->RemoveFromParent();

    ToastCard = WidgetTree->ConstructWidget<UBorder>();
    ToastCard->SetBrush(RoundedBrush(FLinearColor(0.020f, 0.075f, 0.095f, 0.98f), 10.0f));
    ToastCard->SetPadding(FMargin(10, 9, 14, 9));

    auto* ToastContent = WidgetTree->ConstructWidget<UHorizontalBox>();
    ToastCard->AddChild(ToastContent);

    auto* ToastRail = WidgetTree->ConstructWidget<UBorder>();
    ToastRail->SetBrush(RoundedBrush(Accent, 2.0f));
    auto* ToastRailSize = WidgetTree->ConstructWidget<USizeBox>();
    ToastRailSize->SetWidthOverride(3.0f);
    ToastRailSize->AddChild(ToastRail);
    ToastContent->AddChildToHorizontalBox(ToastRailSize)->SetPadding(FMargin(0, 1, 10, 1));
    ToastContent->AddChildToHorizontalBox(MakeText(Message, 9, true, TextPrimary));

    auto* Slot = RootOverlay->AddChildToOverlay(ToastCard);
    Slot->SetHorizontalAlignment(HAlign_Right);
    Slot->SetVerticalAlignment(VAlign_Bottom);
    Slot->SetPadding(FMargin(24, 24, 24, 72));

    ToastTimeRemaining = 1.8f;
    const auto* UISettings = UWTGPerformanceSettings::Get();
    const bool bReduceMotion = UISettings && UISettings->bReduceUIMotion;
    ToastAnimationTime = bReduceMotion ? 0.18f : 0.0f;
    ToastCard->SetRenderOpacity(bReduceMotion ? 1.0f : 0.0f);
    ToastCard->SetRenderTranslation(
        bReduceMotion ? FVector2D::ZeroVector : FVector2D(18.0f, 0.0f));
}

void UPlayerMenuWidget::RefreshWithSettingsToast()
{
    Refresh();
    ShowToast(TEXT("USTAWIENIA ZAPISANE"));
}

void UPlayerMenuWidget::UpdateFocusPresentation()
{
    auto HasFocus = [](UButton* Button)
    {
        return Button && (Button->HasAnyUserFocus() || Button->HasKeyboardFocus());
    };

    for (int32 Index = 0; Index < TabButtons.Num(); ++Index)
    {
        UButton* Button = TabButtons[Index];
        if (!Button)
            continue;
        const bool bFocused = HasFocus(Button);
        const bool bActive = Index == ActiveTab;
        const float Scale = bFocused ? 1.015f : (bActive ? 1.008f : 1.0f);
        Button->SetRenderScale(FVector2D(Scale, Scale));
        Button->SetRenderOpacity((bFocused || bActive) ? 1.0f : 0.94f);
    }

    for (UButton* Button : ActionButtons)
    {
        if (!Button)
            continue;
        const bool bFocused = HasFocus(Button);
        const float Scale = bFocused ? 1.015f : 1.0f;
        Button->SetRenderScale(FVector2D(Scale, Scale));
        Button->SetRenderOpacity(bFocused ? 1.0f : 0.97f);
    }
}

void UPlayerMenuWidget::FocusPrimaryAction()
{
    if (PendingConfirmation != 0)
        return;

    if (ActiveTab == 6 && ActionButtons.IsValidIndex(SettingsSection))
    {
        UButton* ActiveSettingsSection = ActionButtons[SettingsSection];
        if (ActiveSettingsSection && ActiveSettingsSection->GetIsEnabled() &&
            ActiveSettingsSection->GetVisibility() == ESlateVisibility::Visible)
        {
            if (APlayerController* PlayerController = GetOwningPlayer())
                ActiveSettingsSection->SetUserFocus(PlayerController);
            else
                ActiveSettingsSection->SetKeyboardFocus();
            return;
        }
    }

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

void UPlayerMenuWidget::AddGameHero()
{
    auto* Mission = GetGameInstance()->GetSubsystem<USliceMission>();
    auto* City = GetWorld()->GetSubsystem<UCityGameplaySubsystem>();
    const bool bCity = City && City->IsActive();
    const bool bInGame = Mission && Mission->bInGame;
    const bool bHasSave = bCity || (Mission && Mission->HasSave());

    int32 Completed = 0;
    int32 Total = 0;
    if (bCity)
    {
        Completed = City->CompletedActivityCount();
        Total = City->TrackableActivityCount();
    }
    else
    {
        Total = static_cast<int32>(Wroclaw::Quests().size());
        if (Mission)
            for (const auto& Quest : Wroclaw::Quests())
                if (Mission->State.QuestComplete(Quest))
                    ++Completed;
    }

    auto* Stage = WidgetTree->ConstructWidget<UOverlay>();
    auto* StageSlot = CenterColumn->AddChildToVerticalBox(Stage);
    StageSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    auto* StageBg = WidgetTree->ConstructWidget<UBorder>();
    StageBg->SetBrush(RoundedBrush(FLinearColor(0.006f, 0.013f, 0.022f, 0.98f), 14.0f));
    Stage->AddChildToOverlay(StageBg);

    if (Studio && Studio->RenderTarget)
    {
        auto* PreviewScale = WidgetTree->ConstructWidget<UScaleBox>();
        PreviewScale->SetStretch(EStretch::ScaleToFit);
        PreviewScale->SetStretchDirection(EStretchDirection::Both);
        auto* PreviewSlot = Stage->AddChildToOverlay(PreviewScale);
        PreviewSlot->SetHorizontalAlignment(HAlign_Right);
        PreviewSlot->SetVerticalAlignment(VAlign_Fill);
        PreviewSlot->SetPadding(FMargin(330, 24, 20, 22));

        auto* Image = WidgetTree->ConstructWidget<UImage>();
        FSlateBrush Brush;
        Brush.SetResourceObject(Studio->RenderTarget);
        Brush.ImageSize = FVector2D(720, 1000);
        Image->SetBrush(Brush);
        Image->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.83f));
        PreviewScale->AddChild(Image);
    }

    auto* TopLeft = WidgetTree->ConstructWidget<UBorder>();
    TopLeft->SetBrush(RoundedBrush(FLinearColor(0.010f, 0.028f, 0.040f, 0.94f), 8.0f));
    TopLeft->SetPadding(FMargin(11, 6));
    auto* TopLeftSlot = Stage->AddChildToOverlay(TopLeft);
    TopLeftSlot->SetHorizontalAlignment(HAlign_Left);
    TopLeftSlot->SetVerticalAlignment(VAlign_Top);
    TopLeftSlot->SetPadding(FMargin(18));
    TopLeft->AddChild(MakeText(
        bCity ? TEXT("WROCŁAW  /  OTWARTY ŚWIAT") : TEXT("ROZDZIAŁ 01  /  PRZEBUDZENIE"),
        9, true, Accent));

    auto* StatusBadge = WidgetTree->ConstructWidget<UBorder>();
    StatusBadge->SetBrush(RoundedBrush(
        bInGame ? FLinearColor(0.04f, 0.24f, 0.29f, 0.94f) : PanelSoft, 8.0f));
    StatusBadge->SetPadding(FMargin(11, 6));
    auto* BadgeSlot = Stage->AddChildToOverlay(StatusBadge);
    BadgeSlot->SetHorizontalAlignment(HAlign_Right);
    BadgeSlot->SetVerticalAlignment(VAlign_Top);
    BadgeSlot->SetPadding(FMargin(18));
    StatusBadge->AddChild(MakeText(
        bInGame ? TEXT("AKTYWNA SESJA") : (bHasSave ? TEXT("ZAPIS GOTOWY") : TEXT("NOWA HISTORIA")),
        9, true, bInGame ? Accent : Muted));

    auto* HeroCard = WidgetTree->ConstructWidget<UBorder>();
    HeroCard->SetBrush(RoundedBrush(FLinearColor(0.004f, 0.009f, 0.015f, 0.91f), 12.0f));
    HeroCard->SetPadding(FMargin(22, 18, 22, 18));
    auto* HeroSlot = Stage->AddChildToOverlay(HeroCard);
    HeroSlot->SetHorizontalAlignment(HAlign_Left);
    HeroSlot->SetVerticalAlignment(VAlign_Bottom);
    HeroSlot->SetPadding(FMargin(18, 18, 300, 18));

    auto* HeroBox = WidgetTree->ConstructWidget<UVerticalBox>();
    HeroCard->AddChild(HeroBox);

    HeroBox->AddChildToVerticalBox(MakeText(TEXT("WROCŁAW"), 10, true, Accent))
        ->SetPadding(FMargin(0, 0, 0, 3));
    HeroBox->AddChildToVerticalBox(MakeText(
        bCity ? TEXT("OTWARTY ŚWIAT") : TEXT("PRZEBUDZENIE"), 30, true, TextPrimary))
        ->SetPadding(FMargin(0, 0, 0, 10));

    const FString Objective = bCity
        ? City->NearbyObjective()
        : (bInGame && Mission
            ? Mission->ObjectiveText()
            : (bHasSave ? TEXT("Wczytaj ostatni zapis i kontynuuj historię.")
                        : TEXT("Rozpocznij nową kampanię i obudź się we Wrocławiu.")));
    HeroBox->AddChildToVerticalBox(MakeText(TEXT("NASTĘPNY CEL"), 9, true, Accent))
        ->SetPadding(FMargin(0, 0, 0, 4));
    auto* ObjectiveText = MakeText(Objective, 12, false, TextPrimary);
    ObjectiveText->SetLineHeightPercentage(1.18f);
    HeroBox->AddChildToVerticalBox(ObjectiveText)->SetPadding(FMargin(0, 0, 0, 14));

    const int32 ProgressPercent = Total > 0
        ? FMath::Clamp(FMath::RoundToInt(static_cast<float>(Completed) * 100.0f / Total), 0, 100)
        : 0;
    auto MakeHeroPill = [&](const FString& Label, const FString& Value, bool bHighlight)
    {
        auto* Pill = WidgetTree->ConstructWidget<UBorder>();
        Pill->SetBrush(RoundedBrush(
            bHighlight ? FLinearColor(0.025f, 0.105f, 0.135f, 0.96f) : PanelSoft, 8.0f));
        Pill->SetPadding(FMargin(10, 7, 10, 7));
        auto* PillText = MakeText(
            FString::Printf(TEXT("%s  •  %s"), *Label, *Value), 9, true,
            bHighlight ? Accent : Muted);
        Pill->AddChild(PillText);
        return Pill;
    };

    auto* HeroSummary = WidgetTree->ConstructWidget<UHorizontalBox>();
    HeroBox->AddChildToVerticalBox(HeroSummary)->SetPadding(FMargin(0, 0, 0, 14));

    auto* SessionPill = MakeHeroPill(
        TEXT("SESJA"), bInGame ? TEXT("AKTYWNA") : TEXT("MENU"), bInGame);
    HeroSummary->AddChildToHorizontalBox(SessionPill)->SetPadding(FMargin(0, 0, 6, 0));

    auto* SavePill = MakeHeroPill(
        TEXT("ZAPIS"), bHasSave ? TEXT("DOSTĘPNY") : TEXT("BRAK"), bHasSave);
    HeroSummary->AddChildToHorizontalBox(SavePill)->SetPadding(FMargin(0, 0, 6, 0));

    auto* ProgressPill = MakeHeroPill(
        TEXT("POSTĘP"), FString::Printf(TEXT("%d%%"), ProgressPercent), ProgressPercent > 0);
    HeroSummary->AddChildToHorizontalBox(ProgressPill);

    auto* ProgressHead = WidgetTree->ConstructWidget<UHorizontalBox>();
    HeroBox->AddChildToVerticalBox(ProgressHead)->SetPadding(FMargin(0, 0, 0, 5));
    ProgressHead->AddChildToHorizontalBox(MakeText(
        bCity ? TEXT("POSTĘP MIASTA") : TEXT("POSTĘP ROZDZIAŁU"), 9, true, Muted))
        ->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* PercentText = MakeText(
        FString::Printf(TEXT("%d / %d"), Completed, FMath::Max(Total, 1)), 9, true, Accent);
    PercentText->SetJustification(ETextJustify::Right);
    ProgressHead->AddChildToHorizontalBox(PercentText);

    auto* Progress = WidgetTree->ConstructWidget<UProgressBar>();
    Progress->SetPercent(Total > 0 ? static_cast<float>(Completed) / Total : 0.0f);
    Progress->SetFillColorAndOpacity(Accent);
    HeroBox->AddChildToVerticalBox(Progress)->SetPadding(FMargin(0, 0, 0, 12));

    const bool bSaveError = bCity ? City->IsWriteBlocked() : (Mission && !Mission->bLastSaveSucceeded);
    const FString SaveText = bCity
        ? (City->IsWriteBlocked() ? TEXT("AUTOMATYCZNY ZAPIS  •  ZABLOKOWANY")
                                  : TEXT("AUTOMATYCZNY ZAPIS  •  AKTYWNY"))
        : (Mission
            ? (Mission->bLastSaveSucceeded ? TEXT("OSTATNI ZAPIS  •  OK") : TEXT("OSTATNI ZAPIS  •  BŁĄD"))
            : TEXT("BRAK AKTYWNEGO ZAPISU"));
    HeroBox->AddChildToVerticalBox(MakeText(
        SaveText, 9, true,
        bSaveError ? FLinearColor(0.95f, 0.42f, 0.34f, 1.0f) : Muted));
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

    auto* StateBadge = WidgetTree->ConstructWidget<UBorder>();
    StateBadge->SetBrush(RoundedBrush(FLinearColor(0.015f, 0.024f, 0.036f, 0.92f), 8.0f));
    StateBadge->SetPadding(FMargin(11, 7));
    auto* StateBadgeSlot = Stage->AddChildToOverlay(StateBadge);
    StateBadgeSlot->SetHorizontalAlignment(HAlign_Right);
    StateBadgeSlot->SetVerticalAlignment(VAlign_Top);
    StateBadgeSlot->SetPadding(FMargin(18));
    auto* StateBox = WidgetTree->ConstructWidget<UVerticalBox>();
    StateBadge->AddChild(StateBox);
    PreviewViewStatus = MakeText(TEXT(""), 8, true, TextPrimary);
    PreviewViewStatus->SetJustification(ETextJustify::Right);
    StateBox->AddChildToVerticalBox(PreviewViewStatus);
    PreviewLightingStatus = MakeText(TEXT(""), 8, true, Muted);
    PreviewLightingStatus->SetJustification(ETextJustify::Right);
    StateBox->AddChildToVerticalBox(PreviewLightingStatus)->SetPadding(FMargin(0, 3, 0, 0));
    UpdatePreviewStatus();

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

    auto* PreviewHints = WidgetTree->ConstructWidget<UHorizontalBox>();
    PreviewHints->AddChildToHorizontalBox(MakeKeycap(TEXT("PPM")))
        ->SetPadding(FMargin(0, 0, 6, 0));
    PreviewHints->AddChildToHorizontalBox(MakeText(TEXT("OBRÓT"), 8, true, Muted))
        ->SetPadding(FMargin(0, 3, 12, 0));
    PreviewHints->AddChildToHorizontalBox(MakeKeycap(TEXT("KÓŁKO")))
        ->SetPadding(FMargin(0, 0, 6, 0));
    PreviewHints->AddChildToHorizontalBox(MakeText(TEXT("ZOOM"), 8, true, Muted))
        ->SetPadding(FMargin(0, 3, 0, 0));
    auto* PreviewHintsSlot = CaptionBox->AddChildToVerticalBox(PreviewHints);
    PreviewHintsSlot->SetHorizontalAlignment(HAlign_Center);
    PreviewHintsSlot->SetPadding(FMargin(0, 5, 0, 0));
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
        const float HealthValue = FMath::Clamp(Player->HealthState->Value, 0.0f, 100.0f);
        const float StaminaValue = FMath::Clamp(Player->StaminaState->Value, 0.0f, 100.0f);

        auto* HealthHead = WidgetTree->ConstructWidget<UHorizontalBox>();
        RightColumn->AddChildToVerticalBox(HealthHead)->SetPadding(FMargin(0, 0, 0, 5));
        HealthHead->AddChildToHorizontalBox(MakeText(TEXT("ZDROWIE"), 9, true, Muted))
            ->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        auto* HealthValueText = MakeText(
            FString::Printf(TEXT("%.0f%%"), HealthValue), 9, true, TextPrimary);
        HealthValueText->SetJustification(ETextJustify::Right);
        HealthHead->AddChildToHorizontalBox(HealthValueText);
        auto* Health = WidgetTree->ConstructWidget<UProgressBar>();
        Health->SetPercent(HealthValue / 100.0f);
        Health->SetFillColorAndOpacity(FLinearColor(0.91f, 0.29f, 0.28f, 1.0f));
        RightColumn->AddChildToVerticalBox(Health)->SetPadding(FMargin(0, 0, 0, 12));

        auto* StaminaHead = WidgetTree->ConstructWidget<UHorizontalBox>();
        RightColumn->AddChildToVerticalBox(StaminaHead)->SetPadding(FMargin(0, 0, 0, 5));
        StaminaHead->AddChildToHorizontalBox(MakeText(TEXT("KONDYCJA"), 9, true, Muted))
            ->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        auto* StaminaValueText = MakeText(
            FString::Printf(TEXT("%.0f%%"), StaminaValue), 9, true, TextPrimary);
        StaminaValueText->SetJustification(ETextJustify::Right);
        StaminaHead->AddChildToHorizontalBox(StaminaValueText);
        auto* Stamina = WidgetTree->ConstructWidget<UProgressBar>();
        Stamina->SetPercent(StaminaValue / 100.0f);
        Stamina->SetFillColorAndOpacity(Accent);
        RightColumn->AddChildToVerticalBox(Stamina)->SetPadding(FMargin(0, 0, 0, 18));
    }

    if (Mission)
    {
        auto* City = GetWorld()->GetSubsystem<UCityGameplaySubsystem>();
        const bool bCity = City && City->IsActive();

        auto* ObjectiveCard = WidgetTree->ConstructWidget<UBorder>();
        ObjectiveCard->SetBrush(RoundedBrush(PanelSoft, 11.0f));
        ObjectiveCard->SetPadding(FMargin(13, 12, 13, 12));
        RightColumn->AddChildToVerticalBox(ObjectiveCard)->SetPadding(FMargin(0, 0, 0, 14));

        auto* ObjectiveBox = WidgetTree->ConstructWidget<UVerticalBox>();
        ObjectiveCard->AddChild(ObjectiveBox);
        ObjectiveBox->AddChildToVerticalBox(MakeText(TEXT("AKTUALNY CEL"), 9, true, Accent))
            ->SetPadding(FMargin(0, 0, 0, 6));
        ObjectiveBox->AddChildToVerticalBox(MakeText(
            bCity ? City->NearbyObjective() : Mission->ObjectiveText(), 13, true, TextPrimary));

        RightColumn->AddChildToVerticalBox(MakeText(
            bCity ? TEXT("MIASTO") : TEXT("SESJA"), 9, true, Accent))
            ->SetPadding(FMargin(0, 0, 0, 7));

        RightColumn->AddChildToVerticalBox(MakeInfoRow(
            FString::Printf(TEXT("%d / 5"), Mission->WorldState.HeatLevel()),
            TEXT("ZAGROŻENIE")))
            ->SetPadding(FMargin(0, 0, 0, 7));

        if (bCity)
        {
            RightColumn->AddChildToVerticalBox(MakeInfoRow(
                FString::Printf(TEXT("%d / %d"), City->CompletedActivityCount(),
                    FMath::Max(City->TrackableActivityCount(), 1)),
                TEXT("AKTYWNOŚCI DZIELNIC"), true))
                ->SetPadding(FMargin(0, 0, 0, 7));
            RightColumn->AddChildToVerticalBox(MakeInfoRow(
                City->IsWriteBlocked() ? TEXT("ZABLOKOWANY") : TEXT("OK"),
                TEXT("ZAPIS MIASTA"), !City->IsWriteBlocked()));
        }
        else
        {
            int32 AchievementCount = 0;
            for (int32 Index = 0; Index < 6; ++Index)
                if ((Mission->LifetimeAchievements & (1 << Index)) != 0)
                    ++AchievementCount;

            RightColumn->AddChildToVerticalBox(MakeInfoRow(
                FString::Printf(TEXT("%.0f MIN"), Mission->State.elapsed / 60.0),
                TEXT("CZAS ROZGRYWKI")))
                ->SetPadding(FMargin(0, 0, 0, 7));
            RightColumn->AddChildToVerticalBox(MakeInfoRow(
                FString::Printf(TEXT("%d / 6"), AchievementCount),
                TEXT("OSIĄGNIĘCIA")))
                ->SetPadding(FMargin(0, 0, 0, 7));
            RightColumn->AddChildToVerticalBox(MakeInfoRow(
                Mission->bLastSaveSucceeded ? TEXT("OK") : TEXT("BŁĄD"),
                TEXT("OSTATNI ZAPIS"), Mission->bLastSaveSucceeded));
        }
    }
}

void UPlayerMenuWidget::BuildGameTab()
{
    PageTitle->SetText(FText::FromString(TEXT("CENTRUM GRACZA  /  GRA")));

    auto* Mission = GetGameInstance()->GetSubsystem<USliceMission>();
    auto* City = GetWorld()->GetSubsystem<UCityGameplaySubsystem>();
    const bool bCity = City && City->IsActive();
    const bool bInGame = Mission && Mission->bInGame;

    ActionColumn->AddChildToVerticalBox(MakeText(
        bCity ? TEXT("WROCŁAW") : TEXT("PRZEBUDZENIE"), 21, true, TextPrimary))
        ->SetPadding(FMargin(2, 1, 2, 2));
    ActionColumn->AddChildToVerticalBox(MakeText(
        bCity ? TEXT("OTWARTY ŚWIAT / WROCŁAW") : TEXT("KAMPANIA FABULARNA"), 9, true, Accent))
        ->SetPadding(FMargin(2, 0, 2, 12));
    ActionColumn->AddChildToVerticalBox(MakeText(
        bCity ? TEXT("Eksploruj dzielnice, odkrywaj aktywności i kontynuuj zapis miasta.")
              : (bInGame ? TEXT("Wróć do bieżącej sesji albo zarządzaj zapisem.")
                         : TEXT("Rozpocznij nową historię albo wczytaj istniejący zapis.")),
        11, false, Muted))->SetPadding(FMargin(2, 0, 2, 18));

    auto* ResumeButton = MakeButton(bInGame ? TEXT("KONTYNUUJ") : TEXT("BRAK AKTYWNEJ SESJI"), bInGame);
    ResumeButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::Resume);
    ResumeButton->SetIsEnabled(bInGame);
    ActionColumn->AddChildToVerticalBox(ResumeButton)->SetPadding(FMargin(0, 0, 0, 8));

    auto* NewButton = MakeButton(
        bCity ? TEXT("NOWY ZAPIS MIASTA") : TEXT("NOWA GRA"), !bInGame);
    NewButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::NewGame);
    ActionColumn->AddChildToVerticalBox(NewButton)->SetPadding(FMargin(0, 0, 0, 8));

    auto* LoadButton = MakeButton(bCity ? TEXT("WCZYTAJ MIASTO") : TEXT("WCZYTAJ ZAPIS"));
    LoadButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::LoadGame);
    const bool bHasSave = bCity || (Mission && Mission->HasSave());
    LoadButton->SetIsEnabled(bHasSave);
    ActionColumn->AddChildToVerticalBox(LoadButton)->SetPadding(FMargin(0, 0, 0, 10));

    ActionColumn->AddChildToVerticalBox(MakeInfoRow(
        bHasSave ? TEXT("DOSTĘPNY") : TEXT("BRAK"),
        TEXT("OSTATNI ZAPIS"),
        bHasSave))->SetPadding(FMargin(0, 0, 0, 18));

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

    AddGameHero();
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

    auto* Full = MakeButton(TEXT("CAŁA SYLWETKA"), PreviewViewLabel == TEXT("CAŁA SYLWETKA"));
    Full->OnClicked.AddDynamic(this, &UPlayerMenuWidget::PreviewFullBody);
    ActionColumn->AddChildToVerticalBox(Full)->SetPadding(FMargin(0, 0, 0, 7));
    PreviewViewButtons.Add(Full);

    auto* Upper = MakeButton(TEXT("GÓRNA CZĘŚĆ"), PreviewViewLabel == TEXT("GÓRNA CZĘŚĆ"));
    Upper->OnClicked.AddDynamic(this, &UPlayerMenuWidget::PreviewUpperBody);
    ActionColumn->AddChildToVerticalBox(Upper)->SetPadding(FMargin(0, 0, 0, 7));
    PreviewViewButtons.Add(Upper);

    auto* Face = MakeButton(TEXT("TWARZ"), PreviewViewLabel == TEXT("TWARZ"));
    Face->OnClicked.AddDynamic(this, &UPlayerMenuWidget::PreviewFace);
    ActionColumn->AddChildToVerticalBox(Face)->SetPadding(FMargin(0, 0, 0, 14));
    PreviewViewButtons.Add(Face);

    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("ŚWIATŁO"), 9, true, Muted))
        ->SetPadding(FMargin(2, 0, 2, 7));

    auto* Modern = MakeButton(TEXT("STUDIO"), PreviewLightingLabel == TEXT("STUDIO"));
    Modern->OnClicked.AddDynamic(this, &UPlayerMenuWidget::PreviewLightingModern);
    ActionColumn->AddChildToVerticalBox(Modern)->SetPadding(FMargin(0, 0, 0, 7));
    PreviewLightingButtons.Add(Modern);

    auto* Day = MakeButton(TEXT("DZIEŃ"), PreviewLightingLabel == TEXT("DZIEŃ"));
    Day->OnClicked.AddDynamic(this, &UPlayerMenuWidget::PreviewLightingDaylight);
    ActionColumn->AddChildToVerticalBox(Day)->SetPadding(FMargin(0, 0, 0, 7));
    PreviewLightingButtons.Add(Day);

    auto* Night = MakeButton(TEXT("NOC"), PreviewLightingLabel == TEXT("NOC"));
    Night->OnClicked.AddDynamic(this, &UPlayerMenuWidget::PreviewLightingNight);
    ActionColumn->AddChildToVerticalBox(Night)->SetPadding(FMargin(0, 0, 0, 14));
    PreviewLightingButtons.Add(Night);

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
    WardrobeBox->AddChildToVerticalBox(MakeText(
        FString::Printf(TEXT("GARDEROBA  •  %d"), ClothingCount), 10, true, Accent))
        ->SetPadding(FMargin(0, 0, 0, 9));
    auto* ClothesScroll = WidgetTree->ConstructWidget<UScrollBox>();
    WardrobeBox->AddChildToVerticalBox(ClothesScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    if (Creator && !Creator->Committed.OwnedClothing.IsEmpty())
    {
        TArray<FName> Clothing = Creator->Committed.OwnedClothing;
        Clothing.Sort([](const FName& A, const FName& B)
        {
            return A.ToString() < B.ToString();
        });
        for (const FName& Id : Clothing)
            ClothesScroll->AddChild(MakeInfoRow(Id.ToString().ToUpper(), TEXT("ELEMENT UBIORU")));
    }
    else
        ClothesScroll->AddChild(MakeInfoRow(TEXT("PUSTO"), TEXT("BRAK ELEMENTÓW GARDEROBY")));

    auto* Items = MakeCard(FMargin(14));
    auto* ItemsSlot = Columns->AddChildToHorizontalBox(Items);
    ItemsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ItemsSlot->SetPadding(FMargin(6, 0, 0, 0));
    auto* ItemsBox = WidgetTree->ConstructWidget<UVerticalBox>();
    Items->AddChild(ItemsBox);
    ItemsBox->AddChildToVerticalBox(MakeText(
        FString::Printf(TEXT("PRZEDMIOTY  •  %d"), ItemTypes), 10, true, Accent))
        ->SetPadding(FMargin(0, 0, 0, 9));
    auto* ItemsScroll = WidgetTree->ConstructWidget<UScrollBox>();
    ItemsBox->AddChildToVerticalBox(ItemsScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    TArray<TPair<FString, int32>> DisplayItems;
    if (Mission)
    {
        for (const auto& Pair : Mission->State.inventory)
        {
            if (Pair.second <= 0)
                continue;
            const auto* Item = Wroclaw::Progress::Item(Pair.first);
            const FString Name = Item ? UTF8_TO_TCHAR(Item->name.c_str()) : UTF8_TO_TCHAR(Pair.first.c_str());
            DisplayItems.Emplace(Name, Pair.second);
        }
    }
    DisplayItems.Sort([](const TPair<FString, int32>& A, const TPair<FString, int32>& B)
    {
        return A.Key < B.Key;
    });
    for (const auto& Item : DisplayItems)
        ItemsScroll->AddChild(MakeInfoRow(
            Item.Key.ToUpper(), FString::Printf(TEXT("ILOŚĆ  × %d"), Item.Value), Item.Value > 1));

    if (DisplayItems.IsEmpty())
        ItemsScroll->AddChild(MakeInfoRow(TEXT("PUSTO"), TEXT("BRAK PRZEDMIOTÓW")));

    AddPlayerStatus();
}

void UPlayerMenuWidget::BuildJournalTab()
{
    PageTitle->SetText(FText::FromString(TEXT("DZIENNIK  /  POSTĘP")));

    auto* Mission = GetGameInstance()->GetSubsystem<USliceMission>();
    auto* City = GetWorld()->GetSubsystem<UCityGameplaySubsystem>();

    const bool bCity = City && City->IsActive();
    int32 TotalMain = bCity ? City->TrackableActivityCount()
                            : static_cast<int32>(Wroclaw::Quests().size());
    int32 TotalSide = bCity ? City->EventCount()
                            : static_cast<int32>(Wroclaw::SideQuests().size());
    int32 MainDone = bCity ? City->CompletedActivityCount() : 0;
    int32 SideDone = bCity ? City->CompletedEventCount() : 0;
    if (!bCity && Mission)
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
        bCity ? TEXT("TRYB MIASTA") : TEXT("KAMPANIA"), 9, true, Accent))
        ->SetPadding(FMargin(2, 0, 2, 14));
    ActionColumn->AddChildToVerticalBox(MakeInfoRow(
        FString::Printf(TEXT("%d / %d"), MainDone, FMath::Max(TotalMain, 1)),
        bCity ? TEXT("AKTYWNOŚCI DZIELNIC") : TEXT("ETAPY GŁÓWNE"), true))
        ->SetPadding(FMargin(0, 0, 0, 7));
    ActionColumn->AddChildToVerticalBox(MakeInfoRow(
        FString::Printf(TEXT("%d / %d"), SideDone, FMath::Max(TotalSide, 1)),
        bCity ? TEXT("ZDARZENIA AMBIENTOWE") : TEXT("ZADANIA POBOCZNE")))
        ->SetPadding(FMargin(0, 0, 0, 7));
    ActionColumn->AddChildToVerticalBox(MakeInfoRow(
        Mission ? FString::Printf(TEXT("%d"), static_cast<int32>(Mission->State.evidence.size())) : TEXT("0"),
        TEXT("ZEBRANE DOWODY")));

    CenterColumn->AddChildToVerticalBox(MakeText(
        bCity ? TEXT("POSTĘP MIASTA") : TEXT("POSTĘP FABULARNY"), 26, true, TextPrimary))
        ->SetPadding(FMargin(8, 7, 8, 8));

    auto* ProgressPanel = MakeCard(FMargin(14, 12, 14, 12));
    CenterColumn->AddChildToVerticalBox(ProgressPanel)->SetPadding(FMargin(8, 0, 8, 12));
    auto* ProgressBox = WidgetTree->ConstructWidget<UVerticalBox>();
    ProgressPanel->AddChild(ProgressBox);

    auto* MainProgressHead = WidgetTree->ConstructWidget<UHorizontalBox>();
    ProgressBox->AddChildToVerticalBox(MainProgressHead)->SetPadding(FMargin(0, 0, 0, 5));
    MainProgressHead->AddChildToHorizontalBox(MakeText(
        bCity ? TEXT("AKTYWNOŚCI DZIELNIC") : TEXT("WĄTEK GŁÓWNY"), 9, true, Muted))
        ->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* MainProgressValue = MakeText(
        FString::Printf(TEXT("%d / %d"), MainDone, FMath::Max(TotalMain, 1)), 9, true, Accent);
    MainProgressValue->SetJustification(ETextJustify::Right);
    MainProgressHead->AddChildToHorizontalBox(MainProgressValue);
    auto* MainProgress = WidgetTree->ConstructWidget<UProgressBar>();
    MainProgress->SetPercent(TotalMain > 0 ? static_cast<float>(MainDone) / TotalMain : 0.0f);
    MainProgress->SetFillColorAndOpacity(Accent);
    ProgressBox->AddChildToVerticalBox(MainProgress)->SetPadding(FMargin(0, 0, 0, 11));

    auto* SideProgressHead = WidgetTree->ConstructWidget<UHorizontalBox>();
    ProgressBox->AddChildToVerticalBox(SideProgressHead)->SetPadding(FMargin(0, 0, 0, 5));
    SideProgressHead->AddChildToHorizontalBox(MakeText(
        bCity ? TEXT("ZDARZENIA AMBIENTOWE") : TEXT("ZADANIA POBOCZNE"), 9, true, Muted))
        ->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* SideProgressValue = MakeText(
        FString::Printf(TEXT("%d / %d"), SideDone, FMath::Max(TotalSide, 1)), 9, true, Accent);
    SideProgressValue->SetJustification(ETextJustify::Right);
    SideProgressHead->AddChildToHorizontalBox(SideProgressValue);
    auto* SideProgress = WidgetTree->ConstructWidget<UProgressBar>();
    SideProgress->SetPercent(TotalSide > 0 ? static_cast<float>(SideDone) / TotalSide : 0.0f);
    SideProgress->SetFillColorAndOpacity(Accent);
    ProgressBox->AddChildToVerticalBox(SideProgress);

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

    if (bCity)
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
    auto* CityGameplay = GetWorld()->GetSubsystem<UCityGameplaySubsystem>();
    auto* MapSubsystem = GetWorld()->GetSubsystem<UWroclawMapSubsystem>();
    const bool bCity = CityGameplay && CityGameplay->IsActive();
    const APawn* Player = GetOwningPlayerPawn();
    const FVector PlayerPosition = Player ? Player->GetActorLocation() : FVector::ZeroVector;

    constexpr float MapWidth = 760.0f;
    constexpr float MapHeight = 470.0f;

    auto MakeCanvasFrame = [&]() -> UCanvasPanel*
    {
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

        auto AddCompassLabel = [&](const FString& Label, const FVector2D& Position)
        {
            auto* Badge = WidgetTree->ConstructWidget<UBorder>();
            Badge->SetBrush(RoundedBrush(FLinearColor(0.015f, 0.040f, 0.055f, 0.94f), 7.0f));
            Badge->SetPadding(FMargin(7, 4));
            Badge->AddChild(MakeText(Label, 9, true, Accent));
            auto* BadgeSlot = Canvas->AddChildToCanvas(Badge);
            BadgeSlot->SetAutoSize(true);
            BadgeSlot->SetAlignment(FVector2D(0.5f, 0.5f));
            BadgeSlot->SetPosition(Position);
        };
        AddCompassLabel(TEXT("N"), FVector2D(MapWidth * 0.5f, 18.0f));
        AddCompassLabel(TEXT("E"), FVector2D(MapWidth - 28.0f, MapHeight * 0.5f));
        AddCompassLabel(TEXT("S"), FVector2D(MapWidth * 0.5f, MapHeight - 24.0f));
        AddCompassLabel(TEXT("W"), FVector2D(20.0f, MapHeight * 0.5f));

        auto* MapMode = WidgetTree->ConstructWidget<UBorder>();
        MapMode->SetBrush(RoundedBrush(FLinearColor(0.010f, 0.028f, 0.040f, 0.92f), 8.0f));
        MapMode->SetPadding(FMargin(10, 5));
        MapMode->AddChild(MakeText(TEXT("WROCŁAW  •  MAPA 2D"), 8, true, Muted));
        auto* ModeSlot = Canvas->AddChildToCanvas(MapMode);
        ModeSlot->SetAutoSize(true);
        ModeSlot->SetAlignment(FVector2D(1.0f, 0.0f));
        ModeSlot->SetPosition(FVector2D(MapWidth - 18.0f, 16.0f));

        return Canvas;
    };

    auto AddCanvasMarker = [&](UCanvasPanel* Canvas, const FString& Label, const FVector2D& Position,
                               const FLinearColor& Color, const FLinearColor& TextColor)
    {
        auto* Marker = WidgetTree->ConstructWidget<UBorder>();
        Marker->SetBrush(RoundedBrush(Color, 7.0f));
        Marker->SetPadding(FMargin(8, 5));
        Marker->AddChild(MakeText(Label, 9, true, TextColor));
        auto* Slot = Canvas->AddChildToCanvas(Marker);
        Slot->SetAutoSize(true);
        Slot->SetAlignment(FVector2D(0.5f, 0.5f));
        Slot->SetPosition(Position);
    };

    if (bCity && MapSubsystem && MapSubsystem->GetCity() &&
        !MapSubsystem->GetCity()->Sectors.IsEmpty())
    {
        const UCityDefinition* Definition = MapSubsystem->GetCity();
        const FCitySectorDefinition* CurrentSector = Player ? MapSubsystem->SectorAt(PlayerPosition) : nullptr;
        const FName WaypointId = MapSubsystem->GetWaypoint();

        int32 VisibleSectors = 0;
        int32 Buildings = 0;
        int32 Roads = 0;
        for (const auto& Sector : Definition->Sectors)
        {
            if (Sector.Status != ECityCoverageStatus::Missing)
                ++VisibleSectors;
            Buildings += Sector.BuildingCount;
            Roads += Sector.RoadCount;
        }

        ActionColumn->AddChildToVerticalBox(MakeText(TEXT("WROCŁAW"), 20, true, TextPrimary))
            ->SetPadding(FMargin(2, 1, 2, 2));
        ActionColumn->AddChildToVerticalBox(MakeText(TEXT("MAPA MIASTA / SEKTORY"), 9, true, Accent))
            ->SetPadding(FMargin(2, 0, 2, 14));
        ActionColumn->AddChildToVerticalBox(MakeInfoRow(
            FString::Printf(TEXT("%d"), VisibleSectors), TEXT("AKTYWNE SEKTORY"), true))
            ->SetPadding(FMargin(0, 0, 0, 7));
        ActionColumn->AddChildToVerticalBox(MakeInfoRow(
            FString::Printf(TEXT("%d"), Buildings), TEXT("BUDYNKI")))
            ->SetPadding(FMargin(0, 0, 0, 7));
        ActionColumn->AddChildToVerticalBox(MakeInfoRow(
            FString::Printf(TEXT("%d"), Roads), TEXT("ODCINKI DRÓG")));

        CenterColumn->AddChildToVerticalBox(MakeText(TEXT("SEKTORY MIASTA"), 26, true, TextPrimary))
            ->SetPadding(FMargin(8, 7, 8, 8));
        UCanvasPanel* Canvas = MakeCanvasFrame();

        double MinX = TNumericLimits<double>::Max();
        double MaxX = TNumericLimits<double>::Lowest();
        double MinY = TNumericLimits<double>::Max();
        double MaxY = TNumericLimits<double>::Lowest();

        for (const auto& Sector : Definition->Sectors)
            for (const FVector2D& Point : Sector.Boundary)
            {
                MinX = FMath::Min(MinX, static_cast<double>(Point.X));
                MaxX = FMath::Max(MaxX, static_cast<double>(Point.X));
                MinY = FMath::Min(MinY, static_cast<double>(Point.Y));
                MaxY = FMath::Max(MaxY, static_cast<double>(Point.Y));
            }

        const double SpanX = FMath::Max(MaxX - MinX, 1.0);
        const double SpanY = FMath::Max(MaxY - MinY, 1.0);
        auto ToCanvas = [&](double X, double Y)
        {
            return FVector2D(
                28.0f + static_cast<float>((X - MinX) / SpanX) * (MapWidth - 80.0f),
                24.0f + (1.0f - static_cast<float>((Y - MinY) / SpanY)) * (MapHeight - 60.0f));
        };

        for (const auto& Sector : Definition->Sectors)
        {
            if (Sector.Status == ECityCoverageStatus::Missing || Sector.Boundary.IsEmpty())
                continue;

            FVector2D Center = FVector2D::ZeroVector;
            for (const FVector2D& Point : Sector.Boundary)
                Center += Point;
            Center /= Sector.Boundary.Num();

            const bool bCurrent = CurrentSector && CurrentSector->Id == Sector.Id;
            const bool bWaypoint = WaypointId == Sector.Id;
            const FLinearColor MarkerColor = bCurrent
                ? FLinearColor(0.04f, 0.30f, 0.35f, 0.98f)
                : (bWaypoint ? FLinearColor(0.40f, 0.27f, 0.08f, 0.98f)
                             : FLinearColor(0.035f, 0.075f, 0.10f, 0.94f));
            const FString Prefix = bCurrent ? TEXT("● ") : (bWaypoint ? TEXT("◆ ") : TEXT(""));
            AddCanvasMarker(
                Canvas, Prefix + Sector.DisplayName, ToCanvas(Center.X, Center.Y),
                MarkerColor, bCurrent ? Accent : TextPrimary);
        }

        if (Player)
        {
            AddCanvasMarker(
                Canvas, TEXT("TY"),
                ToCanvas(PlayerPosition.X, PlayerPosition.Y),
                FLinearColor(Accent.R, Accent.G, Accent.B, 1.0f),
                Background);
        }

        RightColumn->AddChildToVerticalBox(MakeText(TEXT("POZYCJA"), 9, true, Accent))
            ->SetPadding(FMargin(0, 0, 0, 7));
        RightColumn->AddChildToVerticalBox(MakeInfoRow(
            CurrentSector ? CurrentSector->DisplayName.ToUpper() : TEXT("POZA SEKTORAMI"),
            TEXT("AKTUALNY SEKTOR"), true))
            ->SetPadding(FMargin(0, 0, 0, 7));

        FString WaypointName = TEXT("BRAK");
        double WaypointDistance = 0.0;
        if (!WaypointId.IsNone())
        {
            if (const FCitySectorDefinition* Waypoint = Definition->Sectors.FindByPredicate(
                    [WaypointId](const FCitySectorDefinition& Sector) { return Sector.Id == WaypointId; }))
            {
                WaypointName = Waypoint->DisplayName.ToUpper();
                if (!Waypoint->Boundary.IsEmpty() && Player)
                {
                    FVector2D Center = FVector2D::ZeroVector;
                    for (const FVector2D& Point : Waypoint->Boundary)
                        Center += Point;
                    Center /= Waypoint->Boundary.Num();
                    WaypointDistance = FVector2D::Distance(
                        FVector2D(PlayerPosition.X, PlayerPosition.Y), Center) / 100.0;
                }
            }
        }

        RightColumn->AddChildToVerticalBox(MakeInfoRow(
            WaypointName,
            WaypointId.IsNone() ? TEXT("CEL SEKTORA")
                                : FString::Printf(TEXT("CEL  •  %.0f M"), WaypointDistance)))
            ->SetPadding(FMargin(0, 0, 0, 14));

        RightColumn->AddChildToVerticalBox(MakeText(TEXT("LEGENDA"), 9, true, Accent))
            ->SetPadding(FMargin(0, 0, 0, 7));
        RightColumn->AddChildToVerticalBox(MakeInfoRow(TEXT("● TURKUS"), TEXT("TY / AKTUALNY SEKTOR"), true))
            ->SetPadding(FMargin(0, 0, 0, 7));
        RightColumn->AddChildToVerticalBox(MakeInfoRow(TEXT("◆ ZŁOTY"), TEXT("WYBRANY CEL")));

        RightColumn->AddChildToVerticalBox(MakeText(TEXT("STEROWANIE"), 9, true, Accent))
            ->SetPadding(FMargin(0, 14, 0, 7));
        RightColumn->AddChildToVerticalBox(MakeText(
            TEXT("C — następny sektor\nBACKSPACE — usuń cel"), 10, false, Muted));
        return;
    }

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
    UCanvasPanel* Canvas = MakeCanvasFrame();

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
        auto ToCanvas = [&](double X, double Y)
        {
            return FVector2D(
                28.0f + static_cast<float>((X - MinX) / SpanX) * (MapWidth - 80.0f),
                24.0f + (1.0f - static_cast<float>((Y - MinY) / SpanY)) * (MapHeight - 60.0f));
        };

        FString NearestName = TEXT("BRAK");
        double NearestDistance = TNumericLimits<double>::Max();

        for (const auto& Location : Locations)
        {
            if (!Mission || !Mission->WorldState.discoveries.count(Location.id))
                continue;

            AddCanvasMarker(
                Canvas,
                UTF8_TO_TCHAR(Location.name.c_str()),
                ToCanvas(Location.position[0], Location.position[1]),
                Location.safehouse ? FLinearColor(0.06f, 0.30f, 0.34f, 0.98f)
                                   : FLinearColor(0.04f, 0.08f, 0.11f, 0.95f),
                Location.safehouse ? Accent : TextPrimary);

            if (Player)
            {
                const FVector Point(Location.position[0], Location.position[1], Location.position[2]);
                const double Distance = FVector::Dist(PlayerPosition, Point);
                if (Distance < NearestDistance)
                {
                    NearestDistance = Distance;
                    NearestName = UTF8_TO_TCHAR(Location.name.c_str());
                }
            }
        }

        const bool bInsideMap = Player &&
            PlayerPosition.X >= MinX && PlayerPosition.X <= MaxX &&
            PlayerPosition.Y >= MinY && PlayerPosition.Y <= MaxY;
        if (bInsideMap)
        {
            AddCanvasMarker(
                Canvas, TEXT("TY"),
                ToCanvas(PlayerPosition.X, PlayerPosition.Y),
                FLinearColor(Accent.R, Accent.G, Accent.B, 1.0f),
                Background);
        }

        RightColumn->AddChildToVerticalBox(MakeText(TEXT("POZYCJA"), 9, true, Accent))
            ->SetPadding(FMargin(0, 0, 0, 7));
        RightColumn->AddChildToVerticalBox(MakeInfoRow(
            bInsideMap ? TEXT("NA MAPIE ODKRYĆ") : TEXT("POZA OBSZAREM MAPY"),
            TEXT("AKTUALNA POZYCJA"), bInsideMap))
            ->SetPadding(FMargin(0, 0, 0, 7));

        RightColumn->AddChildToVerticalBox(MakeInfoRow(
            NearestName.ToUpper(),
            NearestDistance < TNumericLimits<double>::Max()
                ? FString::Printf(TEXT("NAJBLIŻEJ  •  %.0f M"), NearestDistance / 100.0)
                : TEXT("NAJBLIŻSZE ODKRYTE MIEJSCE")))
            ->SetPadding(FMargin(0, 0, 0, 14));
    }

    RightColumn->AddChildToVerticalBox(MakeText(TEXT("LEGENDA"), 9, true, Accent))
        ->SetPadding(FMargin(0, 0, 0, 7));
    RightColumn->AddChildToVerticalBox(MakeInfoRow(TEXT("TY"), TEXT("AKTUALNA POZYCJA"), true))
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
    auto* Mission = GetGameInstance()->GetSubsystem<USliceMission>();
    auto* City = GetWorld()->GetSubsystem<UCityGameplaySubsystem>();
    const bool bCity = City && City->IsActive();

    PageTitle->SetText(FText::FromString(
        bCity ? TEXT("STATYSTYKI  /  OTWARTY ŚWIAT") : TEXT("STATYSTYKI  /  SESJA")));

    ActionColumn->AddChildToVerticalBox(MakeText(
        bCity ? TEXT("MIASTO") : TEXT("SESJA"), 20, true, TextPrimary))
        ->SetPadding(FMargin(2, 1, 2, 2));
    ActionColumn->AddChildToVerticalBox(MakeText(
        bCity ? TEXT("WROCŁAW / OTWARTY ŚWIAT") : TEXT("PODSUMOWANIE"), 9, true, Accent))
        ->SetPadding(FMargin(2, 0, 2, 14));

    if (!Mission)
    {
        ActionColumn->AddChildToVerticalBox(MakeInfoRow(TEXT("BRAK"), TEXT("AKTYWNEJ SESJI")));
        AddTextPage(TEXT("STATYSTYKI"), TEXT("Brak danych."));
        return;
    }

    if (bCity)
    {
        const int32 Completed = City->CompletedActivityCount();
        const int32 Total = City->TrackableActivityCount();
        const int32 EventsDone = City->CompletedEventCount();
        const int32 EventsTotal = City->EventCount();
        const int32 Available = City->AvailableActivityCount();
        const int32 Discoveries = static_cast<int32>(Mission->WorldState.discoveries.size());

        ActionColumn->AddChildToVerticalBox(MakeInfoRow(
            TEXT("AKTYWNY"), TEXT("TRYB OTWARTEGO ŚWIATA"), true))
            ->SetPadding(FMargin(0, 0, 0, 7));
        ActionColumn->AddChildToVerticalBox(MakeInfoRow(
            City->IsWriteBlocked() ? TEXT("ZABLOKOWANY") : TEXT("POPRAWNY"),
            TEXT("AUTOMATYCZNY ZAPIS"), !City->IsWriteBlocked()))
            ->SetPadding(FMargin(0, 0, 0, 7));
        ActionColumn->AddChildToVerticalBox(MakeInfoRow(
            FString::Printf(TEXT("%d / 5"), Mission->WorldState.HeatLevel()),
            TEXT("ZAGROŻENIE")));

        CenterColumn->AddChildToVerticalBox(MakeText(TEXT("METRYKI MIASTA"), 26, true, TextPrimary))
            ->SetPadding(FMargin(8, 7, 8, 10));

        auto* Metrics = WidgetTree->ConstructWidget<UVerticalBox>();
        CenterColumn->AddChildToVerticalBox(Metrics)->SetPadding(FMargin(8, 0, 8, 8));

        Metrics->AddChildToVerticalBox(MakeInfoRow(
            FString::Printf(TEXT("%d / %d"), Completed, FMath::Max(Total, 1)),
            TEXT("AKTYWNOŚCI DZIELNIC"), true))
            ->SetPadding(FMargin(0, 0, 0, 7));
        Metrics->AddChildToVerticalBox(MakeInfoRow(
            FString::Printf(TEXT("%d / %d"), EventsDone, FMath::Max(EventsTotal, 1)),
            TEXT("ZDARZENIA AMBIENTOWE")))
            ->SetPadding(FMargin(0, 0, 0, 7));
        Metrics->AddChildToVerticalBox(MakeInfoRow(
            FString::Printf(TEXT("%d"), Available), TEXT("DOSTĘPNE TERAZ")))
            ->SetPadding(FMargin(0, 0, 0, 7));
        Metrics->AddChildToVerticalBox(MakeInfoRow(
            FString::Printf(TEXT("%d"), Discoveries), TEXT("ODKRYTE LOKACJE")))
            ->SetPadding(FMargin(0, 0, 0, 7));
        Metrics->AddChildToVerticalBox(MakeInfoRow(
            FString::Printf(TEXT("%d / 5"), Mission->WorldState.HeatLevel()),
            TEXT("AKTUALNY HEAT")));

        RightColumn->AddChildToVerticalBox(MakeText(TEXT("NAJBLIŻSZY CEL"), 9, true, Accent))
            ->SetPadding(FMargin(0, 0, 0, 8));
        RightColumn->AddChildToVerticalBox(MakeInfoRow(
            City->NearbyObjective(), TEXT("OTWARTY ŚWIAT"), true))
            ->SetPadding(FMargin(0, 0, 0, 14));

        RightColumn->AddChildToVerticalBox(MakeText(TEXT("POSTĘP"), 9, true, Accent))
            ->SetPadding(FMargin(0, 0, 0, 8));
        auto* Progress = WidgetTree->ConstructWidget<UProgressBar>();
        Progress->SetPercent(Total > 0 ? static_cast<float>(Completed) / Total : 0.0f);
        Progress->SetFillColorAndOpacity(Accent);
        RightColumn->AddChildToVerticalBox(Progress)->SetPadding(FMargin(0, 0, 0, 7));
        RightColumn->AddChildToVerticalBox(MakeText(
            FString::Printf(TEXT("%.0f%% aktywności dzielnic ukończonych"),
                Total > 0 ? 100.0f * Completed / Total : 0.0f),
            10, false, Muted));
        return;
    }

    const int32 MainTotal = static_cast<int32>(Wroclaw::Quests().size());
    int32 MainDone = 0;
    for (const auto& Quest : Wroclaw::Quests())
        if (Mission->State.QuestComplete(Quest))
            ++MainDone;
    const float CampaignProgress = MainTotal > 0 ? static_cast<float>(MainDone) / MainTotal : 0.0f;

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
        FString::Printf(TEXT("%d / %d"), MainDone, FMath::Max(MainTotal, 1)),
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

    auto* CampaignProgressCard = MakeCard(FMargin(14, 12, 14, 12));
    CenterColumn->AddChildToVerticalBox(CampaignProgressCard)->SetPadding(FMargin(8, 6, 8, 8));
    auto* CampaignProgressBox = WidgetTree->ConstructWidget<UVerticalBox>();
    CampaignProgressCard->AddChild(CampaignProgressBox);
    auto* CampaignProgressHead = WidgetTree->ConstructWidget<UHorizontalBox>();
    CampaignProgressBox->AddChildToVerticalBox(CampaignProgressHead)->SetPadding(FMargin(0, 0, 0, 6));
    CampaignProgressHead->AddChildToHorizontalBox(MakeText(TEXT("POSTĘP KAMPANII"), 9, true, Muted))
        ->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* CampaignProgressValue = MakeText(
        FString::Printf(TEXT("%.0f%%"), CampaignProgress * 100.0f), 9, true, Accent);
    CampaignProgressValue->SetJustification(ETextJustify::Right);
    CampaignProgressHead->AddChildToHorizontalBox(CampaignProgressValue);
    auto* CampaignProgressBar = WidgetTree->ConstructWidget<UProgressBar>();
    CampaignProgressBar->SetPercent(CampaignProgress);
    CampaignProgressBar->SetFillColorAndOpacity(Accent);
    CampaignProgressBox->AddChildToVerticalBox(CampaignProgressBar);

    RightColumn->AddChildToVerticalBox(MakeText(TEXT("OSIĄGNIĘCIA"), 9, true, Accent))
        ->SetPadding(FMargin(0, 0, 0, 8));

    const TCHAR* Names[] = {
        TEXT("Pierwsze kroki"), TEXT("Escape Artist"), TEXT("Bez śladu"),
        TEXT("Detektyw"), TEXT("Pacyfista"), TEXT("Szybkie myślenie")
    };
    int32 UnlockedAchievements = 0;
    for (int32 Index = 0; Index < 6; ++Index)
        if ((Mission->LifetimeAchievements & (1 << Index)) != 0)
            ++UnlockedAchievements;

    RightColumn->AddChildToVerticalBox(MakeInfoRow(
        FString::Printf(TEXT("%d / 6"), UnlockedAchievements), TEXT("ODBLOKOWANE"), true))
        ->SetPadding(FMargin(0, 0, 0, 10));

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
    auto* Preferences = UWTGPerformanceSettings::Get();
    auto* AudioPreferences = UWTGAudioSettings::Get();
    UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr;

    ActionColumn->AddChildToVerticalBox(MakeText(TEXT("USTAWIENIA"), 20, true, TextPrimary))
        ->SetPadding(FMargin(2, 1, 2, 2));
    ActionColumn->AddChildToVerticalBox(MakeText(
        FString::Printf(TEXT("KATEGORIE  •  %d / 4"), SettingsSection + 1), 9, true, Accent))
        ->SetPadding(FMargin(2, 0, 2, 7));

    auto* SettingsNavProgress = WidgetTree->ConstructWidget<UProgressBar>();
    SettingsNavProgress->SetPercent(FMath::Clamp((SettingsSection + 1) / 4.0f, 0.0f, 1.0f));
    SettingsNavProgress->SetFillColorAndOpacity(Accent);
    auto* SettingsNavProgressSize = WidgetTree->ConstructWidget<USizeBox>();
    SettingsNavProgressSize->SetHeightOverride(3.0f);
    SettingsNavProgressSize->AddChild(SettingsNavProgress);
    ActionColumn->AddChildToVerticalBox(SettingsNavProgressSize)->SetPadding(FMargin(2, 0, 2, 13));

    auto* DisplaySection = MakeButton(TEXT("01  OBRAZ"), SettingsSection == 0);
    DisplaySection->OnClicked.AddDynamic(this, &UPlayerMenuWidget::SettingsDisplay);
    ActionColumn->AddChildToVerticalBox(DisplaySection)->SetPadding(FMargin(0, 0, 0, 7));

    auto* PerformanceSection = MakeButton(TEXT("02  WYDAJNOŚĆ"), SettingsSection == 1);
    PerformanceSection->OnClicked.AddDynamic(this, &UPlayerMenuWidget::SettingsPerformance);
    ActionColumn->AddChildToVerticalBox(PerformanceSection)->SetPadding(FMargin(0, 0, 0, 7));

    auto* InterfaceSection = MakeButton(TEXT("03  INTERFEJS"), SettingsSection == 2);
    InterfaceSection->OnClicked.AddDynamic(this, &UPlayerMenuWidget::SettingsInterface);
    ActionColumn->AddChildToVerticalBox(InterfaceSection)->SetPadding(FMargin(0, 0, 0, 7));

    auto* AudioSection = MakeButton(TEXT("04  DŹWIĘK"), SettingsSection == 3);
    AudioSection->OnClicked.AddDynamic(this, &UPlayerMenuWidget::SettingsAudio);
    ActionColumn->AddChildToVerticalBox(AudioSection)->SetPadding(FMargin(0, 0, 0, 18));

    auto* ResetButton = MakeButton(TEXT("PRZYWRÓĆ DOMYŚLNE"));
    ResetButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::ResetSettings);
    ActionColumn->AddChildToVerticalBox(ResetButton)->SetPadding(FMargin(0, 0, 0, 14));

    ActionColumn->AddChildToVerticalBox(MakeInfoRow(
        TEXT("AUTOMATYCZNY ZAPIS"),
        TEXT("ZMIANY USTAWIEŃ"),
        true))->SetPadding(FMargin(0, 0, 0, 10));
    ActionColumn->AddChildToVerticalBox(MakeText(
        TEXT("Ustawienia są stosowane od razu. Zmiany trybu obrazu mają bezpieczne potwierdzenie z automatycznym cofnięciem."),
        10, false, Muted));

    if (!UserSettings)
    {
        PageTitle->SetText(FText::FromString(TEXT("USTAWIENIA  /  NIEDOSTĘPNE")));
        AddTextPage(TEXT("USTAWIENIA"), TEXT("Nie udało się pobrać UGameUserSettings."));
        return;
    }

    const bool bFPSVisible = Preferences && Preferences->bShowFPS;
    const bool bReduceMotion = Preferences && Preferences->bReduceUIMotion;
    const bool bMenuBlur = !Preferences || Preferences->bMenuBackgroundBlur;
    const bool bUISounds = !Preferences || Preferences->bUISounds;
    const float SFXVolume = AudioPreferences ? AudioPreferences->SFXVolume : 1.0f;
    const float UIVolume = AudioPreferences ? AudioPreferences->UIVolume : 1.0f;
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
    const FString LimitLabel = Limit == 0 ? TEXT("BEZ LIMITU") : FString::Printf(TEXT("%d FPS"), Limit);

    auto AddCenterButton = [&](UButton* Button, float Bottom = 8.0f)
    {
        CenterColumn->AddChildToVerticalBox(Button)->SetPadding(FMargin(18, 0, 18, Bottom));
    };

    if (SettingsSection == 0)
    {
        PageTitle->SetText(FText::FromString(TEXT("USTAWIENIA  /  OBRAZ")));
        CenterColumn->AddChildToVerticalBox(MakeText(TEXT("OBRAZ"), 22, true, TextPrimary))
            ->SetPadding(FMargin(18, 16, 18, 3));
        CenterColumn->AddChildToVerticalBox(MakeText(
            TEXT("TRYB EKRANU, ROZDZIELCZOŚĆ I JAKOŚĆ RENDEROWANIA"), 9, true, Accent))
            ->SetPadding(FMargin(18, 0, 18, 18));

        auto* ModeButton = MakeButton(
            FString::Printf(TEXT("TRYB EKRANU  •  %s"), *WindowModeLabel(WindowMode)), true);
        ModeButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::CycleWindowMode);
        AddCenterButton(ModeButton);

        auto* ResolutionButton = MakeButton(
            FString::Printf(TEXT("ROZDZIELCZOŚĆ  •  %d × %d"), Resolution.X, Resolution.Y));
        ResolutionButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::CycleResolution);
        AddCenterButton(ResolutionButton);

        auto* QualityButton = MakeButton(
            FString::Printf(TEXT("PRESET JAKOŚCI  •  %s"), *QualityLabel(Quality)));
        QualityButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::CycleQuality);
        AddCenterButton(QualityButton);

        RightColumn->AddChildToVerticalBox(MakeText(TEXT("AKTUALNY OBRAZ"), 9, true, Accent))
            ->SetPadding(FMargin(0, 0, 0, 8));
        RightColumn->AddChildToVerticalBox(MakeInfoRow(
            WindowModeLabel(WindowMode), TEXT("TRYB EKRANU"), true))->SetPadding(FMargin(0, 0, 0, 7));
        RightColumn->AddChildToVerticalBox(MakeInfoRow(
            FString::Printf(TEXT("%d × %d"), Resolution.X, Resolution.Y), TEXT("ROZDZIELCZOŚĆ")))
            ->SetPadding(FMargin(0, 0, 0, 7));
        RightColumn->AddChildToVerticalBox(MakeInfoRow(
            QualityLabel(Quality), TEXT("PRESET JAKOŚCI")))->SetPadding(FMargin(0, 0, 0, 14));

        RightColumn->AddChildToVerticalBox(MakeText(TEXT("SZCZEGÓŁY JAKOŚCI"), 9, true, Accent))
            ->SetPadding(FMargin(0, 0, 0, 8));

        auto AddQualityRow = [&](const FString& Label, int32 Value, float Bottom = 6.0f)
        {
            RightColumn->AddChildToVerticalBox(MakeInfoRow(
                FString::Printf(TEXT("%d / 4"), Value), Label))
                ->SetPadding(FMargin(0, 0, 0, Bottom));
        };

        AddQualityRow(TEXT("WIDOCZNOŚĆ"), UserSettings->GetViewDistanceQuality());
        AddQualityRow(TEXT("CIENIE"), UserSettings->GetShadowQuality());
        AddQualityRow(TEXT("TEKSTURY"), UserSettings->GetTextureQuality());
        AddQualityRow(TEXT("ANTYALIASING"), UserSettings->GetAntiAliasingQuality());
        AddQualityRow(TEXT("EFEKTY"), UserSettings->GetVisualEffectQuality());
        AddQualityRow(TEXT("POST-PROCESSING"), UserSettings->GetPostProcessingQuality());
        AddQualityRow(TEXT("ROŚLINNOŚĆ"), UserSettings->GetFoliageQuality());
        AddQualityRow(TEXT("GLOBALNE OŚWIETLENIE"), UserSettings->GetGlobalIlluminationQuality());
        AddQualityRow(TEXT("ODBICIA"), UserSettings->GetReflectionQuality());
        AddQualityRow(TEXT("CIENIOWANIE"), UserSettings->GetShadingQuality(), 0.0f);
        return;
    }

    if (SettingsSection == 1)
    {
        PageTitle->SetText(FText::FromString(TEXT("USTAWIENIA  /  WYDAJNOŚĆ")));
        CenterColumn->AddChildToVerticalBox(MakeText(TEXT("WYDAJNOŚĆ"), 22, true, TextPrimary))
            ->SetPadding(FMargin(18, 16, 18, 3));
        CenterColumn->AddChildToVerticalBox(MakeText(
            TEXT("PŁYNNOŚĆ, SKALOWANIE I LIMIT KLATEK"), 9, true, Accent))
            ->SetPadding(FMargin(18, 0, 18, 18));

        auto* ScaleButton = MakeButton(
            FString::Printf(TEXT("SKALA RENDERU  •  %.0f%%"), ScaleValue), true);
        ScaleButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::CycleResolutionScale);
        AddCenterButton(ScaleButton);

        auto* VSyncButton = MakeButton(
            FString::Printf(TEXT("VSYNC  •  %s"), bVSync ? TEXT("WŁ.") : TEXT("WYŁ.")), bVSync);
        VSyncButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::ToggleVSync);
        AddCenterButton(VSyncButton);

        auto* DynamicButton = MakeButton(
            FString::Printf(TEXT("DYNAMICZNA ROZDZIELCZOŚĆ  •  %s"),
                bDynamicResolution ? TEXT("WŁ.") : TEXT("WYŁ.")), bDynamicResolution);
        DynamicButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::ToggleDynamicResolution);
        AddCenterButton(DynamicButton);

        auto* LimitButton = MakeButton(FString::Printf(TEXT("LIMIT KLATEK  •  %s"), *LimitLabel));
        LimitButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::CycleFPSLimit);
        AddCenterButton(LimitButton, 12.0f);

        CenterColumn->AddChildToVerticalBox(MakeText(TEXT("SZYBKI LIMIT FPS"), 9, true, Muted))
            ->SetPadding(FMargin(18, 0, 18, 7));
        auto* FPSPresets = WidgetTree->ConstructWidget<UHorizontalBox>();
        CenterColumn->AddChildToVerticalBox(FPSPresets)->SetPadding(FMargin(18, 0, 18, 12));

        auto AddFPSPresetSlot = [&](UButton* Button)
        {
            auto* Slot = FPSPresets->AddChildToHorizontalBox(Button);
            Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            Slot->SetPadding(FMargin(2, 0, 2, 0));
        };

        auto* FPS60Button = MakeButton(TEXT("60"), Limit == 60);
        FPS60Button->OnClicked.AddDynamic(this, &UPlayerMenuWidget::FPS60);
        AddFPSPresetSlot(FPS60Button);

        auto* FPS120Button = MakeButton(TEXT("120"), Limit == 120);
        FPS120Button->OnClicked.AddDynamic(this, &UPlayerMenuWidget::FPS120);
        AddFPSPresetSlot(FPS120Button);

        auto* FPS144Button = MakeButton(TEXT("144"), Limit == 144);
        FPS144Button->OnClicked.AddDynamic(this, &UPlayerMenuWidget::FPS144);
        AddFPSPresetSlot(FPS144Button);

        auto* FPS165Button = MakeButton(TEXT("165"), Limit == 165);
        FPS165Button->OnClicked.AddDynamic(this, &UPlayerMenuWidget::FPS165);
        AddFPSPresetSlot(FPS165Button);

        auto* FPSUnlimitedButton = MakeButton(TEXT("∞"), Limit == 0);
        FPSUnlimitedButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::FPSUnlimited);
        AddFPSPresetSlot(FPSUnlimitedButton);

        auto* FPSButton = MakeButton(
            FString::Printf(TEXT("LICZNIK FPS  •  %s"), bFPSVisible ? TEXT("WŁ.") : TEXT("WYŁ.")),
            bFPSVisible);
        FPSButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::ToggleFPSCounter);
        AddCenterButton(FPSButton);

        RightColumn->AddChildToVerticalBox(MakeText(TEXT("METRYKI RENDERU"), 9, true, Accent))
            ->SetPadding(FMargin(0, 0, 0, 8));
        RightColumn->AddChildToVerticalBox(MakeInfoRow(
            FString::Printf(TEXT("%.0f%%"), ScaleValue), TEXT("SKALA RENDERU"), true))
            ->SetPadding(FMargin(0, 0, 0, 7));
        RightColumn->AddChildToVerticalBox(MakeInfoRow(
            LimitLabel, TEXT("LIMIT KLATEK")))->SetPadding(FMargin(0, 0, 0, 7));
        RightColumn->AddChildToVerticalBox(MakeInfoRow(
            bVSync ? TEXT("WŁĄCZONY") : TEXT("WYŁĄCZONY"), TEXT("VSYNC")))
            ->SetPadding(FMargin(0, 0, 0, 7));
        RightColumn->AddChildToVerticalBox(MakeInfoRow(
            bDynamicResolution ? TEXT("WŁĄCZONA") : TEXT("WYŁĄCZONA"), TEXT("DYNAMICZNA ROZDZIELCZOŚĆ")))
            ->SetPadding(FMargin(0, 0, 0, 7));
        RightColumn->AddChildToVerticalBox(MakeText(
            FString::Printf(TEXT("Zakres skali renderu: %.0f–%.0f%%"), MinScale, MaxScale),
            10, false, Muted))->SetPadding(FMargin(2, 8, 2, 0));
        return;
    }

    if (SettingsSection == 2)
    {
        PageTitle->SetText(FText::FromString(TEXT("USTAWIENIA  /  INTERFEJS")));
        CenterColumn->AddChildToVerticalBox(MakeText(TEXT("INTERFEJS"), 22, true, TextPrimary))
            ->SetPadding(FMargin(18, 16, 18, 3));
        CenterColumn->AddChildToVerticalBox(MakeText(
            TEXT("ANIMACJE, TŁO I DŹWIĘKI MENU"), 9, true, Accent))
            ->SetPadding(FMargin(18, 0, 18, 18));

        auto* MotionButton = MakeButton(
            FString::Printf(TEXT("ANIMACJE UI  •  %s"), bReduceMotion ? TEXT("OGRANICZONE") : TEXT("PEŁNE")),
            bReduceMotion);
        MotionButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::ToggleReduceUIMotion);
        AddCenterButton(MotionButton);

        auto* BlurButton = MakeButton(
            FString::Printf(TEXT("ROZMYCIE TŁA  •  %s"), bMenuBlur ? TEXT("WŁ.") : TEXT("WYŁ.")),
            bMenuBlur);
        BlurButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::ToggleMenuBackgroundBlur);
        AddCenterButton(BlurButton);

        auto* UISoundButton = MakeButton(
            FString::Printf(TEXT("DŹWIĘKI UI  •  %s"), bUISounds ? TEXT("WŁ.") : TEXT("WYŁ.")),
            bUISounds);
        UISoundButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::ToggleUISounds);
        AddCenterButton(UISoundButton);

        RightColumn->AddChildToVerticalBox(MakeText(TEXT("DOSTĘPNOŚĆ UI"), 9, true, Accent))
            ->SetPadding(FMargin(0, 0, 0, 8));
        RightColumn->AddChildToVerticalBox(MakeInfoRow(
            bReduceMotion ? TEXT("OGRANICZONE") : TEXT("PEŁNE"), TEXT("ANIMACJE"), bReduceMotion))
            ->SetPadding(FMargin(0, 0, 0, 7));
        RightColumn->AddChildToVerticalBox(MakeInfoRow(
            bMenuBlur ? TEXT("WŁĄCZONE") : TEXT("WYŁĄCZONE"), TEXT("ROZMYCIE TŁA")))
            ->SetPadding(FMargin(0, 0, 0, 7));
        RightColumn->AddChildToVerticalBox(MakeInfoRow(
            bUISounds ? TEXT("WŁĄCZONE") : TEXT("WYŁĄCZONE"), TEXT("DŹWIĘKI UI")))
            ->SetPadding(FMargin(0, 0, 0, 14));
        RightColumn->AddChildToVerticalBox(MakeText(
            TEXT("Opcja ograniczenia animacji wyłącza ruchome przejścia i ambientowe animacje menu."),
            10, false, Muted));
        return;
    }

    SettingsSection = 3;
    PageTitle->SetText(FText::FromString(TEXT("USTAWIENIA  /  DŹWIĘK")));
    CenterColumn->AddChildToVerticalBox(MakeText(TEXT("DŹWIĘK"), 22, true, TextPrimary))
        ->SetPadding(FMargin(18, 16, 18, 3));
    CenterColumn->AddChildToVerticalBox(MakeText(
        TEXT("GŁOŚNOŚĆ EFEKTÓW ŚWIATA I INTERFEJSU"), 9, true, Accent))
        ->SetPadding(FMargin(18, 0, 18, 18));

    SFXVolumeButton = MakeButton(FString::Printf(
        TEXT("EFEKTY ŚWIATA  •  %d%%"), FMath::RoundToInt(SFXVolume * 100.0f)), true);
    auto* SFXButton = SFXVolumeButton.Get();
    SFXButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::CycleSFXVolume);
    AddCenterButton(SFXButton, 5.0f);

    SFXVolumeMeter = WidgetTree->ConstructWidget<UProgressBar>();
    auto* SFXMeter = SFXVolumeMeter.Get();
    SFXMeter->SetPercent(SFXVolume);
    SFXMeter->SetFillColorAndOpacity(Accent);
    CenterColumn->AddChildToVerticalBox(SFXMeter)->SetPadding(FMargin(18, 0, 18, 5));

    auto* SFXSlider = WidgetTree->ConstructWidget<USlider>();
    SFXSlider->SetValue(SFXVolume);
    SFXSlider->SetStepSize(0.05f);
    SFXSlider->OnValueChanged.AddDynamic(this, &UPlayerMenuWidget::SetSFXVolumeFromSlider);
    SFXSlider->OnMouseCaptureEnd.AddDynamic(this, &UPlayerMenuWidget::CommitAudioSliderChange);
    SFXSlider->OnControllerCaptureEnd.AddDynamic(this, &UPlayerMenuWidget::CommitAudioSliderChange);
    CenterColumn->AddChildToVerticalBox(SFXSlider)->SetPadding(FMargin(18, 0, 18, 13));

    UIVolumeButton = MakeButton(FString::Printf(
        TEXT("INTERFEJS  •  %d%%"), FMath::RoundToInt(UIVolume * 100.0f)));
    auto* UILevelButton = UIVolumeButton.Get();
    UILevelButton->OnClicked.AddDynamic(this, &UPlayerMenuWidget::CycleUIVolume);
    AddCenterButton(UILevelButton, 5.0f);

    UIVolumeMeter = WidgetTree->ConstructWidget<UProgressBar>();
    auto* UIMeter = UIVolumeMeter.Get();
    UIMeter->SetPercent(UIVolume);
    UIMeter->SetFillColorAndOpacity(Accent);
    CenterColumn->AddChildToVerticalBox(UIMeter)->SetPadding(FMargin(18, 0, 18, 5));

    auto* UISlider = WidgetTree->ConstructWidget<USlider>();
    UISlider->SetValue(UIVolume);
    UISlider->SetStepSize(0.05f);
    UISlider->OnValueChanged.AddDynamic(this, &UPlayerMenuWidget::SetUIVolumeFromSlider);
    UISlider->OnMouseCaptureEnd.AddDynamic(this, &UPlayerMenuWidget::CommitAudioSliderChange);
    UISlider->OnControllerCaptureEnd.AddDynamic(this, &UPlayerMenuWidget::CommitAudioSliderChange);
    CenterColumn->AddChildToVerticalBox(UISlider)->SetPadding(FMargin(18, 0, 18, 10));

    CenterColumn->AddChildToVerticalBox(MakeText(
        TEXT("SUWAK: PRECYZYJNA REGULACJA  •  PRZYCISK: SKOK CO 25%"), 9, true, Muted))
        ->SetPadding(FMargin(18, 0, 18, 8));

    RightColumn->AddChildToVerticalBox(MakeText(TEXT("POZIOMY GŁOŚNOŚCI"), 9, true, Accent))
        ->SetPadding(FMargin(0, 0, 0, 8));
    RightColumn->AddChildToVerticalBox(MakeInfoRow(
        FString::Printf(TEXT("%d%%"), FMath::RoundToInt(SFXVolume * 100.0f)),
        TEXT("EFEKTY ŚWIATA"), true))
        ->SetPadding(FMargin(0, 0, 0, 7));
    RightColumn->AddChildToVerticalBox(MakeInfoRow(
        FString::Printf(TEXT("%d%%"), FMath::RoundToInt(UIVolume * 100.0f)),
        TEXT("INTERFEJS")))
        ->SetPadding(FMargin(0, 0, 0, 14));
    RightColumn->AddChildToVerticalBox(MakeInfoRow(
        TEXT("WSPÓLNY MIKS"),
        TEXT("MUZYKA")))
        ->SetPadding(FMargin(0, 0, 0, 8));
    RightColumn->AddChildToVerticalBox(MakeText(
        TEXT("Muzyka korzysta obecnie ze wspólnego miksu gry; osobna regulacja pojawi się jako oddzielna kontrolka."),
        10, false, Muted));
}

void UPlayerMenuWidget::TabGame() { SelectTab(0); }
void UPlayerMenuWidget::TabCharacter() { SelectTab(1); }
void UPlayerMenuWidget::TabInventory() { SelectTab(2); }
void UPlayerMenuWidget::TabJournal() { SelectTab(3); }
void UPlayerMenuWidget::TabMap() { SelectTab(4); }
void UPlayerMenuWidget::TabStats() { SelectTab(5); }
void UPlayerMenuWidget::TabSettings() { SelectTab(6); }
void UPlayerMenuWidget::SettingsDisplay() { SettingsSection = 0; Refresh(); }
void UPlayerMenuWidget::SettingsPerformance() { SettingsSection = 1; Refresh(); }
void UPlayerMenuWidget::SettingsInterface() { SettingsSection = 2; Refresh(); }
void UPlayerMenuWidget::SettingsAudio() { SettingsSection = 3; Refresh(); }

void UPlayerMenuWidget::CycleSFXVolume()
{
    if (auto* Settings = UWTGAudioSettings::Get())
        Settings->SetSFXVolume(NextAudioVolume(Settings->SFXVolume));
    RefreshWithSettingsToast();
}

void UPlayerMenuWidget::CycleUIVolume()
{
    if (auto* Settings = UWTGAudioSettings::Get())
        Settings->SetUIVolume(NextAudioVolume(Settings->UIVolume));
    RefreshWithSettingsToast();
}

void UPlayerMenuWidget::ResetSettings()
{
    ShowConfirmation(
        5,
        TEXT("PRZYWRÓCIĆ USTAWIENIA DOMYŚLNE?"),
        TEXT("Zostaną przywrócone domyślne ustawienia jakości, skali renderu, VSync, dynamicznej rozdzielczości, limitu FPS, interfejsu i dźwięku. Tryb ekranu i rozdzielczość pozostaną bez zmian."),
        TEXT("PRZYWRÓĆ"));
}

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
    auto* Mission = GetGameInstance()->GetSubsystem<USliceMission>();
    if (Mission && Mission->bInGame)
    {
        ShowConfirmation(
            4,
            TEXT("WCZYTAĆ OSTATNI ZAPIS?"),
            TEXT("Bieżący stan sesji zostanie zastąpiony danymi z ostatniego zapisu. Niezapisane zmiany zostaną utracone."),
            TEXT("WCZYTAJ"));
        return;
    }

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
    const auto* Settings = UWTGPerformanceSettings::Get();
    if (!Settings || Settings->bUISounds)
        USliceAudio::PlayUI(this, TEXT("UIHover"), 0.16f);
}

void UPlayerMenuWidget::PlayUIClick()
{
    const auto* Settings = UWTGPerformanceSettings::Get();
    if (!Settings || Settings->bUISounds)
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
    ConfirmationSecondsTotal = Action == 3 ? 15.0f : 0.0f;
    ConfirmationSecondsRemaining = ConfirmationSecondsTotal;
    ConfirmationAnimationTime = 0.0f;

    ConfirmationOverlay = WidgetTree->ConstructWidget<UBorder>();
    ConfirmationOverlay->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.78f));
    auto* OverlaySlot = RootOverlay->AddChildToOverlay(ConfirmationOverlay);
    OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
    OverlaySlot->SetVerticalAlignment(VAlign_Fill);

    auto* ModalBlur = WidgetTree->ConstructWidget<UBackgroundBlur>();
    ModalBlur->SetBlurStrength(8.0f);
    ModalBlur->SetBlurRadius(12);
    ConfirmationOverlay->AddChild(ModalBlur);

    auto* Center = WidgetTree->ConstructWidget<UOverlay>();
    ModalBlur->AddChild(Center);

    auto* CardSize = WidgetTree->ConstructWidget<USizeBox>();
    CardSize->SetWidthOverride(560.0f);
    auto* CardSlot = Center->AddChildToOverlay(CardSize);
    CardSlot->SetHorizontalAlignment(HAlign_Center);
    CardSlot->SetVerticalAlignment(VAlign_Center);

    ConfirmationCard = MakeCard(FMargin(28, 26, 28, 24));
    CardSize->AddChild(ConfirmationCard);
    const auto* UISettings = UWTGPerformanceSettings::Get();
    const bool bReduceMotion = UISettings && UISettings->bReduceUIMotion;
    ConfirmationAnimationTime = bReduceMotion ? 0.18f : 0.0f;
    ConfirmationCard->SetRenderOpacity(bReduceMotion ? 1.0f : 0.0f);
    ConfirmationCard->SetRenderTranslation(
        bReduceMotion ? FVector2D::ZeroVector : FVector2D(0.0f, 18.0f));

    auto* Column = WidgetTree->ConstructWidget<UVerticalBox>();
    ConfirmationCard->AddChild(Column);

    const bool bDestructive = Action == 1 || Action == 2 || Action == 4;
    const FString ConfirmationCategory = bDestructive
        ? TEXT("OSTRZEŻENIE")
        : (Action == 5 ? TEXT("USTAWIENIA") : TEXT("USTAWIENIA WIDEO"));
    Column->AddChildToVerticalBox(MakeText(
        ConfirmationCategory,
        9, true, bDestructive ? Danger : Accent))
        ->SetPadding(FMargin(0, 0, 0, 5));
    Column->AddChildToVerticalBox(MakeText(Title, 24, true, TextPrimary))
        ->SetPadding(FMargin(0, 0, 0, 12));

    auto* Description = MakeText(Body, 12, false, Muted);
    Description->SetLineHeightPercentage(1.25f);
    Column->AddChildToVerticalBox(Description)->SetPadding(FMargin(0, 0, 0, 16));

    if (bDestructive)
    {
        auto* Mission = GetGameInstance()->GetSubsystem<USliceMission>();
        const bool bSaveOK = Mission && Mission->bLastSaveSucceeded;
        auto* Warning = WidgetTree->ConstructWidget<UBorder>();
        Warning->SetBrush(RoundedBrush(
            bSaveOK ? FLinearColor(0.035f, 0.105f, 0.105f, 0.94f)
                    : FLinearColor(0.18f, 0.045f, 0.040f, 0.96f),
            9.0f));
        Warning->SetPadding(FMargin(12, 9));
        Warning->AddChild(MakeText(
            bSaveOK ? TEXT("OSTATNI ZAPIS: POPRAWNY")
                    : TEXT("OSTATNI ZAPIS: BRAK POTWIERDZENIA POPRAWNEGO ZAPISU"),
            9, true, bSaveOK ? Accent : DangerHover));
        Column->AddChildToVerticalBox(Warning)->SetPadding(FMargin(0, 0, 0, 16));
    }

    if (Action == 3)
    {
        ConfirmationCountdown = MakeText(TEXT("AUTOMATYCZNE COFNIĘCIE ZA 15 S"), 10, true, Accent);
        Column->AddChildToVerticalBox(ConfirmationCountdown)->SetPadding(FMargin(0, 0, 0, 7));

        ConfirmationProgress = WidgetTree->ConstructWidget<UProgressBar>();
        ConfirmationProgress->SetPercent(1.0f);
        ConfirmationProgress->SetFillColorAndOpacity(Accent);
        Column->AddChildToVerticalBox(ConfirmationProgress)->SetPadding(FMargin(0, 0, 0, 15));
    }

    auto* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>();
    Column->AddChildToVerticalBox(Buttons);

    auto* Cancel = MakeButton(Action == 3 ? TEXT("COFNIJ") : TEXT("ANULUJ"));
    Cancel->OnClicked.AddDynamic(this, &UPlayerMenuWidget::CancelConfirmation);
    auto* CancelSlot = Buttons->AddChildToHorizontalBox(Cancel);
    CancelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CancelSlot->SetPadding(FMargin(0, 0, 5, 0));

    auto* Confirm = MakeButton(ConfirmLabel, true);
    if (bDestructive)
    {
        FButtonStyle DangerStyle = Confirm->GetStyle();
        DangerStyle.Normal = RoundedBrush(Danger, 9.0f);
        DangerStyle.Hovered = RoundedBrush(DangerHover, 9.0f);
        DangerStyle.Pressed = RoundedBrush(DangerPressed, 9.0f);
        Confirm->SetStyle(DangerStyle);
        if (auto* ConfirmText = Cast<UTextBlock>(Confirm->GetContent()))
            ConfirmText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    }
    Confirm->OnClicked.AddDynamic(this, &UPlayerMenuWidget::ConfirmPendingAction);
    auto* ConfirmSlot = Buttons->AddChildToHorizontalBox(Confirm);
    ConfirmSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ConfirmSlot->SetPadding(FMargin(5, 0, 0, 0));

    auto* ModalShortcuts = WidgetTree->ConstructWidget<UHorizontalBox>();
    ModalShortcuts->AddChildToHorizontalBox(MakeKeycap(TEXT("ENTER / A")))
        ->SetPadding(FMargin(0, 0, 6, 0));
    ModalShortcuts->AddChildToHorizontalBox(MakeText(TEXT("POTWIERDŹ"), 8, true, Muted))
        ->SetPadding(FMargin(0, 3, 14, 0));
    ModalShortcuts->AddChildToHorizontalBox(MakeKeycap(TEXT("ESC / B")))
        ->SetPadding(FMargin(0, 0, 6, 0));
    ModalShortcuts->AddChildToHorizontalBox(MakeText(TEXT("ANULUJ"), 8, true, Muted))
        ->SetPadding(FMargin(0, 3, 0, 0));
    auto* ModalShortcutsSlot = Column->AddChildToVerticalBox(ModalShortcuts);
    ModalShortcutsSlot->SetHorizontalAlignment(HAlign_Center);
    ModalShortcutsSlot->SetPadding(FMargin(0, 12, 0, 0));

    UButton* DefaultFocus = bDestructive ? Cancel : Confirm;
    if (APlayerController* PlayerController = GetOwningPlayer())
        DefaultFocus->SetUserFocus(PlayerController);
    else
        DefaultFocus->SetKeyboardFocus();
}

void UPlayerMenuWidget::ClearConfirmation()
{
    if (ConfirmationOverlay)
        ConfirmationOverlay->RemoveFromParent();
    ConfirmationOverlay = nullptr;
    ConfirmationCard = nullptr;
    ConfirmationCountdown = nullptr;
    ConfirmationProgress = nullptr;
    PendingConfirmation = 0;
    ConfirmationSecondsTotal = 0.0f;
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
        ShowToast(TEXT("USTAWIENIA OBRAZU ZAPISANE"));
        return;
    }

    if (Action == 5)
    {
        ClearConfirmation();

        if (auto* Preferences = UWTGPerformanceSettings::Get())
            Preferences->ResetToDefaults();
        if (auto* AudioPreferences = UWTGAudioSettings::Get())
            AudioPreferences->ResetToDefaults();

        if (GEngine)
        {
            if (UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
            {
                UserSettings->SetOverallScalabilityLevel(3);
                UserSettings->SetResolutionScaleValueEx(100.0f);
                UserSettings->SetVSyncEnabled(false);
                UserSettings->SetDynamicResolutionEnabled(false);
                UserSettings->ApplySettings(false);
                UserSettings->SaveSettings();
            }
        }

        if (MenuBackgroundBlur)
        {
            MenuBackgroundBlur->SetBlurStrength(12.0f);
            MenuBackgroundBlur->SetBlurRadius(18);
        }

        Refresh();
        ShowToast(TEXT("USTAWIENIA DOMYŚLNE PRZYWRÓCONE"));
        return;
    }

    ClearConfirmation();

    if (auto* Controller = Cast<ASliceController>(GetOwningPlayer()))
    {
        if (Action == 1)
            Controller->NewGame();
        else if (Action == 2)
            Controller->Quit();
        else if (Action == 4)
            Controller->LoadGame();
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

void UPlayerMenuWidget::ToggleReduceUIMotion()
{
    if (auto* Settings = UWTGPerformanceSettings::Get())
        Settings->SetReduceUIMotion(!Settings->bReduceUIMotion);
    RefreshWithSettingsToast();
}

void UPlayerMenuWidget::ToggleMenuBackgroundBlur()
{
    if (auto* Settings = UWTGPerformanceSettings::Get())
    {
        Settings->SetMenuBackgroundBlur(!Settings->bMenuBackgroundBlur);
        if (MenuBackgroundBlur)
        {
            MenuBackgroundBlur->SetBlurStrength(Settings->bMenuBackgroundBlur ? 12.0f : 0.0f);
            MenuBackgroundBlur->SetBlurRadius(Settings->bMenuBackgroundBlur ? 18 : 0);
        }
    }
    RefreshWithSettingsToast();
}

void UPlayerMenuWidget::ToggleUISounds()
{
    if (auto* Settings = UWTGPerformanceSettings::Get())
        Settings->SetUISounds(!Settings->bUISounds);
    RefreshWithSettingsToast();
}

void UPlayerMenuWidget::SetSFXVolumeFromSlider(float Volume)
{
    if (auto* Settings = UWTGAudioSettings::Get())
        Settings->SetSFXVolume(Volume);

    if (SFXVolumeMeter)
        SFXVolumeMeter->SetPercent(Volume);
    if (SFXVolumeButton)
        if (auto* Label = Cast<UTextBlock>(SFXVolumeButton->GetContent()))
            Label->SetText(FText::FromString(FString::Printf(
                TEXT("EFEKTY ŚWIATA  •  %d%%"), FMath::RoundToInt(Volume * 100.0f))));
}

void UPlayerMenuWidget::SetUIVolumeFromSlider(float Volume)
{
    if (auto* Settings = UWTGAudioSettings::Get())
        Settings->SetUIVolume(Volume);

    if (UIVolumeMeter)
        UIVolumeMeter->SetPercent(Volume);
    if (UIVolumeButton)
        if (auto* Label = Cast<UTextBlock>(UIVolumeButton->GetContent()))
            Label->SetText(FText::FromString(FString::Printf(
                TEXT("INTERFEJS  •  %d%%"), FMath::RoundToInt(Volume * 100.0f))));
}

void UPlayerMenuWidget::CommitAudioSliderChange()
{
    RefreshWithSettingsToast();
}

void UPlayerMenuWidget::ToggleFPSCounter()
{
    auto* Preferences = UWTGPerformanceSettings::Get();
    if (!Preferences)
        return;
    Preferences->SetShowFPS(!Preferences->bShowFPS);
    RefreshWithSettingsToast();
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
    RefreshWithSettingsToast();
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
    RefreshWithSettingsToast();
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
    RefreshWithSettingsToast();
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
    RefreshWithSettingsToast();
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
    RefreshWithSettingsToast();
}

void UPlayerMenuWidget::SetFPSLimit(int32 Limit)
{
    if (auto* Preferences = UWTGPerformanceSettings::Get())
        Preferences->SetFPSLimit(Limit);
    RefreshWithSettingsToast();
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
    PreviewViewLabel = TEXT("CAŁA SYLWETKA");
    UpdatePreviewStatus();
    UpdatePreviewControlStyles();
}

void UPlayerMenuWidget::PreviewUpperBody()
{
    if (Studio) Studio->SetView(TEXT("UpperBody"));
    PreviewViewLabel = TEXT("GÓRNA CZĘŚĆ");
    UpdatePreviewStatus();
    UpdatePreviewControlStyles();
}

void UPlayerMenuWidget::PreviewFace()
{
    if (Studio) Studio->SetView(TEXT("Face"));
    PreviewViewLabel = TEXT("TWARZ");
    UpdatePreviewStatus();
    UpdatePreviewControlStyles();
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
    PreviewLightingLabel = TEXT("STUDIO");
    UpdatePreviewStatus();
    UpdatePreviewControlStyles();
}

void UPlayerMenuWidget::PreviewLightingDaylight()
{
    if (Studio) Studio->SetLighting(TEXT("Daylight"));
    PreviewLightingLabel = TEXT("DZIEŃ");
    UpdatePreviewStatus();
    UpdatePreviewControlStyles();
}

void UPlayerMenuWidget::PreviewLightingNight()
{
    if (Studio) Studio->SetLighting(TEXT("Night"));
    PreviewLightingLabel = TEXT("NOC");
    UpdatePreviewStatus();
    UpdatePreviewControlStyles();
}

void UPlayerMenuWidget::PreviewReset()
{
    if (Studio) Studio->ResetPresentation();
    PreviewViewLabel = TEXT("CAŁA SYLWETKA");
    PreviewLightingLabel = TEXT("STUDIO");
    UpdatePreviewStatus();
    UpdatePreviewControlStyles();
}

void UPlayerMenuWidget::UpdatePreviewStatus()
{
    if (PreviewViewStatus)
        PreviewViewStatus->SetText(FText::FromString(
            FString::Printf(TEXT("KADR  •  %s"), *PreviewViewLabel)));
    if (PreviewLightingStatus)
        PreviewLightingStatus->SetText(FText::FromString(
            FString::Printf(TEXT("ŚWIATŁO  •  %s"), *PreviewLightingLabel)));
}

void UPlayerMenuWidget::UpdatePreviewControlStyles()
{
    auto ApplySelectedStyle = [&](UButton* Button, bool bSelected)
    {
        if (!Button)
            return;

        FButtonStyle Style = Button->GetStyle();
        Style.Normal = RoundedBrush(bSelected ? Accent : PanelSoft, 9.0f);
        Style.Hovered = RoundedBrush(bSelected ? AccentHover : PanelHover, 9.0f);
        Style.Pressed = RoundedBrush(bSelected ? AccentPressed : Panel, 9.0f);
        Button->SetStyle(Style);

        if (auto* Text = Cast<UTextBlock>(Button->GetContent()))
            Text->SetColorAndOpacity(FSlateColor(bSelected ? Background : TextPrimary));
    };

    const FString ViewLabels[] = {TEXT("CAŁA SYLWETKA"), TEXT("GÓRNA CZĘŚĆ"), TEXT("TWARZ")};
    for (int32 Index = 0; Index < PreviewViewButtons.Num() && Index < UE_ARRAY_COUNT(ViewLabels); ++Index)
        ApplySelectedStyle(PreviewViewButtons[Index], PreviewViewLabel == ViewLabels[Index]);

    const FString LightingLabels[] = {TEXT("STUDIO"), TEXT("DZIEŃ"), TEXT("NOC")};
    for (int32 Index = 0; Index < PreviewLightingButtons.Num() && Index < UE_ARRAY_COUNT(LightingLabels); ++Index)
        ApplySelectedStyle(PreviewLightingButtons[Index], PreviewLightingLabel == LightingLabels[Index]);
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

    if (ActiveTab == 4)
    {
        auto* CityGameplay = GetWorld()->GetSubsystem<UCityGameplaySubsystem>();
        auto* MapSubsystem = GetWorld()->GetSubsystem<UWroclawMapSubsystem>();
        if (CityGameplay && CityGameplay->IsActive() && MapSubsystem)
        {
            if (Key == EKeys::C)
            {
                MapSubsystem->CycleWaypoint();
                Refresh();
                return FReply::Handled();
            }
            if (Key == EKeys::BackSpace)
            {
                MapSubsystem->ClearWaypoint();
                Refresh();
                return FReply::Handled();
            }
        }
    }

    const FKey DirectTabKeys[] = {
        EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four,
        EKeys::Five, EKeys::Six, EKeys::Seven
    };
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(DirectTabKeys); ++Index)
    {
        if (Key == DirectTabKeys[Index])
        {
            SelectTab(Index);
            return FReply::Handled();
        }
    }

    if (Key == EKeys::Home)
    {
        SelectTab(0);
        return FReply::Handled();
    }
    if (Key == EKeys::End)
    {
        SelectTab(TabButtons.Num() - 1);
        return FReply::Handled();
    }

    if (Key == EKeys::Q || Key == EKeys::Gamepad_LeftShoulder || Key == EKeys::Gamepad_DPad_Left)
    {
        SelectTab((ActiveTab + TabButtons.Num() - 1) % TabButtons.Num());
        return FReply::Handled();
    }

    if (Key == EKeys::E || Key == EKeys::Gamepad_RightShoulder || Key == EKeys::Gamepad_DPad_Right)
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

    const auto* UISettings = UWTGPerformanceSettings::Get();
    const bool bReduceMotion = UISettings && UISettings->bReduceUIMotion;

    UpdateFocusPresentation();

    ClockRefreshAccumulator += InDeltaTime;
    if (ClockText && ClockRefreshAccumulator >= 1.0f)
    {
        ClockRefreshAccumulator = 0.0f;
        ClockText->SetText(FText::FromString(FDateTime::Now().ToString(TEXT("%d.%m.%Y  •  %H:%M"))));
    }

    if (!bReduceMotion)
        AmbientAnimationTime += InDeltaTime;

    if (!bReduceMotion && ShellLayout && ShellAnimationTime < 0.28f)
    {
        ShellAnimationTime = FMath::Min(0.28f, ShellAnimationTime + InDeltaTime);
        const float T = FMath::Clamp(ShellAnimationTime / 0.28f, 0.0f, 1.0f);
        const float Ease = 1.0f - FMath::Pow(1.0f - T, 3.0f);
        ShellLayout->SetRenderOpacity(Ease);
        ShellLayout->SetRenderTranslation(FVector2D(0.0f, FMath::Lerp(10.0f, 0.0f, Ease)));
        const float Scale = FMath::Lerp(0.985f, 1.0f, Ease);
        ShellLayout->SetRenderScale(FVector2D(Scale, Scale));
    }

    if (!bReduceMotion && ConfirmationCard && ConfirmationAnimationTime < 0.18f)
    {
        ConfirmationAnimationTime = FMath::Min(0.18f, ConfirmationAnimationTime + InDeltaTime);
        const float T = FMath::Clamp(ConfirmationAnimationTime / 0.18f, 0.0f, 1.0f);
        const float Ease = 1.0f - FMath::Pow(1.0f - T, 3.0f);
        ConfirmationCard->SetRenderOpacity(Ease);
        ConfirmationCard->SetRenderTranslation(FVector2D(0.0f, FMath::Lerp(18.0f, 0.0f, Ease)));
    }
    if (!bReduceMotion && AmbientGlowA)
    {
        AmbientGlowA->SetRenderTranslation(FVector2D(
            FMath::Sin(AmbientAnimationTime * 0.22f) * 18.0f,
            FMath::Cos(AmbientAnimationTime * 0.17f) * 12.0f));
        AmbientGlowA->SetRenderOpacity(0.78f + FMath::Sin(AmbientAnimationTime * 0.31f) * 0.12f);
    }
    if (!bReduceMotion && AmbientGlowB)
    {
        AmbientGlowB->SetRenderTranslation(FVector2D(
            FMath::Cos(AmbientAnimationTime * 0.19f) * 14.0f,
            FMath::Sin(AmbientAnimationTime * 0.15f) * 10.0f));
        AmbientGlowB->SetRenderOpacity(0.72f + FMath::Cos(AmbientAnimationTime * 0.27f) * 0.10f);
    }

    if (!bReduceMotion && PageAnimationTime < 0.22f && ActionColumn && CenterColumn && RightColumn)
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

    if (ToastCard && ToastTimeRemaining > 0.0f)
    {
        ToastTimeRemaining = FMath::Max(0.0f, ToastTimeRemaining - InDeltaTime);

        if (!bReduceMotion && ToastAnimationTime < 0.18f)
        {
            ToastAnimationTime = FMath::Min(0.18f, ToastAnimationTime + InDeltaTime);
            const float T = FMath::Clamp(ToastAnimationTime / 0.18f, 0.0f, 1.0f);
            const float Ease = 1.0f - FMath::Pow(1.0f - T, 3.0f);
            ToastCard->SetRenderOpacity(Ease);
            ToastCard->SetRenderTranslation(FVector2D(FMath::Lerp(18.0f, 0.0f, Ease), 0.0f));
        }

        if (!bReduceMotion && ToastTimeRemaining < 0.30f)
            ToastCard->SetRenderOpacity(FMath::Clamp(ToastTimeRemaining / 0.30f, 0.0f, 1.0f));
        if (ToastTimeRemaining <= 0.0f)
        {
            ToastCard->RemoveFromParent();
            ToastCard = nullptr;
        }
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
    if (ConfirmationProgress && ConfirmationSecondsTotal > 0.0f)
    {
        ConfirmationProgress->SetPercent(FMath::Clamp(
            ConfirmationSecondsRemaining / ConfirmationSecondsTotal, 0.0f, 1.0f));
    }

    if (ConfirmationSecondsRemaining <= 0.0f)
        CancelConfirmation();
}

FReply UPlayerMenuWidget::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
    const bool bPreviewPanelHovered = CenterScroll &&
        CenterScroll->GetCachedGeometry().IsUnderLocation(Event.GetScreenSpacePosition());
    if ((ActiveTab == 0 || ActiveTab == 1) && Studio && bPreviewPanelHovered &&
        Event.GetEffectingButton() == EKeys::RightMouseButton)
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
    const bool bPreviewPanelHovered = CenterScroll &&
        CenterScroll->GetCachedGeometry().IsUnderLocation(Event.GetScreenSpacePosition());
    if ((ActiveTab == 0 || ActiveTab == 1) && Studio && bPreviewPanelHovered)
    {
        Studio->Zoom(-Event.GetWheelDelta() * 18.0f);
        return FReply::Handled();
    }
    return Super::NativeOnMouseWheel(Geometry, Event);
}
