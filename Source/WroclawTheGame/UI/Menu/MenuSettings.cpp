#include "UI/Menu/MenuShell.h"
#include "UI/Menu/MenuStyle.h"
#include "UI/Menu/MenuFlowSubsystem.h"
#include "UI/WTGInputProfileSubsystem.h"
#include "UI/PerformanceSettings.h"
#include "Audio/WTGAudioSettings.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Rendering/SlateRenderTransform.h"

TSharedRef<SWidget> SMenuShell::Choice(const FString& Label,const TArray<FString>& Values,int32 Selected,TFunction<void(int32)> Changed)
{
    auto Options=MakeShared<TArray<TSharedPtr<FString>>>();for(const auto& V:Values)Options->Add(MakeShared<FString>(V));
    auto Current=MakeShared<FString>(Values[FMath::Clamp(Selected,0,Values.Num()-1)]);
    auto Combo=SNew(SComboBox<TSharedPtr<FString>>).OptionsSource(&Options.Get()).InitiallySelectedItem((*Options)[FMath::Clamp(Selected,0,Values.Num()-1)])
        .OnGenerateWidget_Lambda([Options](TSharedPtr<FString> Item){return WTGMenu::Text(*Item,20);})
        .OnSelectionChanged_Lambda([Options,Current,Changed](TSharedPtr<FString> Item,ESelectInfo::Type){if(Item){*Current=*Item;Changed(Options->IndexOfByKey(Item));}})
        [SNew(STextBlock).Text_Lambda([Current]{return FText::FromString(*Current);}).Font(WTGMenu::Font(20)).ColorAndOpacity(WTGMenu::Paper)];
    FocusOrder.Add(Combo);
    return SNew(SHorizontalBox)
        +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(0,12)[WTGMenu::Text(Label,22)]
        +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(10,12)[Combo];
}
TSharedRef<SWidget> SMenuShell::ValueSlider(const FString& Label,TAttribute<float> Value,TFunction<void(float)> Changed)
{
    auto Slider=SNew(SSlider).Value(Value).StepSize(.05f).SliderBarColor(WTGMenu::Muted).SliderHandleColor(WTGMenu::Accent)
        .OnValueChanged_Lambda([Changed](float V){Changed(V);});FocusOrder.Add(Slider);
    return SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight().Padding(0,14,0,8)
            [SNew(STextBlock).Text_Lambda([Label,Value]{return FText::FromString(FString::Printf(TEXT("%s  /  %.0f%%"),*Label,Value.Get()*100));}).Font(WTGMenu::Font(20)).ColorAndOpacity(WTGMenu::Paper)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,20)[Slider];
}
TSharedRef<SWidget> SMenuShell::ValueToggle(const FString& Label,TAttribute<bool> Value,TFunction<void(bool)> Changed)
{
    auto Check=SNew(SCheckBox).IsChecked_Lambda([Value]{return Value.Get()?ECheckBoxState::Checked:ECheckBoxState::Unchecked;})
        .OnCheckStateChanged_Lambda([Changed](ECheckBoxState S){Changed(S==ECheckBoxState::Checked);})
        .Padding(FMargin(10,16))[WTGMenu::Text(Label,22)];FocusOrder.Add(Check);return Check;
}
void SMenuShell::BuildSettings()
{
    Content->AddSlot().AutoHeight()[WTGMenu::Text(TEXT("Ustawienia"),38,WTGMenu::Paper,true)];
    auto SettingsCategoryRow=SNew(SHorizontalBox);const TCHAR* Names[]={TEXT("OBRAZ"),TEXT("DŹWIĘK"),TEXT("INTERFEJS")};
    for(int32 I=0;I<3;++I)SettingsCategoryRow->AddSlot().FillWidth(1).Padding(0,18,6,20)[Action(Names[I],[this,I]{SettingsCategory=I;Refresh();})];
    Content->AddSlot().AutoHeight()[SettingsCategoryRow];auto Form=SNew(SVerticalBox);
    Content->AddSlot().AutoHeight()[WTGMenu::Card(Form)];
    if(SettingsCategory==0){
        auto* S=GEngine->GetGameUserSettings();
        if(!VideoPending){PendingResolution=S->GetScreenResolution();PendingWindowMode=int32(S->GetFullscreenMode());}
        const TArray<FIntPoint> Sizes={{1280,720},{1600,900},{1920,1080},{2560,1440},{3440,1440},{3840,2160}};
        TArray<FString> SizeNames;int32 Current=0;for(int32 I=0;I<Sizes.Num();++I){SizeNames.Add(FString::Printf(TEXT("%d × %d"),Sizes[I].X,Sizes[I].Y));if(Sizes[I]==PendingResolution)Current=I;}
        Form->AddSlot().AutoHeight()[Choice(TEXT("Rozdzielczość"),SizeNames,Current,[this,Sizes](int32 I){PendingResolution=Sizes[I];})];
        Form->AddSlot().AutoHeight()[Choice(TEXT("Tryb ekranu"),{TEXT("Pełny ekran"),TEXT("Bez ramek"),TEXT("Okno")},PendingWindowMode,[this](int32 I){PendingWindowMode=I;})];
        Form->AddSlot().AutoHeight()[Action(TEXT("Zastosuj tryb ekranu"),[this]{BeginVideoChange();})];
        Form->AddSlot().AutoHeight()[ValueToggle(TEXT("Synchronizacja pionowa"),TAttribute<bool>::CreateLambda([S]{return S->IsVSyncEnabled();}),[S](bool V){S->SetVSyncEnabled(V);S->ApplyNonResolutionSettings();S->SaveSettings();})];
        const TArray<int32> Limits={0,30,60,90,120,144,165,240};int32 Limit=Limits.IndexOfByKey(UWTGPerformanceSettings::Get()->FPSLimit);
        Form->AddSlot().AutoHeight()[Choice(TEXT("Limit FPS"),{TEXT("Bez limitu"),TEXT("30"),TEXT("60"),TEXT("90"),TEXT("120"),TEXT("144"),TEXT("165"),TEXT("240")},FMath::Max(0,Limit),[Limits](int32 I){UWTGPerformanceSettings::Get()->SetFPSLimit(Limits[I]);})];
        const TArray<FString> Quality={TEXT("Niska"),TEXT("Średnia"),TEXT("Wysoka"),TEXT("Ultra")};
        auto Apply=[S]{S->ApplyNonResolutionSettings();S->SaveSettings();};
        auto Presets=Quality;Presets.Add(TEXT("Własne ustawienia"));
        Form->AddSlot().AutoHeight()[Choice(TEXT("Preset jakości"),Presets,S->GetOverallScalabilityLevel()<0?4:S->GetOverallScalabilityLevel(),[this,S,Apply](int32 I){if(I<4){S->SetOverallScalabilityLevel(I);Apply();Refresh();}})];
        Form->AddSlot().AutoHeight()[Choice(TEXT("Tekstury"),Quality,S->GetTextureQuality(),[S,Apply](int32 I){S->SetTextureQuality(I);Apply();})];
        Form->AddSlot().AutoHeight()[Choice(TEXT("Cienie"),Quality,S->GetShadowQuality(),[S,Apply](int32 I){S->SetShadowQuality(I);Apply();})];
        Form->AddSlot().AutoHeight()[Choice(TEXT("Antyaliasing"),Quality,S->GetAntiAliasingQuality(),[S,Apply](int32 I){S->SetAntiAliasingQuality(I);Apply();})];
        Form->AddSlot().AutoHeight()[Choice(TEXT("Odległość renderowania"),Quality,S->GetViewDistanceQuality(),[S,Apply](int32 I){S->SetViewDistanceQuality(I);Apply();})];
        Form->AddSlot().AutoHeight()[WTGMenu::Text(TEXT("Zmiany jakości są zapisywane od razu. Nowy tryb ekranu wymaga potwierdzenia w ciągu 15 sekund."),18,WTGMenu::Muted)];
    }else if(SettingsCategory==1){
        auto* A=UWTGAudioSettings::Get();
        Form->AddSlot().AutoHeight()[ValueSlider(TEXT("Głośność główna"),TAttribute<float>::CreateLambda([A]{return A->MasterVolume;}),[A](float V){A->SetMasterVolume(V);})];
        Form->AddSlot().AutoHeight()[ValueSlider(TEXT("Muzyka"),TAttribute<float>::CreateLambda([A]{return A->MusicVolume;}),[A](float V){A->SetMusicVolume(V);})];
        Form->AddSlot().AutoHeight()[ValueSlider(TEXT("Efekty"),TAttribute<float>::CreateLambda([A]{return A->SFXVolume;}),[A](float V){A->SetSFXVolume(V);})];
        Form->AddSlot().AutoHeight()[ValueSlider(TEXT("Otoczenie"),TAttribute<float>::CreateLambda([A]{return A->AmbientVolume;}),[A](float V){A->SetAmbientVolume(V);})];
        Form->AddSlot().AutoHeight()[ValueSlider(TEXT("Interfejs"),TAttribute<float>::CreateLambda([A]{return A->UIVolume;}),[A](float V){A->SetUIVolume(V);})];
    }else{
        auto* P=UWTGPerformanceSettings::Get();
        Form->AddSlot().AutoHeight()[ValueToggle(TEXT("Ogranicz animacje"),TAttribute<bool>::CreateLambda([P]{return P->bReduceUIMotion;}),[P](bool V){P->SetReduceUIMotion(V);})];
        Form->AddSlot().AutoHeight()[ValueToggle(TEXT("Rozmycie tła"),TAttribute<bool>::CreateLambda([P]{return P->bMenuBackgroundBlur;}),[P](bool V){P->SetMenuBackgroundBlur(V);})];
        Form->AddSlot().AutoHeight()[ValueToggle(TEXT("Dźwięki nawigacji"),TAttribute<bool>::CreateLambda([P]{return P->bUISounds;}),[P](bool V){P->SetUISounds(V);})];
        Form->AddSlot().AutoHeight()[ValueToggle(TEXT("Licznik FPS"),TAttribute<bool>::CreateLambda([P]{return P->bShowFPS;}),[P](bool V){P->SetShowFPS(V);})];
    }
}
void SMenuShell::BuildControls()
{
    auto* I=Flow->GetGameInstance()->GetSubsystem<UWTGInputProfileSubsystem>();
    Content->AddSlot().AutoHeight()[WTGMenu::Text(TEXT("Sterowanie"),38,WTGMenu::Paper,true)];
    auto Form=SNew(SVerticalBox);Content->AddSlot().AutoHeight().Padding(0,18)[WTGMenu::Card(Form)];
    Form->AddSlot().AutoHeight()[ValueSlider(TEXT("Czułość myszy (0,1–5)"),TAttribute<float>::CreateLambda([I]{return (I->Profile.MouseSensitivity-.1f)/4.9f;}),[I](float V){I->SetMouseSensitivity(.1f+4.9f*V);})];
    Form->AddSlot().AutoHeight()[ValueToggle(TEXT("Odwróć oś Y"),TAttribute<bool>::CreateLambda([I]{return I->Profile.bInvertMouseY;}),[I](bool V){I->Profile.bInvertMouseY=V;I->SaveProfile();})];
    Form->AddSlot().AutoHeight()[ValueSlider(TEXT("Czułość kontrolera (0,1–5)"),TAttribute<float>::CreateLambda([I]{return (I->Profile.GamepadSensitivity-.1f)/4.9f;}),[I](float V){I->SetGamepadSensitivity(.1f+4.9f*V);})];
    Form->AddSlot().AutoHeight()[ValueSlider(TEXT("Martwa strefa kontrolera (0–0,5)"),TAttribute<float>::CreateLambda([I]{return I->Profile.GamepadDeadzone*2;}),[I](float V){I->SetGamepadDeadzone(V*.5f);})];
    Form->AddSlot().AutoHeight()[WTGMenu::Text(TEXT("Pad: lewy drążek — ruch / skręt; prawy — kamera; RT/LT — gaz / hamowanie. Stałe: ESC / START — pauza; F5/F9 — zapis/odczyt; ENTER, N, L, Q — skróty ekranów kampanii."),18,WTGMenu::Muted)];
    Form->AddSlot().AutoHeight()[Action(TEXT("Przywróć domyślne sterowanie"),[this,I]{Confirm(TEXT("Przywrócić sterowanie?"),TEXT("Własne przypisania i czułość zostaną zastąpione."),[this,I]{I->ResetToDefaults();Refresh();});})];
    if(!CapturingBinding.IsNone())Form->AddSlot().AutoHeight()[WTGMenu::Text(TEXT("Naciśnij nowy klawisz / przycisk. ESC / START: anuluj."),24,WTGMenu::Accent,true)];
    for(const auto& B:I->Bindings()){
        Form->AddSlot().AutoHeight().Padding(0,10,0,0)[WTGMenu::Text(B.Label,20,WTGMenu::Muted)];
        auto Row=SNew(SHorizontalBox);
        Row->AddSlot().FillWidth(1)[Action(I->DisplayKey(I->Key(B.Id)),[this,Id=B.Id]{CapturingBinding=Id;CapturingGamepad=false;Refresh();})];
        if(B.Gamepad.IsValid())Row->AddSlot().FillWidth(1).Padding(8,0)[Action(I->DisplayKey(I->Key(B.Id,true)),[this,Id=B.Id]{CapturingBinding=Id;CapturingGamepad=true;Refresh();})];
        Form->AddSlot().AutoHeight()[Row];
    }
}
void SMenuShell::BeginVideoChange()
{
    if(VideoPending)return;auto* S=GEngine->GetGameUserSettings();S->ConfirmVideoMode();
    S->SetScreenResolution(PendingResolution);S->SetFullscreenMode(EWindowMode::Type(PendingWindowMode));S->ApplyResolutionSettings(false);
    VideoPending=true;VideoConfirmDeadline=FPlatformTime::Seconds()+15;
    Confirm(TEXT("Zachować tryb ekranu?"),TEXT("Potwierdź w ciągu 15 sekund. Brak odpowiedzi przywróci poprzedni tryb."),[this,S]{S->ConfirmVideoMode();S->SaveSettings();VideoPending=false;Refresh();});
}
void SMenuShell::RevertVideoChange()
{
    if(!VideoPending)return;auto* S=GEngine->GetGameUserSettings();S->RevertVideoMode();S->ApplyResolutionSettings(false);VideoPending=false;
}
void SMenuShell::PreviewVideoMode(FIntPoint Resolution,int32 Mode){PendingResolution=Resolution;PendingWindowMode=Mode;BeginVideoChange();}
void SMenuShell::Tick(const FGeometry& G,double T,float Dt)
{
    SCompoundWidget::Tick(G,T,Dt);
    EntranceSeconds+=Dt;
    const float Alpha=UWTGPerformanceSettings::Get()->bReduceUIMotion?1.f:1-FMath::Pow(1-FMath::Clamp(EntranceSeconds/.2f,0.f,1.f),3.f);
    if(Stage){Stage->SetRenderOpacity(Alpha*(Flow.IsValid()?Flow->TransitionOpacity:1.f));Stage->SetRenderTransform(FSlateRenderTransform(FVector2D((1-Alpha)*22,0)));}
    if(VideoPending && FPlatformTime::Seconds()>VideoConfirmDeadline){RevertVideoChange();Refresh();}
}
