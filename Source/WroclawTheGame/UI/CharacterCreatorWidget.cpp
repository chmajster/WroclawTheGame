#include "UI/CharacterCreatorWidget.h"
#include "Character/CharacterCreator.h"
#include "Character/CharacterCreatorSubsystem.h"
#include "Character/CharacterAppearanceComponent.h"
#include "UI/SliceController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/SpinBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/ScaleBox.h"
#include "Components/SafeZone.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
const FLinearColor Background(0.005f, 0.009f, 0.016f, 0.98f);
const FLinearColor Panel(0.020f, 0.030f, 0.044f, 0.97f);
const FLinearColor PanelSoft(0.034f, 0.049f, 0.069f, 0.95f);
const FLinearColor PanelHover(0.055f, 0.078f, 0.105f, 1.0f);
const FLinearColor Accent(0.18f, 0.79f, 0.96f, 1.0f);
const FLinearColor AccentHover(0.32f, 0.86f, 1.0f, 1.0f);
const FLinearColor AccentPressed(0.10f, 0.61f, 0.78f, 1.0f);
const FLinearColor TextPrimary(0.95f, 0.97f, 1.0f, 1.0f);
const FLinearColor Muted(0.57f, 0.64f, 0.72f, 1.0f);
const FLinearColor Divider(0.11f, 0.16f, 0.21f, 1.0f);

FSlateRoundedBoxBrush RoundedBrush(const FLinearColor& Color, float Radius)
{
    return FSlateRoundedBoxBrush(Color, Radius, FVector2f(64.0f, 64.0f));
}

UTextBlock* Label(UWidgetTree* Tree, const FString& Text, int32 Size = 16,
                  bool bBold = false, FLinearColor Color = TextPrimary)
{
    auto* T = Tree->ConstructWidget<UTextBlock>();
    T->SetText(FText::FromString(Text));
    auto Font = T->GetFont();
    Font.Size = Size;
    if (bBold)
        Font.TypefaceFontName = TEXT("Bold");
    T->SetFont(Font);
    T->SetColorAndOpacity(FSlateColor(Color));
    T->SetAutoWrapText(true);
    return T;
}

void StyleButton(UButton* Button, bool bAccent = false)
{
    FButtonStyle Style = Button->GetStyle();
    Style.Normal = RoundedBrush(bAccent ? Accent : PanelSoft, 9.0f);
    Style.Hovered = RoundedBrush(bAccent ? AccentHover : PanelHover, 9.0f);
    Style.Pressed = RoundedBrush(bAccent ? AccentPressed : Panel, 9.0f);
    Style.NormalPadding = FMargin(13.0f, 9.0f);
    Style.PressedPadding = FMargin(13.0f, 10.0f, 13.0f, 8.0f);
    Button->SetStyle(Style);
    Button->SetBackgroundColor(FLinearColor::White);
}

UBorder* Card(UWidgetTree* Tree, const FMargin& Padding = FMargin(16),
              FLinearColor Color = Panel, float Radius = 14.0f)
{
    auto* Border = Tree->ConstructWidget<UBorder>();
    Border->SetBrush(RoundedBrush(Color, Radius));
    Border->SetPadding(Padding);
    return Border;
}

