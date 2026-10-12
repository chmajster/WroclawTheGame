#include "UI/CharacterCreatorWidget.h"
#include "Character/CharacterCreator.h"
#include "Character/CharacterCreatorSubsystem.h"
#include "Character/CharacterAppearanceComponent.h"
#include "UI/SliceController.h"
#include "UI/Menu/MenuStyle.h"
#include "UI/PerformanceSettings.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/SkeletalMesh.h"
#include "Styling/CoreStyle.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"

namespace {
struct FCreatorCategory { const TCHAR* Name; EAppearanceCategory Category; bool Face; };
const FCreatorCategory Categories[]={
    {TEXT("Tożsamość"),EAppearanceCategory::Character,false},{TEXT("Twarz"),EAppearanceCategory::Face,true},
    {TEXT("Włosy"),EAppearanceCategory::Hair,true},{TEXT("Zarost"),EAppearanceCategory::Beard,true},
    {TEXT("Oczy"),EAppearanceCategory::Eyes,true},{TEXT("Nos"),EAppearanceCategory::Nose,true},
    {TEXT("Usta"),EAppearanceCategory::Mouth,true},{TEXT("Szczęka"),EAppearanceCategory::Jaw,true},
    {TEXT("Sylwetka"),EAppearanceCategory::Body,false},{TEXT("Skóra"),EAppearanceCategory::Skin,true},
    {TEXT("Ubrania"),EAppearanceCategory::Clothing,false},{TEXT("Akcesoria"),EAppearanceCategory::Accessories,false},
    {TEXT("Podsumowanie"),EAppearanceCategory::Summary,false}};
TSharedRef<SWidget> Text(const FString& S,int32 Size=18,FLinearColor Color=WTGMenu::Paper,bool Bold=false)
{return WTGMenu::Text(S,Size,Color,Bold);}
class SCreatorIcon : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SCreatorIcon){} SLATE_ARGUMENT(int32, Index) SLATE_END_ARGS()
    void Construct(const FArguments& A){Index=A._Index;}
    virtual FVector2D ComputeDesiredSize(float) const override {return FVector2D(24);}
    virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& E,int32 L,const FWidgetStyle&,bool) const override
    {
        auto Line=[&](std::initializer_list<FVector2D> V){TArray<FVector2D>P(V);FSlateDrawElement::MakeLines(E,L,G.ToPaintGeometry(),P,ESlateDrawEffect::None,WTGMenu::Accent,true,1.5f);};
        if(Index==10 || Index==11){Line({{4,8},{9,3},{15,3},{20,8},{17,11},{17,21},{7,21},{7,11},{4,8}});if(Index==11)Line({{7,15},{17,15}});}
        else if(Index==12){Line({{5,7},{9,11},{19,3}});Line({{5,17},{19,17}});Line({{5,22},{16,22}});}
        else if(Index==4){Line({{2,12},{7,7},{17,7},{22,12},{17,17},{7,17},{2,12}});Line({{12,8},{12,16}});}
        else if(Index==5){Line({{12,3},{10,15},{16,17},{8,17}});}
        else if(Index==6){Line({{3,12},{8,9},{12,11},{16,9},{21,12},{16,16},{8,16},{3,12}});}
        else if(Index==8){Line({{8,2},{16,2},{16,6},{21,10},{18,13},{17,22},{13,22},{12,15},{11,22},{7,22},{6,13},{3,10},{8,6},{8,2}});}
        else {Line({{7,3},{17,3},{21,9},{18,19},{12,23},{6,19},{3,9},{7,3}});if(Index==2)Line({{4,9},{8,6},{12,9},{16,6},{20,9}});else if(Index==3)Line({{6,15},{12,19},{18,15}});else if(Index==9)Line({{8,11},{8,12}});else Line({{8,10},{10,10}});}
        return L;
    }
