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
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace {
UTextBlock* Label(UWidgetTree* Tree,const FString& Text,int32 Size=16)
{
    auto* T=Tree->ConstructWidget<UTextBlock>(); T->SetText(FText::FromString(Text)); auto Font=T->GetFont(); Font.Size=Size; T->SetFont(Font); T->SetAutoWrapText(true); return T;
}
const TCHAR* Categories[]={TEXT("POSTAĆ"),TEXT("TWARZ"),TEXT("OCZY"),TEXT("NOS"),TEXT("USTA"),TEXT("SZCZĘKA"),TEXT("USZY"),TEXT("SKÓRA"),TEXT("WŁOSY"),TEXT("ZAROST"),TEXT("CIAŁO"),TEXT("UBRANIA"),TEXT("GŁOS")};
const TCHAR* Slots[]={TEXT("Kurtka"),TEXT("Koszulka"),TEXT("Spodnie"),TEXT("Buty"),TEXT("Czapka"),TEXT("Akcesoria"),TEXT("Plecak")};
}
void UAppearanceControl::Button(const FString& Name)
{
    auto* B=WidgetTree->ConstructWidget<UButton>(); WidgetTree->RootWidget=B; B->AddChild(Label(WidgetTree,Name)); B->SetBackgroundColor(FLinearColor(.1f,.15f,.2f)); B->OnClicked.AddDynamic(this,&UAppearanceControl::Click);
}
void UAppearanceControl::Number(const FString& Name,float V,float Min,float Max,float Step)
{
    auto* Box=WidgetTree->ConstructWidget<UVerticalBox>(); WidgetTree->RootWidget=Box; Box->AddChildToVerticalBox(Label(WidgetTree,Name));
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
    Super::NativeOnInitialized(); Creator=GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>();
    SetIsFocusable(true);
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* BP=LoadClass<AWTG_CharacterCreator>(nullptr,TEXT("/Game/CharacterCreator/BP_WTG_CharacterCreator.BP_WTG_CharacterCreator_C"));
    Studio=GetWorld()->SpawnActor<AWTG_CharacterCreator>(BP?BP:AWTG_CharacterCreator::StaticClass(),FVector(0,0,-50000),FRotator::ZeroRotator,Params);
    auto* Background=WidgetTree->ConstructWidget<UBorder>(); Background->SetBrushColor(FLinearColor(.018f,.023f,.032f)); Background->SetPadding(FMargin(24)); WidgetTree->RootWidget=Background;
    auto* Layout=WidgetTree->ConstructWidget<UHorizontalBox>(); Background->AddChild(Layout);
    auto* Left=WidgetTree->ConstructWidget<UVerticalBox>(); auto* LS=Layout->AddChildToHorizontalBox(Left); LS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Heading=Label(WidgetTree,TEXT("KREATOR POSTACI"),24); Left->AddChildToVerticalBox(Heading);
    auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>(); Left->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Controls=WidgetTree->ConstructWidget<UVerticalBox>(); Scroll->AddChild(Controls);
    auto* Center=WidgetTree->ConstructWidget<UVerticalBox>(); auto* CS=Layout->AddChildToHorizontalBox(Center); FSlateChildSize Fill(ESlateSizeRule::Fill); Fill.Value=1.8f; CS->SetSize(Fill); CS->SetPadding(FMargin(18,0));
    auto* Image=WidgetTree->ConstructWidget<UImage>(); FSlateBrush Brush; Brush.SetResourceObject(Studio->RenderTarget); Brush.ImageSize=FVector2D(720,1000); Image->SetBrush(Brush); Center->AddChildToVerticalBox(Image)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    if (auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/CharacterCreator/M_CharacterPreview.M_CharacterPreview")))
    {
        auto* MID=UMaterialInstanceDynamic::Create(Material,this); MID->SetTextureParameterValue(TEXT("PreviewTexture"),Studio->RenderTarget); Image->SetBrushFromMaterial(MID);
    }
    Center->AddChildToVerticalBox(Label(WidgetTree,TEXT("Przeciągnij prawym przyciskiem, aby obrócić • kółko: zoom\nPodgląd zastępczy — finalna jakość zależy od podłączonych assetów."),12));
    auto* CommandScroll=WidgetTree->ConstructWidget<UScrollBox>(); Layout->AddChildToHorizontalBox(CommandScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Commands=WidgetTree->ConstructWidget<UVerticalBox>(); CommandScroll->AddChild(Commands);
    Status=Label(WidgetTree,TEXT(""),12); Left->AddChildToVerticalBox(Status); Refresh();
}
void UCharacterCreatorWidget::NativeDestruct()
{ if (Studio) Studio->Destroy(); Studio=nullptr; Super::NativeDestruct(); }
void UCharacterCreatorWidget::Refresh()
{
    Controls->ClearChildren(); Commands->ClearChildren(); const auto& A=Creator->Draft; const auto* C=Creator->Catalog.Get();
    auto Add=[&](UVerticalBox* Box,UAppearanceControl* R){Box->AddChildToVerticalBox(R)->SetPadding(FMargin(0,5));};
    auto Button=[&](const TCHAR* ID,const FString& Text){auto* R=Row(Commands,ID); R->Button(Text); Add(Commands,R);};
    auto Choice=[&](FName ID,const FString& Text,const TArray<FName>& Values,FName Value){auto* R=Row(Controls,ID); R->Choice(Text,Values,Value); Add(Controls,R);};
    auto Num=[&](FName ID,const FString& Text,float V,float Min,float Max,float Step=1.f){auto* R=Row(Controls,ID); R->Number(Text,V,Min,Max,Step); Add(Controls,R);};
    auto Part=[&](FName Field,const FString& Text,const TArray<FAppearancePartDefinition>& Parts,FName Value){TArray<FName> IDs; for (const auto& P:Parts) if (C->Compatible(P,A)) IDs.Add(P.ID); Choice(Field,Text,IDs,Value);};
    Heading->SetText(FText::FromString(Creator->bSummary?TEXT("TWOJA POSTAĆ"):TEXT("KREATOR POSTACI")));
    if (Creator->bSummary)
    {
        Controls->AddChildToVerticalBox(Label(WidgetTree,FString::Printf(TEXT("%s\n%.0f cm • wiek wizualny %.0f\n%s\nSeed: %d\n"),*A.Name,A.Height,A.VisualAge,*A.BodyBuild.ToString(),A.RandomSeed),22));
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
    if (ID==TEXT("Lighting")) { static int32 Index=0; const FName P[]={TEXT("Neutral"),TEXT("Daylight"),TEXT("Night")}; Studio->SetLighting(P[(++Index)%3]); }
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