const TCHAR* Categories[]={TEXT("POSTAĆ"),TEXT("TWARZ"),TEXT("OCZY"),TEXT("NOS"),TEXT("USTA"),TEXT("SZCZĘKA"),TEXT("USZY"),TEXT("SKÓRA"),TEXT("WŁOSY"),TEXT("ZAROST"),TEXT("CIAŁO"),TEXT("UBRANIA"),TEXT("GŁOS")};
const TCHAR* Slots[]={TEXT("Kurtka"),TEXT("Koszulka"),TEXT("Spodnie"),TEXT("Buty"),TEXT("Czapka"),TEXT("Akcesoria"),TEXT("Plecak")};
}
void UAppearanceControl::Button(const FString& Name)
{
    auto* B=WidgetTree->ConstructWidget<UButton>();
    WidgetTree->RootWidget=B;
    StyleButton(B, Name.Contains(TEXT("ROZPOCZNIJ")) || Name.Contains(TEXT("PODSUMOWANIE")));
    auto* Text=Label(WidgetTree,Name,12,true,TextPrimary);
    Text->SetJustification(ETextJustify::Center);
    B->AddChild(Text);
    B->OnClicked.AddDynamic(this,&UAppearanceControl::Click);
}
void UAppearanceControl::Number(const FString& Name,float V,float Min,float Max,float Step)
{
    auto* Box=WidgetTree->ConstructWidget<UVerticalBox>(); WidgetTree->RootWidget=Box; Box->AddChildToVerticalBox(Label(WidgetTree,Name,10,true,Muted))->SetPadding(FMargin(0,0,0,5));
    auto* S=WidgetTree->ConstructWidget<USpinBox>(); S->SetMinValue(Min); S->SetMaxValue(Max); S->SetMinSliderValue(Min); S->SetMaxSliderValue(Max); S->SetDelta(Step); S->SetValue(V); Box->AddChildToVerticalBox(S); S->OnValueChanged.AddDynamic(this,&UAppearanceControl::Changed);
}
void UAppearanceControl::Choice(const FString& Name,const TArray<FName>& Options,FName Value)
{
    Values=Options; auto* Box=WidgetTree->ConstructWidget<UVerticalBox>(); WidgetTree->RootWidget=Box; Box->AddChildToVerticalBox(Label(WidgetTree,Name));
    auto* S=WidgetTree->ConstructWidget<UComboBoxString>(); for (auto ID:Options) S->AddOption(ID.ToString()); S->SetSelectedOption(Value.ToString()); Box->AddChildToVerticalBox(S); S->OnSelectionChanged.AddDynamic(this,&UAppearanceControl::Selected);
}
void UAppearanceControl::TextEntry(const FString& Name,const FString& Value)
{
    auto* Box=WidgetTree->ConstructWidget<UVerticalBox>(); WidgetTree->RootWidget=Box; Box->AddChildToVerticalBox(Label(WidgetTree,Name));
    auto* T=WidgetTree->ConstructWidget<UEditableTextBox>(); T->SetText(FText::FromString(Value)); Box->AddChildToVerticalBox(T); T->OnTextCommitted.AddDynamic(this,&UAppearanceControl::TextChanged);
}
void UAppearanceControl::Click() { if (OwnerWidget) OwnerWidget->Action(Field); }
void UAppearanceControl::Changed(float V) { if (OwnerWidget) OwnerWidget->SetNumber(Field,V); }
void UAppearanceControl::Selected(FString V,ESelectInfo::Type Type) { if (OwnerWidget && Type!=ESelectInfo::Direct) OwnerWidget->SetChoice(Field,FName(*V)); }
void UAppearanceControl::TextChanged(const FText& V,ETextCommit::Type Type) { if (OwnerWidget) OwnerWidget->SetText(Field,V.ToString()); }
UAppearanceControl* UCharacterCreatorWidget::Row(UVerticalBox* Box,FName ID)
{
    auto* R=CreateWidget<UAppearanceControl>(GetOwningPlayer()); R->OwnerWidget=this; R->Field=ID;
    // Add after constructing the row's content (UMG caches its Slate tree on insertion).
    return R;
}
void UCharacterCreatorWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    Creator=GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>();
    SetIsFocusable(true);

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* BP=LoadClass<AWTG_CharacterCreator>(
        nullptr,TEXT("/Game/CharacterCreator/BP_WTG_CharacterCreator.BP_WTG_CharacterCreator_C"));
    Studio=GetWorld()->SpawnActor<AWTG_CharacterCreator>(
        BP?BP:AWTG_CharacterCreator::StaticClass(),FVector(0,0,-50000),FRotator::ZeroRotator,Params);

    auto* Root=WidgetTree->ConstructWidget<UOverlay>();
    WidgetTree->RootWidget=Root;

    auto* Backdrop=WidgetTree->ConstructWidget<UBorder>();
    Backdrop->SetBrushColor(Background);
    auto* BackdropSlot=Root->AddChildToOverlay(Backdrop);
    BackdropSlot->SetHorizontalAlignment(HAlign_Fill);
    BackdropSlot->SetVerticalAlignment(VAlign_Fill);

    auto* TopAccent=WidgetTree->ConstructWidget<UBorder>();
    TopAccent->SetBrushColor(FLinearColor(Accent.R,Accent.G,Accent.B,0.72f));
    auto* TopAccentSize=WidgetTree->ConstructWidget<USizeBox>();
    TopAccentSize->SetHeightOverride(3.0f);
    TopAccentSize->AddChild(TopAccent);
    auto* TopAccentSlot=Root->AddChildToOverlay(TopAccentSize);
    TopAccentSlot->SetHorizontalAlignment(HAlign_Fill);
    TopAccentSlot->SetVerticalAlignment(VAlign_Top);

    auto* Safe=WidgetTree->ConstructWidget<USafeZone>();
    auto* SafeSlot=Root->AddChildToOverlay(Safe);
    SafeSlot->SetHorizontalAlignment(HAlign_Fill);
    SafeSlot->SetVerticalAlignment(VAlign_Fill);

    auto* Shell=WidgetTree->ConstructWidget<UVerticalBox>();
    Safe->AddChild(Shell);

    auto* HeaderCard=Card(WidgetTree,FMargin(20,15),Panel);
    Shell->AddChildToVerticalBox(HeaderCard)->SetPadding(FMargin(18,16,18,10));
    auto* HeaderRow=WidgetTree->ConstructWidget<UHorizontalBox>();
    HeaderCard->AddChild(HeaderRow);

    auto* HeaderText=WidgetTree->ConstructWidget<UVerticalBox>();
    auto* HeaderTextSlot=HeaderRow->AddChildToHorizontalBox(HeaderText);
    HeaderTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    HeaderText->AddChildToVerticalBox(Label(WidgetTree,TEXT("KREATOR POSTACI"),24,true,TextPrimary));
    HeaderText->AddChildToVerticalBox(Label(
        WidgetTree,TEXT("WYGLĄD  /  PROFIL  /  UBRANIA  /  GŁOS"),9,true,Accent))
        ->SetPadding(FMargin(0,3,0,0));

    auto* LiveBadge=WidgetTree->ConstructWidget<UBorder>();
    LiveBadge->SetBrush(RoundedBrush(FLinearColor(0.025f,0.105f,0.135f,0.98f),8.0f));
    LiveBadge->SetPadding(FMargin(12,7));
    LiveBadge->AddChild(Label(WidgetTree,TEXT("PODGLĄD NA ŻYWO"),9,true,Accent));
    HeaderRow->AddChildToHorizontalBox(LiveBadge)->SetPadding(FMargin(12,2,0,2));

    auto* Body=WidgetTree->ConstructWidget<UHorizontalBox>();
    auto* BodySlot=Shell->AddChildToVerticalBox(Body);
    BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    BodySlot->SetPadding(FMargin(18,0,18,10));

    auto* LeftSize=WidgetTree->ConstructWidget<USizeBox>();
    LeftSize->SetWidthOverride(360.0f);
    Body->AddChildToHorizontalBox(LeftSize);

    auto* LeftCard=Card(WidgetTree,FMargin(16,15),Panel);
    LeftSize->AddChild(LeftCard);
    auto* Left=WidgetTree->ConstructWidget<UVerticalBox>();
    LeftCard->AddChild(Left);
    Left->AddChildToVerticalBox(Label(WidgetTree,TEXT("PERSONALIZACJA"),9,true,Accent))
        ->SetPadding(FMargin(0,0,0,4));
    Heading=Label(WidgetTree,TEXT("KREATOR POSTACI"),20,true,TextPrimary);
    Left->AddChildToVerticalBox(Heading)->SetPadding(FMargin(0,0,0,12));
    auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>();
    Left->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Controls=WidgetTree->ConstructWidget<UVerticalBox>();
    Scroll->AddChild(Controls);

    auto* CenterCard=Card(WidgetTree,FMargin(14,14,14,12),Panel);
    auto* CenterSlot=Body->AddChildToHorizontalBox(CenterCard);
    FSlateChildSize CenterFill(ESlateSizeRule::Fill);
    CenterFill.Value=1.8f;
    CenterSlot->SetSize(CenterFill);
    CenterSlot->SetPadding(FMargin(12,0));

    auto* Center=WidgetTree->ConstructWidget<UVerticalBox>();
    CenterCard->AddChild(Center);
    auto* PreviewHead=WidgetTree->ConstructWidget<UHorizontalBox>();
    Center->AddChildToVerticalBox(PreviewHead)->SetPadding(FMargin(2,0,2,10));
    PreviewHead->AddChildToHorizontalBox(Label(WidgetTree,TEXT("PODGLĄD"),9,true,Accent))
        ->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* PreviewMode=Label(WidgetTree,TEXT("KADR  •  CAŁA SYLWETKA"),8,true,Muted);
    PreviewMode->SetJustification(ETextJustify::Right);
    PreviewHead->AddChildToHorizontalBox(PreviewMode);

    auto* PreviewStage=WidgetTree->ConstructWidget<UBorder>();
    PreviewStage->SetBrush(RoundedBrush(FLinearColor(0.008f,0.014f,0.024f,1.0f),12.0f));
    PreviewStage->SetPadding(FMargin(8));
    auto* StageSlot=Center->AddChildToVerticalBox(PreviewStage);
    StageSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    auto* PreviewScale=WidgetTree->ConstructWidget<UScaleBox>();
    PreviewScale->SetStretch(EStretch::ScaleToFit);
    PreviewScale->SetStretchDirection(EStretchDirection::Both);
    PreviewStage->AddChild(PreviewScale);

    auto* Image=WidgetTree->ConstructWidget<UImage>();
    if (Studio && Studio->RenderTarget)
    {
        if (auto* Material=LoadObject<UMaterialInterface>(
                nullptr,TEXT("/Game/CharacterCreator/M_CharacterPreview.M_CharacterPreview")))
        {
            auto* MID=UMaterialInstanceDynamic::Create(Material,Image);
            MID->SetTextureParameterValue(TEXT("PreviewTexture"),Studio->RenderTarget);
            Image->SetBrushFromMaterial(MID);
        }
        else
        {
            FSlateBrush Brush;
            Brush.SetResourceObject(Studio->RenderTarget);
            Brush.ImageSize=FVector2D(720,1000);
            Image->SetBrush(Brush);
        }
    }
    Image->SetColorAndOpacity(FLinearColor::White);
    PreviewScale->AddChild(Image);

    auto* HintCard=WidgetTree->ConstructWidget<UBorder>();
    HintCard->SetBrush(RoundedBrush(PanelSoft,9.0f));
    HintCard->SetPadding(FMargin(12,8));
    Center->AddChildToVerticalBox(HintCard)->SetPadding(FMargin(0,10,0,0));
    auto* Hint=Label(WidgetTree,
        TEXT("PPM  OBRÓT    •    KÓŁKO  ZOOM    •    WIDOK I ŚWIATŁO PO PRAWEJ"),
        9,true,Muted);
    Hint->SetJustification(ETextJustify::Center);
    HintCard->AddChild(Hint);

    auto* RightSize=WidgetTree->ConstructWidget<USizeBox>();
    RightSize->SetWidthOverride(340.0f);
    Body->AddChildToHorizontalBox(RightSize);
    auto* RightCard=Card(WidgetTree,FMargin(14,15),Panel);
    RightSize->AddChild(RightCard);
    auto* Right=WidgetTree->ConstructWidget<UVerticalBox>();
    RightCard->AddChild(Right);
    Right->AddChildToVerticalBox(Label(WidgetTree,TEXT("AKCJE"),9,true,Accent))
        ->SetPadding(FMargin(0,0,0,4));
    Right->AddChildToVerticalBox(Label(WidgetTree,TEXT("STEROWANIE I ZATWIERDZENIE"),15,true,TextPrimary))
        ->SetPadding(FMargin(0,0,0,12));
    auto* CommandScroll=WidgetTree->ConstructWidget<UScrollBox>();
    Right->AddChildToVerticalBox(CommandScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Commands=WidgetTree->ConstructWidget<UVerticalBox>();
    CommandScroll->AddChild(Commands);

    auto* FooterCard=Card(WidgetTree,FMargin(16,10),PanelSoft,10.0f);
    Shell->AddChildToVerticalBox(FooterCard)->SetPadding(FMargin(18,0,18,16));
    Status=Label(WidgetTree,TEXT(""),10,true,Muted);
    FooterCard->AddChild(Status);

    Refresh();
}