private:int32 Index=0;
};
class SCreatorCategoryButton : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCreatorCategoryButton){} SLATE_ARGUMENT(UCharacterCreatorWidget*,Owner) SLATE_ARGUMENT(int32,Index) SLATE_END_ARGS()
    void Construct(const FArguments& A)
    {
        Owner=A._Owner;Index=A._Index;
        ChildSlot[SNew(SButton).ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder")).ContentPadding(0)
            .OnClicked_Lambda([this]{if(Owner.IsValid())Owner->SelectCategory(Index);return FReply::Handled();})
            [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor_Lambda([this]{return FLinearColor(.78f,.67f,.43f,.015f+.13f*Highlight);}).Padding(FMargin(9,10))
                [SNew(SHorizontalBox)+SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SCreatorIcon).Index(Index)]
                 +SHorizontalBox::Slot().FillWidth(1).Padding(9,0)[SNew(STextBlock).Text(FText::FromString(Categories[Index].Name)).Font(WTGMenu::Font(18))
                    .ColorAndOpacity_Lambda([this]{return FSlateColor(FMath::Lerp(WTGMenu::Paper,WTGMenu::Accent,Highlight));})]]]];
    }
    virtual void Tick(const FGeometry& G,double T,float Dt) override
    {SCompoundWidget::Tick(G,T,Dt);const float V=(Owner.IsValid() && Owner->SelectedCategory()==Index) || IsHovered() || HasFocusedDescendants()?1.f:0.f;Highlight=UWTGPerformanceSettings::Get()->bReduceUIMotion?V:FMath::FInterpTo(Highlight,V,Dt,14.f);}
private:TWeakObjectPtr<UCharacterCreatorWidget> Owner;int32 Index=0;float Highlight=0;
};
class SStudioPreview : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SStudioPreview){} SLATE_ARGUMENT(UCharacterCreatorWidget*, Owner) SLATE_END_ARGS()
    void Construct(const FArguments& A){Owner=A._Owner;ChildSlot[SNew(SImage).Image(&Owner->PreviewBrush)];}
    virtual void Tick(const FGeometry& G,double Time,float Dt) override
    {SCompoundWidget::Tick(G,Time,Dt);if(Owner.IsValid() && Owner->Studio){auto S=G.GetAbsoluteSize();if(S.X>0 && S.Y>0)Owner->Studio->ResizePreview(FIntPoint(FMath::RoundToInt(S.X),FMath::RoundToInt(S.Y)));}}
    virtual FReply OnMouseButtonDown(const FGeometry&,const FPointerEvent& E) override
    {if(E.GetEffectingButton()==EKeys::LeftMouseButton || E.GetEffectingButton()==EKeys::RightMouseButton){Dragging=true;return FReply::Handled().CaptureMouse(SharedThis(this));}return FReply::Unhandled();}
    virtual FReply OnMouseButtonUp(const FGeometry&,const FPointerEvent&) override
    {if(Dragging){Dragging=false;return FReply::Handled().ReleaseMouseCapture();}return FReply::Unhandled();}
    virtual void OnMouseCaptureLost(const FCaptureLostEvent&) override {Dragging=false;}
    virtual FReply OnMouseMove(const FGeometry&,const FPointerEvent& E) override
    {if(Dragging && Owner.IsValid() && Owner->Studio){Owner->Studio->Rotate(E.GetCursorDelta().X*.45f);return FReply::Handled();}return FReply::Unhandled();}
    virtual FReply OnMouseWheel(const FGeometry&,const FPointerEvent& E) override
    {if(Owner.IsValid() && Owner->Studio){Owner->Studio->Zoom(-E.GetWheelDelta()*18);return FReply::Handled();}return FReply::Unhandled();}