void UCharacterCreatorWidget::NativeDestruct()
{ if (Studio) Studio->Destroy(); Studio=nullptr; Super::NativeDestruct(); }
void UCharacterCreatorWidget::Refresh()
{
    Controls->ClearChildren(); Commands->ClearChildren(); const auto& A=Creator->Draft; const auto* C=Creator->Catalog.Get();
    auto Add=[&](UVerticalBox* Box,UAppearanceControl* R)
    {
        if (Box==Controls)
        {
            auto* RowCard=Card(WidgetTree,FMargin(10,8),PanelSoft,9.0f);
            RowCard->AddChild(R);
            Box->AddChildToVerticalBox(RowCard)->SetPadding(FMargin(0,0,0,7));
        }
        else
        {
            Box->AddChildToVerticalBox(R)->SetPadding(FMargin(0,0,0,7));
        }
    };
    auto Button=[&](const TCHAR* ID,const FString& Text){auto* R=Row(Commands,ID); R->Button(Text); Add(Commands,R);};
    auto Choice=[&](FName ID,const FString& Text,const TArray<FName>& Values,FName Value){auto* R=Row(Controls,ID); R->Choice(Text,Values,Value); Add(Controls,R);};
    auto Num=[&](FName ID,const FString& Text,float V,float Min,float Max,float Step=1.f){auto* R=Row(Controls,ID); R->Number(Text,V,Min,Max,Step); Add(Controls,R);};
    auto Part=[&](FName Field,const FString& Text,const TArray<FAppearancePartDefinition>& Parts,FName Value){TArray<FName> IDs; for (const auto& P:Parts) if (C->Compatible(P,A)) IDs.Add(P.ID); Choice(Field,Text,IDs,Value);};
    Heading->SetText(FText::FromString(Creator->bSummary?TEXT("TWOJA POSTAĆ"):TEXT("KREATOR POSTACI")));
    if (Creator->bSummary)
    {
        Controls->AddChildToVerticalBox(Label(WidgetTree,FString::Printf(TEXT("%s\n%.0f cm • wiek wizualny %.0f\n%s\nSeed: %d"),*A.Name,A.Height,A.VisualAge,*A.BodyBuild.ToString(),A.RandomSeed),18,true,TextPrimary))->SetPadding(FMargin(4,4,4,10));
        for (const auto& P:A.Clothing) Controls->AddChildToVerticalBox(Label(WidgetTree,P.Value.ToString()));
        Button(TEXT("Edit"),TEXT("EDYTUJ")); Button(TEXT("Start"),TEXT("ROZPOCZNIJ KAMPANIĘ"));
    }
    else
    {
        TArray<FName> Cats; for (auto Text:Categories) Cats.Add(FName(Text)); Choice(TEXT("Category"),TEXT("Kategoria"),Cats,FName(Categories[static_cast<int32>(Category)]));
        Button(TEXT("Mode"),Advanced?TEXT("TRYB: ZAAWANSOWANY"):TEXT("TRYB: PODSTAWOWY"));
        if (Category==EAppearanceCategory::Character)
        {
            TArray<FName> Presets; for (const auto& P:C->Presets) Presets.Add(P.PresetID); Choice(TEXT("Preset"),TEXT("Punkt wyjścia"),Presets,A.PresetID);
            Choice(TEXT("Sex"),TEXT("Baza postaci"),{TEXT("Male"),TEXT("Female")},A.Sex==EAppearanceSex::Male?TEXT("Male"):TEXT("Female"));
            auto* N=Row(Controls,TEXT("Name")); N->TextEntry(TEXT("Imię"),A.Name); Add(Controls,N);
            auto* S=Row(Controls,TEXT("Seed")); S->TextEntry(TEXT("Random seed (Enter: generuj)"),FString::FromInt(A.RandomSeed)); Add(Controls,S);
        }
        if (Category==EAppearanceCategory::Body) { Num(TEXT("Height"),TEXT("Wzrost (cm)"),A.Height,160,195); Choice(TEXT("Build"),TEXT("Sylwetka"),{TEXT("Slim"),TEXT("Average"),TEXT("Athletic"),TEXT("Heavy")},A.BodyBuild); }
        if (Category==EAppearanceCategory::Face) Choice(TEXT("FacePreset"),TEXT("Twarz"),C->FacePresets,A.FacePreset);
        if (Category==EAppearanceCategory::Eyes) { Num(TEXT("EyeColor"),TEXT("Naturalny kolor oczu"),A.EyeColor,0,C->EyePalette.Num()-1); Choice(TEXT("EyeShape"),TEXT("Kształt oczu"),C->EyeShapes,A.EyeShape); }
        if (Category==EAppearanceCategory::Skin) { Num(TEXT("SkinTone"),TEXT("Odcień skóry"),A.SkinTone,0,C->SkinPalette.Num()-1); Num(TEXT("Age"),TEXT("Wiek wizualny"),A.VisualAge,20,45); if (Advanced) { Num(TEXT("Roughness"),TEXT("Szorstkość skóry"),A.SkinRoughness,.3f,.8f,.01f); Num(TEXT("Freckles"),TEXT("Piegi"),A.Freckles,0,1,.01f); Num(TEXT("Imperfections"),TEXT("Detale skóry"),A.SkinImperfections,0,1,.01f); } }
        if (Category==EAppearanceCategory::Hair) { Part(TEXT("Hair"),TEXT("Fryzura"),C->HairStyleDefinitions,A.HairStyle); Num(TEXT("HairColor"),TEXT("Kolor włosów (0–7)"),A.HairColor,0,C->HairPalette.Num()-1); Part(TEXT("Brows"),TEXT("Brwi"),C->EyebrowStyleDefinitions,A.EyebrowStyle); Num(TEXT("BrowColor"),TEXT("Kolor brwi"),A.EyebrowColor,0,C->HairPalette.Num()-1); }
        if (Category==EAppearanceCategory::Beard) { Part(TEXT("Beard"),TEXT("Zarost"),C->BeardStyleDefinitions,A.BeardStyle); Num(TEXT("BeardColor"),TEXT("Kolor zarostu"),A.BeardColor,0,C->HairPalette.Num()-1); }
        if (Advanced) for (const auto& M:C->Morphs) if (M.Category==Category) Num(M.MorphID,M.DisplayName.ToString(),A.FaceMorphs.FindRef(M.MorphID),M.MinValue,M.MaxValue,.01f);
        if (Category==EAppearanceCategory::Clothing) for (int32 I=0; I<7; ++I)
        {
            auto S=static_cast<EClothingSlot>(I); TArray<FName> Options; if (I==0 || I>=4) Options.Add(NAME_None);
            for (const auto& P:C->ClothingDefinitions) if (P.Slot==S && C->Compatible(P,A)) Options.Add(P.ID);
            Choice(FName(*FString::Printf(TEXT("Slot%d"),I)),Slots[I],Options,A.Clothing.FindRef(S));
            if (const auto* P=C->Part(C->ClothingDefinitions,A.Clothing.FindRef(S))) if (P->ColorSupport && P->Variants.Num()) Num(FName(*FString::Printf(TEXT("Color%d"),I)),TEXT("Wariant koloru"),A.ClothingVariants.FindRef(S),0,P->Variants.Num()-1);
        }
        if (Category==EAppearanceCategory::Voice) { TArray<FName> Voices; for (const auto& V:C->VoiceProfiles) Voices.Add(V.ID); Choice(TEXT("Voice"),TEXT("Profil głosu"),Voices,A.VoiceProfileID); Button(TEXT("VoicePreview"),TEXT("ODSŁUCH GŁOSU")); }
        Button(TEXT("Random"),TEXT("LOSOWA POSTAĆ")); Button(TEXT("Quick"),TEXT("SZYBKI START")); Button(TEXT("Default"),TEXT("UŻYJ DOMYŚLNEJ POSTACI"));
        Button(TEXT("Undo"),TEXT("COFNIJ")); Button(TEXT("Redo"),TEXT("PONÓW")); Button(TEXT("Reset"),TEXT("RESET KATEGORII")); Button(TEXT("ResetAll"),TEXT("RESET CAŁOŚCI")); Button(TEXT("Summary"),TEXT("PODSUMOWANIE →"));
    }
    Button(TEXT("FullBody"),TEXT("CAŁA SYLWETKA")); Button(TEXT("UpperBody"),TEXT("GÓRNA CZĘŚĆ")); Button(TEXT("Face"),TEXT("KAMERA TWARZY"));
    Button(TEXT("RotateLeft"),TEXT("↶ OBRÓĆ W LEWO")); Button(TEXT("RotateRight"),TEXT("OBRÓĆ W PRAWO ↷"));
    Button(TEXT("Lighting"),TEXT("ŚWIATŁO: NEUTRALNE / DZIEŃ / NOC")); Button(TEXT("Movement"),TEXT("ANIMACJA: IDLE / WALK / JOG / CROUCH"));
    Button(TEXT("Debug"),TEXT("DEBUG / ID / MORPHY / SEED"));
    if (Debug) { Button(TEXT("Reload"),TEXT("RELOAD APPEARANCE")); Button(TEXT("Validate"),TEXT("VALIDATE APPEARANCE")); TArray<FName> Bodies; for (const auto& B:C->Bodies) Bodies.Add(B.ID); Choice(TEXT("ForceBody"),TEXT("ForceBodyPreset"),Bodies,A.BodyPreset); }
    Button(TEXT("Cancel"),TEXT("WRÓĆ DO MENU"));
    Status->SetText(FText::FromString(Debug?Creator->DebugText():FString::Printf(TEXT("Seed: %d • wygląd zostanie zapisany przed startem"),A.RandomSeed)));
}
void UCharacterCreatorWidget::Action(FName ID)
{
    if (ID==TEXT("Cancel")) { CastChecked<ASliceController>(GetOwningPlayer())->CancelCharacterCreator(); return; }
    if (ID==TEXT("Start")) { if (!CastChecked<ASliceController>(GetOwningPlayer())->StartCreatedCampaign()) Status->SetText(FText::FromString(TEXT("Nie udało się zapisać postaci. Kampania nie została uruchomiona."))); return; }
    if (ID==TEXT("Mode")) Advanced=!Advanced;
    if (ID==TEXT("Undo")) Creator->Undo(); if (ID==TEXT("Redo")) Creator->Redo();
    if (ID==TEXT("Reset")) Creator->ResetCategory(Category); if (ID==TEXT("ResetAll")) Creator->ResetAll();
    if (ID==TEXT("Random") || ID==TEXT("Quick")) Creator->Randomize(FMath::Rand());
    if (ID==TEXT("Default")) Creator->ResetAll();
    if (ID==TEXT("Summary") || ID==TEXT("Quick") || ID==TEXT("Default")) { Creator->bSummary=true; Studio->SetView(TEXT("FullBody")); }
    if (ID==TEXT("Edit")) Creator->bSummary=false;
    if (ID==TEXT("FullBody") || ID==TEXT("UpperBody") || ID==TEXT("Face")) Studio->SetView(ID);
    if (ID==TEXT("RotateLeft")) Studio->Rotate(-20); if (ID==TEXT("RotateRight")) Studio->Rotate(20);
    if (ID==TEXT("Debug")) Debug=!Debug;
    if (ID==TEXT("Reload")) Studio->Appearance->ReloadAppearance();
    if (ID==TEXT("Lighting")) { static int32 Index=0; const FName P[]={TEXT("Modern"),TEXT("Neutral"),TEXT("Daylight"),TEXT("Night")}; Studio->SetLighting(P[(++Index)%4]); }
    if (ID==TEXT("Movement")) { const FName P[]={TEXT("Idle"),TEXT("Walk"),TEXT("Jog"),TEXT("Crouch")}; int32 I=0; for (; I<4 && P[I]!=Studio->Appearance->PreviewMovement; ++I) {} Studio->Appearance->PreviewMovement=P[(I+1)%4]; }
    Refresh();
    if (ID==TEXT("Validate")) { auto Issues=UCharacterCreatorValidator::ValidateCatalog(Creator->Catalog); Status->SetText(FText::FromString(Issues.IsEmpty()?TEXT("Walidacja OK (placeholdery dozwolone)"):FString::Join(Issues,TEXT("\n")))); }
    if (ID==TEXT("VoicePreview")) { auto* V=Creator->Catalog->VoiceProfiles.FindByPredicate([&](const auto& X){return X.ID==Creator->Draft.VoiceProfileID;}); if (V && V->Preview.LoadSynchronous()) UGameplayStatics::PlaySound2D(this,V->Preview.Get()); else Status->SetText(FText::FromString(TEXT("Profil przygotowany; brak nagrania głosu w katalogu."))); }
}
void UCharacterCreatorWidget::SetNumber(FName ID,float V)
{
    auto A=Creator->Draft;
    if (ID==TEXT("Height")) A.Height=V; else if (ID==TEXT("Age")) A.VisualAge=V;
    else if (ID==TEXT("SkinTone")) A.SkinTone=FMath::RoundToInt(V); else if (ID==TEXT("Roughness")) A.SkinRoughness=V;
    else if (ID==TEXT("Freckles")) A.Freckles=V; else if (ID==TEXT("Imperfections")) A.SkinImperfections=V;
    else if (ID==TEXT("EyeColor")) A.EyeColor=FMath::RoundToInt(V); else if (ID==TEXT("HairColor")) A.HairColor=FMath::RoundToInt(V);
    else if (ID==TEXT("BrowColor")) A.EyebrowColor=FMath::RoundToInt(V); else if (ID==TEXT("BeardColor")) A.BeardColor=FMath::RoundToInt(V);
    else if (ID.ToString().StartsWith(TEXT("Color"))) A.ClothingVariants.Add(static_cast<EClothingSlot>(FCString::Atoi(*ID.ToString().Mid(5))),FMath::RoundToInt(V));
    else A.FaceMorphs.Add(ID,V);
    Creator->Change(A);
}
void UCharacterCreatorWidget::SetChoice(FName ID,FName V)
{
    auto A=Creator->Draft;
    if (ID==TEXT("Category")) { for (int32 I=0; I<13; ++I) if (V==FName(Categories[I])) Category=static_cast<EAppearanceCategory>(I); Studio->SetView(Category==EAppearanceCategory::Clothing || Category==EAppearanceCategory::Body || Category==EAppearanceCategory::Character?TEXT("FullBody"):TEXT("Face")); Refresh(); return; }
    if (ID==TEXT("Preset")) { for (int32 I=0; I<Creator->Catalog->Presets.Num(); ++I) if (Creator->Catalog->Presets[I].PresetID==V) Creator->SelectPreset(I); Refresh(); return; }
    if (ID==TEXT("Sex")) A.Sex=V==TEXT("Female")?EAppearanceSex::Female:EAppearanceSex::Male;
    if (ID==TEXT("Build")) A.BodyBuild=V; if (ID==TEXT("FacePreset")) { A.FacePreset=V; for (const auto& P:Creator->Catalog->Presets) if (P.FacePreset==V) { A.FaceMorphs=P.FaceMorphs; break; } }
    if (ID==TEXT("Hair")) A.HairStyle=V; if (ID==TEXT("Beard")) A.BeardStyle=V; if (ID==TEXT("Brows")) A.EyebrowStyle=V;
    if (ID==TEXT("EyeShape")) A.EyeShape=V; if (ID==TEXT("Voice")) A.VoiceProfileID=V;
    if (ID==TEXT("ForceBody")) if (const auto* B=Creator->Catalog->Body(V)) { A.BodyPreset=V; A.Sex=B->Sex; }
    if (ID.ToString().StartsWith(TEXT("Slot"))) { auto S=static_cast<EClothingSlot>(FCString::Atoi(*ID.ToString().Mid(4))); if (V.IsNone()) A.Clothing.Remove(S); else A.Clothing.Add(S,V); A.ClothingVariants.Add(S,0); }
    Creator->Change(A); Refresh();
}
void UCharacterCreatorWidget::SetText(FName ID,const FString& V)
{
    if (ID==TEXT("Seed")) { int32 Seed; if (LexTryParseString(Seed,*V)) Creator->Randomize(Seed); }
    else { auto A=Creator->Draft; A.Name=V; Creator->Change(A); } Refresh();
}
FReply UCharacterCreatorWidget::NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E)
{ if (E.GetEffectingButton()==EKeys::RightMouseButton) { Dragging=true; return FReply::Handled().CaptureMouse(TakeWidget()); } return Super::NativeOnMouseButtonDown(G,E); }
FReply UCharacterCreatorWidget::NativeOnMouseButtonUp(const FGeometry& G,const FPointerEvent& E)
{ if (Dragging) { Dragging=false; return FReply::Handled().ReleaseMouseCapture(); } return Super::NativeOnMouseButtonUp(G,E); }
FReply UCharacterCreatorWidget::NativeOnMouseMove(const FGeometry& G,const FPointerEvent& E)
{ if (Dragging && Studio) { Studio->Rotate(E.GetCursorDelta().X*.4f); return FReply::Handled(); } return Super::NativeOnMouseMove(G,E); }
FReply UCharacterCreatorWidget::NativeOnMouseWheel(const FGeometry& G,const FPointerEvent& E)
{ if (Studio) Studio->Zoom(-E.GetWheelDelta()*15); return FReply::Handled(); }