private:TWeakObjectPtr<UCharacterCreatorWidget> Owner;bool Dragging=false;
};
}
class SCharacterCreatorPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCharacterCreatorPanel){} SLATE_ARGUMENT(UCharacterCreatorWidget*,Owner) SLATE_END_ARGS()
    void Construct(const FArguments& A)
    {
        Owner=A._Owner;auto Nav=SNew(SVerticalBox);
        Nav->AddSlot().AutoHeight().Padding(8,8,0,16)[Text(TEXT("WYGLĄD"),16,WTGMenu::Muted,true)];
        for(int32 I=0;I<UE_ARRAY_COUNT(Categories);++I)Nav->AddSlot().AutoHeight().Padding(0,2)[SNew(SCreatorCategoryButton).Owner(Owner.Get()).Index(I)];
        auto Views=SNew(SHorizontalBox);TArray<TPair<FString,float>> Angles={{TEXT("Przód"),0},{TEXT("Profil"),90},{TEXT("Tył"),180}};
        for(auto P:Angles)Views->AddSlot().FillWidth(1).Padding(3)[Button(P.Key,[this,P]{Owner->Studio->SetAngle(P.Value);})];
        auto Tools=SNew(SHorizontalBox);
        Tools->AddSlot().AutoWidth().Padding(0,0,8,0)[Button(TEXT("← Menu"),[this]{Owner->Action(TEXT("Cancel"));})];
        Tools->AddSlot().AutoWidth().Padding(0,0,8,0)[Button(TEXT("Losuj"),[this]{Owner->Action(TEXT("Random"));})];
        Tools->AddSlot().AutoWidth().Padding(0,0,8,0)[Button(TEXT("Cofnij"),[this]{Owner->Action(TEXT("Undo"));},TAttribute<bool>::CreateLambda([this]{return Owner->Creator->CanUndo();}))];
        Tools->AddSlot().AutoWidth()[Button(TEXT("Ponów"),[this]{Owner->Action(TEXT("Redo"));},TAttribute<bool>::CreateLambda([this]{return Owner->Creator->CanRedo();}))];
        Tools->AddSlot().FillWidth(1).HAlign(HAlign_Right)[Button(TEXT("Gotowe →"),[this]{Owner->SelectCategory(12);})];
        ChildSlot[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(WTGMenu::Ink).Padding(18)
            [SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)[SNew(SHorizontalBox)
                    +SHorizontalBox::Slot().FillWidth(1)[Text(TEXT("WROCŁAW  /  TWOJA POSTAĆ"),26,WTGMenu::Paper,true)]
                    +SHorizontalBox::Slot().AutoWidth()[Text(TEXT("STUDIO NADODRZE"),14,WTGMenu::Accent)]]
                +SVerticalBox::Slot().FillHeight(1)[SNew(SHorizontalBox)
                    +SHorizontalBox::Slot().FillWidth(.16f).Padding(0,0,10,0)[WTGMenu::Card(SNew(SScrollBox)+SScrollBox::Slot()[Nav],FMargin(6))]
                    +SHorizontalBox::Slot().FillWidth(.56f)[SNew(SVerticalBox)
                        +SVerticalBox::Slot().FillHeight(1)[SNew(SStudioPreview).Owner(Owner.Get())]
                        +SVerticalBox::Slot().AutoHeight().Padding(12,7)[Views]
                        +SVerticalBox::Slot().AutoHeight().Padding(12,4)[Text(TEXT("Przeciągnij: obrót  •  Kółko: przybliżenie"),14,WTGMenu::Muted)]]
                    +SHorizontalBox::Slot().FillWidth(.28f).Padding(12,0,0,0)[WTGMenu::Card(SNew(SVerticalBox)
                        +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,16)[SNew(STextBlock).Text_Lambda([this]{return FText::FromString(Categories[Owner->SelectedCategory()].Name);}).Font(WTGMenu::Font(25,true)).ColorAndOpacity(WTGMenu::Accent)]
                        +SVerticalBox::Slot().FillHeight(1)[SNew(SScrollBox)+SScrollBox::Slot()[SAssignNew(Content,SVerticalBox)]]
                        +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,0)[Button(TEXT("Przywróć kategorię"),[this]{Owner->Action(TEXT("Reset"));})],FMargin(20))]]
                +SVerticalBox::Slot().AutoHeight().Padding(0,14,0,0)[Tools]
                +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,0)[SNew(STextBlock).Text_Lambda([this]{return FText::FromString(Owner.IsValid()?Owner->Status:FString());}).Font(WTGMenu::Font(14)).ColorAndOpacity(WTGMenu::Muted)]]];
        Refresh();
    }
    void Refresh()
    {
        if(!Owner.IsValid() || !Content)return;Content->ClearChildren();Fade=0;
        auto* C=Owner->Creator->Catalog.Get();const auto& A=Owner->Creator->Draft;const auto Cat=Categories[Owner->SelectedCategory()].Category;
        if(Cat==EAppearanceCategory::Character)
        {
            Content->AddSlot().AutoHeight().Padding(0,0,0,18)[Text(TEXT("Zacznij od wyglądu bazowego i dopracuj szczegóły."),17,WTGMenu::Muted)];
            Content->AddSlot().AutoHeight().Padding(0,4)[Text(TEXT("Imię"))];
            Content->AddSlot().AutoHeight().Padding(0,6,0,18)[SNew(SEditableTextBox).Text(FText::FromString(A.Name)).Font(WTGMenu::Font(22))
                .OnTextCommitted_Lambda([this](const FText& V,ETextCommit::Type){Owner->SetText(TEXT("Name"),V.ToString());})];
            Options(TEXT("Baza"),TEXT("Sex"),{{TEXT("Male"),TEXT("Męska")},{TEXT("Female"),TEXT("Żeńska")}},A.Sex==EAppearanceSex::Male?TEXT("Male"):TEXT("Female"));
            TArray<TPair<FName,FString>> P;for(int32 I=0;I<C->Presets.Num();++I)P.Add({C->Presets[I].PresetID,FString::Printf(TEXT("Wygląd %d"),I+1)});Options(TEXT("Punkt wyjścia"),TEXT("Preset"),P,A.PresetID);
        }
        if(Cat==EAppearanceCategory::Body){Number(TEXT("Height"),TEXT("Wzrost (cm)"),160,195);Options(TEXT("Budowa"),TEXT("Build"),{{TEXT("Slim"),TEXT("Smukła")},{TEXT("Average"),TEXT("Średnia")},{TEXT("Athletic"),TEXT("Atletyczna")},{TEXT("Heavy"),TEXT("Masywna")}},A.BodyBuild);}
        if(Cat==EAppearanceCategory::Hair){Parts(TEXT("Fryzura"),TEXT("Hair"),C->HairStyleDefinitions,A.HairStyle);Palette(TEXT("Kolor włosów"),TEXT("HairColor"),C->HairPalette);Parts(TEXT("Brwi"),TEXT("Brows"),C->EyebrowStyleDefinitions,A.EyebrowStyle);Palette(TEXT("Kolor brwi"),TEXT("BrowColor"),C->HairPalette);}
        if(Cat==EAppearanceCategory::Beard){Parts(TEXT("Zarost"),TEXT("Beard"),C->BeardStyleDefinitions,A.BeardStyle);Palette(TEXT("Kolor zarostu"),TEXT("BeardColor"),C->HairPalette);}
        if(Cat==EAppearanceCategory::Eyes)Palette(TEXT("Kolor tęczówki"),TEXT("EyeColor"),C->EyePalette);
        if(Cat==EAppearanceCategory::Skin){Palette(TEXT("Odcień skóry"),TEXT("SkinTone"),C->SkinPalette);Number(TEXT("Roughness"),TEXT("Matowość skóry"),.3f,.8f);Number(TEXT("Freckles"),TEXT("Piegi"),0,1);Number(TEXT("Imperfections"),TEXT("Nierówności"),0,1);Number(TEXT("Age"),TEXT("Wiek wizualny"),20,45);}
        auto* B=C->Body(A.BodyPreset);auto* Mesh=B?(B->FaceMesh.Get()?B->FaceMesh.Get():B->Mesh.Get()):nullptr;
        int32 Count=0;for(const auto& M:C->Morphs)if(M.Category==Cat && Mesh && Mesh->FindMorphTarget(M.MorphTargetName)){Number(M.MorphID,M.DisplayName.ToString(),M.MinValue,M.MaxValue);++Count;}
        if((Cat==EAppearanceCategory::Face || Cat==EAppearanceCategory::Nose || Cat==EAppearanceCategory::Mouth || Cat==EAppearanceCategory::Jaw) && Count==0)Content->AddSlot().AutoHeight()[Text(TEXT("Ta baza nie ma aktywnych kształtów tej części."),17,WTGMenu::Muted)];
        if(Cat==EAppearanceCategory::Clothing || Cat==EAppearanceCategory::Accessories)
        {
            const TCHAR* Slots[]={TEXT("Kurtka"),TEXT("Góra"),TEXT("Spodnie"),TEXT("Buty"),TEXT("Nakrycie głowy"),TEXT("Akcesoria"),TEXT("Plecak")};
            for(int32 I=Cat==EAppearanceCategory::Clothing?0:4;I<(Cat==EAppearanceCategory::Clothing?4:7);++I){TArray<FAppearancePartDefinition>P;for(const auto& X:C->ClothingDefinitions)if(X.Slot==static_cast<EClothingSlot>(I))P.Add(X);Parts(Slots[I],FName(*FString::Printf(TEXT("Slot%d"),I)),P,A.Clothing.FindRef(static_cast<EClothingSlot>(I)),I==0 || I>=4);
                if(const auto* Item=C->Part(C->ClothingDefinitions,A.Clothing.FindRef(static_cast<EClothingSlot>(I))))if(Item->ColorSupport && Item->Variants.Num()){TArray<FLinearColor> Colors;for(const auto& V:Item->Variants)Colors.Add(V.Color);Palette(TEXT("Kolor"),FName(*FString::Printf(TEXT("Color%d"),I)),Colors);}}
        }
        if(Cat==EAppearanceCategory::Summary){Content->AddSlot().AutoHeight().Padding(0,0,0,16)[Text(A.Name,30,WTGMenu::Paper,true)];Content->AddSlot().AutoHeight().Padding(0,0,0,16)[Text(FString::Printf(TEXT("%.0f cm  /  %.0f lat"),A.Height,A.VisualAge),20)];for(const auto& P:A.Clothing)if(const auto* Item=C->Part(C->ClothingDefinitions,P.Value))Content->AddSlot().AutoHeight().Padding(0,4)[Text(Item->DisplayName.ToString(),18,WTGMenu::Muted)];Content->AddSlot().AutoHeight().Padding(0,22)[Button(TEXT("Rozpocznij grę →"),[this]{Owner->Action(TEXT("Start"));})];Content->AddSlot().AutoHeight()[Text(TEXT("Wygląd zostanie zapisany i zastosowany do bohatera w świecie gry."),16,WTGMenu::Muted)];}
    }
    virtual bool SupportsKeyboardFocus() const override {return true;}
    virtual FReply OnKeyDown(const FGeometry&,const FKeyEvent& E) override
    {if(E.GetKey()==EKeys::Escape){Owner->Action(TEXT("Cancel"));return FReply::Handled();}if(E.IsControlDown() && E.GetKey()==EKeys::Z){Owner->Action(E.IsShiftDown()?TEXT("Redo"):TEXT("Undo"));return FReply::Handled();}if(E.GetKey()==EKeys::PageDown || E.GetKey()==EKeys::PageUp){Owner->SelectCategory((Owner->SelectedCategory()+(E.GetKey()==EKeys::PageDown?1:12))%13);return FReply::Handled();}return FReply::Unhandled();}
    virtual void Tick(const FGeometry& G,double T,float Dt) override
    {SCompoundWidget::Tick(G,T,Dt);Fade=FMath::Min(Fade+Dt*8,1.f);if(Content)Content->SetRenderOpacity(UWTGPerformanceSettings::Get()->bReduceUIMotion?1.f:.45f+.55f*Fade);}
private:
    TWeakObjectPtr<UCharacterCreatorWidget> Owner;TSharedPtr<SVerticalBox> Content;float Fade=1;
    TSharedRef<SWidget> Button(const FString& S,TFunction<void()> Click,TAttribute<bool> Enabled=true)
    {return SNew(SButton).ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder")).ContentPadding(FMargin(12,10)).IsEnabled(Enabled).OnClicked_Lambda([Click]{Click();return FReply::Handled();})[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(WTGMenu::Panel).Padding(4)[Text(S,18)]];}
    float Value(FName ID) const
    {const auto& A=Owner->Creator->Draft;if(ID==TEXT("Height"))return A.Height;if(ID==TEXT("Age"))return A.VisualAge;if(ID==TEXT("Roughness"))return A.SkinRoughness;if(ID==TEXT("Freckles"))return A.Freckles;if(ID==TEXT("Imperfections"))return A.SkinImperfections;if(ID==TEXT("SkinTone"))return A.SkinTone;if(ID==TEXT("EyeColor"))return A.EyeColor;if(ID==TEXT("HairColor"))return A.HairColor;if(ID==TEXT("BrowColor"))return A.EyebrowColor;if(ID==TEXT("BeardColor"))return A.BeardColor;if(ID.ToString().StartsWith(TEXT("Color")))return A.ClothingVariants.FindRef(static_cast<EClothingSlot>(FCString::Atoi(*ID.ToString().Mid(5))));return A.FaceMorphs.FindRef(ID);}
    void Number(FName ID,const FString& S,float Min,float Max)
    {Content->AddSlot().AutoHeight().Padding(0,10,0,3)[SNew(SHorizontalBox)+SHorizontalBox::Slot().FillWidth(1)[Text(S,18)]+SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text_Lambda([this,ID,Min,Max]{return FText::AsNumber((Max-Min)>5?FMath::RoundToInt(Value(ID)):FMath::RoundToInt((Value(ID)-Min)/(Max-Min)*100));}).Font(WTGMenu::Font(16)).ColorAndOpacity(WTGMenu::Muted)]];Content->AddSlot().AutoHeight().Padding(0,8,0,12)[SNew(SSlider).Value_Lambda([this,ID,Min,Max]{return (Value(ID)-Min)/(Max-Min);}).StepSize(.01f).SliderBarColor(WTGMenu::Muted).SliderHandleColor(WTGMenu::Accent).OnValueChanged_Lambda([this,ID,Min,Max](float V){Owner->SetNumber(ID,FMath::Lerp(Min,Max,V));})];}
    void Options(const FString& Label,FName Field,const TArray<TPair<FName,FString>>& Items,FName Selected)
    {Content->AddSlot().AutoHeight().Padding(0,12,0,6)[Text(Label,18)];auto Grid=SNew(SUniformGridPanel).SlotPadding(FMargin(3));for(int32 I=0;I<Items.Num();++I){auto ID=Items[I].Key;Grid->AddSlot(I%2,I/2)[SNew(SButton).ContentPadding(FMargin(8,12)).ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder")).OnClicked_Lambda([this,Field,ID]{Owner->SetChoice(Field,ID);return FReply::Handled();})[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(ID==Selected?FLinearColor(.14f,.12f,.085f):WTGMenu::Panel).Padding(8)[Text(Items[I].Value,17,ID==Selected?WTGMenu::Accent:WTGMenu::Paper)]]];}Content->AddSlot().AutoHeight()[Grid];}
    void Parts(const FString& Label,FName Field,const TArray<FAppearancePartDefinition>& P,FName Selected,bool Optional=false)
    {TArray<TPair<FName,FString>> Items;if(Optional)Items.Add({NAME_None,TEXT("Brak")});for(const auto& X:P)if(Owner->Creator->Catalog->Compatible(X,Owner->Creator->Draft))Items.Add({X.ID,X.ID.IsNone()?TEXT("Brak"):X.DisplayName.ToString()});Options(Label,Field,Items,Selected);}
    void Palette(const FString& Label,FName Field,const TArray<FLinearColor>& Colors)
    {Content->AddSlot().AutoHeight().Padding(0,14,0,6)[Text(Label,18)];auto Grid=SNew(SUniformGridPanel).SlotPadding(FMargin(3));for(int32 I=0;I<Colors.Num();++I)Grid->AddSlot(I%6,I/6)[SNew(SButton).ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder")).ContentPadding(3).ToolTipText(FText::FromString(FString::Printf(TEXT("%s %d"),*Label,I+1))).OnClicked_Lambda([this,Field,I]{Owner->SetNumber(Field,float(I));return FReply::Handled();})[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor_Lambda([this,Field,I]{return FMath::RoundToInt(Value(Field))==I?WTGMenu::Accent:WTGMenu::Panel;}).Padding(3)[SNew(SBox).HeightOverride(26).WidthOverride(26)[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Colors[I])]]]];Content->AddSlot().AutoHeight()[Grid];}
};
void UCharacterCreatorWidget::NativeOnInitialized()
{Super::NativeOnInitialized();SetIsFocusable(true);Creator=GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>();FActorSpawnParameters P;P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;Studio=GetWorld()->SpawnActor<AWTG_CharacterCreator>(AWTG_CharacterCreator::StaticClass(),FVector(0,0,-50000),FRotator::ZeroRotator,P);PreviewBrush.SetResourceObject(Studio->RenderTarget);PreviewBrush.ImageSize=FVector2D(720,1000);Status=TEXT("Wygląd aktualizuje się na żywo.  PgUp / PgDn: kategorie  •  Ctrl+Z: cofnij");}
TSharedRef<SWidget> UCharacterCreatorWidget::RebuildWidget(){return SAssignNew(Panel,SCharacterCreatorPanel).Owner(this);}
void UCharacterCreatorWidget::ReleaseSlateResources(bool Children){Super::ReleaseSlateResources(Children);Panel.Reset();}
void UCharacterCreatorWidget::NativeDestruct(){if(Studio)Studio->Destroy();Studio=nullptr;Super::NativeDestruct();}
void UCharacterCreatorWidget::Refresh(){if(Panel)Panel->Refresh();}
void UCharacterCreatorWidget::SelectCategory(int32 I){CategoryIndex=FMath::Clamp(I,0,12);Creator->bSummary=CategoryIndex==12;Studio->SetView(Categories[CategoryIndex].Face?TEXT("Face"):TEXT("FullBody"));Refresh();}
void UCharacterCreatorWidget::Action(FName ID)
{if(ID==TEXT("Cancel")){CastChecked<ASliceController>(GetOwningPlayer())->CancelCharacterCreator();return;}if(ID==TEXT("Start")){if(!CastChecked<ASliceController>(GetOwningPlayer())->StartCreatedCampaign())Status=TEXT("Nie udało się zapisać postaci. Spróbuj ponownie.");return;}if(ID==TEXT("Undo"))Creator->Undo();if(ID==TEXT("Redo"))Creator->Redo();if(ID==TEXT("Random"))Creator->Randomize(FMath::Rand());if(ID==TEXT("Reset"))Creator->ResetCategory(Categories[CategoryIndex].Category);Refresh();}
void UCharacterCreatorWidget::SetNumber(FName ID,float V)
{auto A=Creator->Draft;if(ID==TEXT("Height"))A.Height=V;else if(ID==TEXT("Age"))A.VisualAge=V;else if(ID==TEXT("SkinTone"))A.SkinTone=FMath::RoundToInt(V);else if(ID==TEXT("Roughness"))A.SkinRoughness=V;else if(ID==TEXT("Freckles"))A.Freckles=V;else if(ID==TEXT("Imperfections"))A.SkinImperfections=V;else if(ID==TEXT("EyeColor"))A.EyeColor=FMath::RoundToInt(V);else if(ID==TEXT("HairColor"))A.HairColor=FMath::RoundToInt(V);else if(ID==TEXT("BrowColor"))A.EyebrowColor=FMath::RoundToInt(V);else if(ID==TEXT("BeardColor"))A.BeardColor=FMath::RoundToInt(V);else if(ID.ToString().StartsWith(TEXT("Color")))A.ClothingVariants.Add(static_cast<EClothingSlot>(FCString::Atoi(*ID.ToString().Mid(5))),FMath::RoundToInt(V));else A.FaceMorphs.Add(ID,V);Creator->Change(A);}
void UCharacterCreatorWidget::SetChoice(FName ID,FName V)
{auto A=Creator->Draft;if(ID==TEXT("Preset")){for(int32 I=0;I<Creator->Catalog->Presets.Num();++I)if(Creator->Catalog->Presets[I].PresetID==V)Creator->SelectPreset(I);Refresh();return;}if(ID==TEXT("Sex"))A.Sex=V==TEXT("Female")?EAppearanceSex::Female:EAppearanceSex::Male;if(ID==TEXT("Build"))A.BodyBuild=V;if(ID==TEXT("Hair"))A.HairStyle=V;if(ID==TEXT("Beard"))A.BeardStyle=V;if(ID==TEXT("Brows"))A.EyebrowStyle=V;if(ID.ToString().StartsWith(TEXT("Slot"))){auto S=static_cast<EClothingSlot>(FCString::Atoi(*ID.ToString().Mid(4)));if(V.IsNone())A.Clothing.Remove(S);else A.Clothing.Add(S,V);A.ClothingVariants.Add(S,0);}Creator->Change(A);Refresh();}
void UCharacterCreatorWidget::SetText(FName ID,const FString& V){auto A=Creator->Draft;A.Name=V;Creator->Change(A);}
