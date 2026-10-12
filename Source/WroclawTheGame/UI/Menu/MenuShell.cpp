#include "UI/Menu/MenuShell.h"
#include "UI/Menu/MenuStyle.h"
#include "UI/Menu/MenuMap.h"
#include "UI/WTGInputProfileSubsystem.h"
#include "Character/SliceCharacter.h"
#include "Character/CharacterCreatorSubsystem.h"
#include "Components/GameplayComponents.h"
#include "Vehicles/DriveableVehicle.h"
#include "Systems/DistrictGameplaySubsystem.h"
#include "Systems/WTGEconomySubsystem.h"
#include "Mission/DistrictDeliverySubsystem.h"
#include "Mission/SliceMission.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UI/Menu/MenuFlowSubsystem.h"
#include "UI/SliceController.h"
#include "Audio/SliceAudio.h"
#include "UI/PerformanceSettings.h"
#include "UI/Menu/MenuLoadingScreen.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"


void SMenuShell::Construct(const FArguments& Args){Flow=Args._Flow;Page=Flow.IsValid() && !Flow->IsFrontEnd()?10:0;Refresh();}
SMenuShell::~SMenuShell(){RevertVideoChange();}
void SMenuShell::NavigationSound(float Volume)
{
    const double Now=FPlatformTime::Seconds();
    if(Flow.IsValid() && UWTGPerformanceSettings::Get()->bUISounds && Now-LastNavigationSound>.08){LastNavigationSound=Now;USliceAudio::PlayUI(Flow.Get(),TEXT("Switch"),Volume);}
}
TSharedRef<SWidget> SMenuShell::Action(const FString& Label,TFunction<void()> Click,bool Enabled,const FString& Hint)
{
    auto B=SNew(SWTGMenuButton).Label(Label).Hint(Hint).OnClicked_Lambda([this,Click,Label]{
        if(Flow.IsValid() && !Flow->IsTransitioning() && (VideoApplyDeadline==0 || Label==TEXT("Anuluj"))){NavigationSound(.25f);Click();}return FReply::Handled();})
        .OnHovered_Lambda([this]{NavigationSound(.09f);})
        .Selected_Lambda([this,Label]{return Flow.IsValid() && Flow->IsFrontEnd() && ((Page==1 && Label==TEXT("Wczytaj grę")) || (Page==2 && Label==TEXT("Ustawienia")) || (Page==3 && Label==TEXT("Sterowanie")) || (Page==4 && Label==TEXT("O grze")));});
    B->SetEnabled(Enabled);if(Enabled)FocusOrder.Add(B->FocusTarget());return B;
}
void SMenuShell::FocusFirst(int32 Preferred)
{
    Focused=Preferred;
    if(!FocusOrder.IsEmpty() && FSlateApplication::Get().SetKeyboardFocus(FocusOrder[FMath::Clamp(Preferred,0,FocusOrder.Num()-1)],EFocusCause::SetDirectly))return;
    RegisterActiveTimer(0,FWidgetActiveTimerDelegate::CreateLambda([this,Preferred](double,float){
        if(!FocusOrder.IsEmpty()){Focused=FMath::Clamp(Preferred,0,FocusOrder.Num()-1);FSlateApplication::Get().SetKeyboardFocus(FocusOrder[Focused],EFocusCause::SetDirectly);}
        return EActiveTimerReturnType::Stop;
    }));
}
void SMenuShell::SelectPage(int32 Index){Page=Index;Refresh();}
void SMenuShell::Refresh(bool PreserveFocus)
{
    int32 PreviousFocus=Focused;
    for(int32 I=0;I<FocusOrder.Num();++I)if(FocusOrder[I]->HasKeyboardFocus() || FocusOrder[I]->HasFocusedDescendants())PreviousFocus=I;
    FocusOrder.Reset();MapView.Reset();Pending=nullptr;DialogTitle.Empty();EntranceSeconds=0;
    if(!Flow.IsValid())return;
    if(Flow->Screen==EWTGMenuScreen::Boot){
        ChildSlot[WTGMenu::LoadingScreen()];return;
    }
    TSharedPtr<SHorizontalBox> Body,Tabs;
    ChildSlot[SAssignNew(Layers,SOverlay)
        +SOverlay::Slot()[SNew(SBackgroundBlur).Clipping(EWidgetClipping::ClipToBoundsAlways).BlurStrength_Lambda([this]{
            if(!Flow.IsValid() || FPlatformTime::Seconds()<BlurSuspendedUntil || !UWTGPerformanceSettings::Get()->bMenuBackgroundBlur)return 0.f;
            auto* Viewport=Flow->MenuWorld()->GetGameViewport();if(!Viewport || !Viewport->Viewport)return 0.f;
            const auto Pixels=Viewport->Viewport->GetSizeXY();const auto Size=GetCachedGeometry().GetAbsoluteSize();
            return Size.X<=Pixels.X+.5 && Size.Y<=Pixels.Y+.5?3.f:0.f;
        })]
        +SOverlay::Slot()[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(FLinearColor(0,0,0,Flow->IsFrontEnd()?.28f:.76f))]
        +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
            [SAssignNew(Stage,SScaleBox).Stretch(EStretch::ScaleToFit)
                [SNew(SBox).WidthOverride(1920).HeightOverride(1080)
                    [SNew(SVerticalBox)
                        +SVerticalBox::Slot().AutoHeight().Padding(100,60,100,12)
                            [WTGMenu::Text(Flow->IsFrontEnd()?TEXT("W R O C \u0141 A W"):TEXT("WROC\u0141AW THE GAME"),Flow->IsFrontEnd()?62:32,WTGMenu::Paper,true)]
                        +SVerticalBox::Slot().AutoHeight().Padding(100,0,100,30)
                            [WTGMenu::Text(Flow->IsFrontEnd()?TEXT("T H E  G A M E  /  NADODRZE"):TEXT("PAUZA  /  NADODRZE"),18,WTGMenu::Accent,true)]
                        +SVerticalBox::Slot().AutoHeight().Padding(100,0,100,22)[SAssignNew(Tabs,SHorizontalBox)]
                        +SVerticalBox::Slot().FillHeight(1).Padding(100,0,100,36)[SAssignNew(Body,SHorizontalBox)]
                        +SVerticalBox::Slot().AutoHeight().Padding(100,0,100,50)
                            [WTGMenu::Text(TEXT("TAB  /  \u2191 \u2193  NAWIGACJA     ENTER / A  WYBIERZ     ESC / B  WSTECZ"),16,WTGMenu::Paper)]
                    ]]]];
    if(Flow->IsFrontEnd())Body->AddSlot().AutoWidth().VAlign(VAlign_Top)[SNew(SBox).WidthOverride(430)[WTGMenu::Card(SAssignNew(Navigation,SVerticalBox),FMargin(16,24))]];
    Body->AddSlot().FillWidth(1).Padding(Flow->IsFrontEnd()?FMargin(40,0,0,0):FMargin(0))
        [Flow->IsFrontEnd() && Page==0?StaticCastSharedRef<SWidget>(SAssignNew(Content,SVerticalBox)):
            StaticCastSharedRef<SWidget>(SNew(SScrollBox).ScrollWhenFocusChanges(EScrollWhenFocusChanges::AnimatedScroll)+SScrollBox::Slot()[SAssignNew(Content,SVerticalBox)])];
    if(Flow->IsFrontEnd())BuildMain();else{
        const TCHAR* Labels[]={TEXT("MAPA"),TEXT("GRA"),TEXT("POSTA\u0106"),TEXT("STATYSTYKI"),TEXT("USTAWIENIA"),TEXT("STEROWANIE")};
        const int32 Pages[]={10,0,11,12,2,3};
        for(int32 I=0;I<6;++I){const int32 Target=Pages[I];
            auto Button=SNew(SWTGMenuButton).Label(Labels[I]).Selected(Page==Target)
                .OnHovered_Lambda([this]{NavigationSound(.09f);})
                .OnClicked_Lambda([this,Target]{if(!Flow->IsTransitioning()){NavigationSound(.25f);SelectPage(Target);}return FReply::Handled();});
            FocusOrder.Add(Button->FocusTarget());Tabs->AddSlot().FillWidth(1).Padding(0,0,5,0)[Button];}
    }
    const int32 FirstPageControl=FocusOrder.Num();
    switch(Page){case 0:if(!Flow->IsFrontEnd())BuildGame();break;case 1:BuildSaves();break;case 2:BuildSettings();break;case 3:BuildControls();break;case 4:BuildAbout();break;case 10:BuildMap();break;case 11:BuildCharacter();break;case 12:BuildStats();break;}
    if(!Flow->Notice.IsEmpty())Content->AddSlot().AutoHeight().Padding(0,22)[WTGMenu::Card(WTGMenu::Text(Flow->Notice,18,WTGMenu::Accent),FMargin(18))];
    const int32 PausePages[]={10,0,11,12,2,3};int32 Preferred=0;
    if(Flow->IsFrontEnd()){if(Page!=0)Preferred=FirstPageControl;}else for(int32 I=0;I<6;++I)if(PausePages[I]==Page)Preferred=I;
    FocusFirst(PreserveFocus?PreviousFocus:Preferred);
}
void SMenuShell::BuildMain()
{
    const auto SavedFiles=Flow->SaveSlots();FString Latest;
    for(const auto& Slot:SavedFiles){bool Valid=false;Flow->DescribeSave(Slot,Valid);if(Valid){Latest=Slot;break;}}
    Navigation->AddSlot().AutoHeight()[Action(TEXT("Kontynuuj"),[this,Latest]{
        auto Load=[this,Latest]{if(!Flow->Load(Latest))Refresh();};
        if(Flow->bHasSession)Confirm(TEXT("Wczytać ostatni zapis?"),TEXT("Niezapisany postęp bieżącej sesji zostanie utracony."),Load);else Load();
    },!Latest.IsEmpty(),TEXT("Ostatni poprawny zapis dzielnicy"))];
    Navigation->AddSlot().AutoHeight()[Action(TEXT("Nowa gra"),[this]{Confirm(TEXT("Rozpocząć nową grę?"),TEXT("Bieżący zapis Nadodrza zostanie zastąpiony. Postać zachowa wybrany wygląd."),[this]{if(!Flow->StartNew())Refresh();});})];
    Navigation->AddSlot().AutoHeight()[Action(TEXT("Wczytaj grę"),[this]{SelectPage(1);})];
    Navigation->AddSlot().AutoHeight()[Action(TEXT("Ustawienia"),[this]{SelectPage(2);})];
    Navigation->AddSlot().AutoHeight()[Action(TEXT("Sterowanie"),[this]{SelectPage(3);})];
    Navigation->AddSlot().AutoHeight()[Action(TEXT("O grze"),[this]{SelectPage(4);})];
    Navigation->AddSlot().AutoHeight()[Action(TEXT("Wyjście"),[this]{Confirm(TEXT("Wyjść z gry?"),TEXT("Niezapisany postęp nie zostanie zachowany."),[this]{Flow->Quit();});})];
    if(Page==0){
        Content->AddSlot().FillHeight(1).VAlign(VAlign_Bottom).HAlign(HAlign_Left)
            [SNew(SBox).WidthOverride(630).HAlign(HAlign_Left)
                [WTGMenu::Card(SNew(SVerticalBox)
                    +SVerticalBox::Slot().AutoHeight()[WTGMenu::Text(TEXT("01  /  NADODRZE"),18,WTGMenu::Accent,true)]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,20)[WTGMenu::Text(TEXT("Miasto ma\nswoje historie."),48,WTGMenu::Paper,true)]
                    +SVerticalBox::Slot().AutoHeight()[WTGMenu::Text(TEXT("Kilka ulic, jedna przesyłka i droga nad Odrę.\nTwoja pierwsza historia zaczyna się przy pracowni Mai."),22,WTGMenu::Paper)])]];
    }
}
void SMenuShell::BuildGame()
{
    Content->AddSlot().AutoHeight()[WTGMenu::Text(TEXT("Twoja rozgrywka"),38,WTGMenu::Paper,true)];
    Content->AddSlot().AutoHeight().Padding(0,24)[Action(TEXT("Wznów"),[this]{Flow->Resume();})];
    Content->AddSlot().AutoHeight()[Action(TEXT("Zapisz grę"),[this]{Flow->Save();Refresh();})];
    Content->AddSlot().AutoHeight()[Action(TEXT("Wczytaj zapis"),[this]{SelectPage(1);})];
    Content->AddSlot().AutoHeight()[Action(TEXT("Rozpocznij od nowa"),[this]{Confirm(TEXT("Rozpocząć od nowa?"),TEXT("Nadpiszesz bieżący zapis i postęp misji."),[this]{if(!Flow->StartNew())Refresh();});})];
    Content->AddSlot().AutoHeight()[Action(TEXT("Menu główne"),[this]{Confirm(TEXT("Wrócić do menu głównego?"),TEXT("Zapisz grę, aby zachować ostatni postęp."),[this]{Page=0;Flow->ShowMain();});})];
    Content->AddSlot().AutoHeight()[Action(TEXT("Wyjdź z gry"),[this]{Confirm(TEXT("Wyjść z gry?"),TEXT("Niezapisany postęp zostanie utracony."),[this]{Flow->Quit();});})];
}
void SMenuShell::BuildSaves()
{
    Content->AddSlot().AutoHeight()[WTGMenu::Text(TEXT("Zapisane historie"),38,WTGMenu::Paper,true)];
    auto SavedFiles=Flow->SaveSlots();if(SavedFiles.IsEmpty())Content->AddSlot().AutoHeight().Padding(0,30)[WTGMenu::Text(TEXT("Nie masz jeszcze zapisanej gry."),22,WTGMenu::Muted)];
    for(const auto& Slot:SavedFiles){bool Valid=false;const FString Description=Flow->DescribeSave(Slot,Valid);
        Content->AddSlot().AutoHeight().Padding(0,14)[WTGMenu::Card(SNew(SVerticalBox)
            +SVerticalBox::Slot().AutoHeight()[WTGMenu::Text(Description,20,Valid?WTGMenu::Paper:WTGMenu::Muted)]
            +SVerticalBox::Slot().AutoHeight().Padding(0,16,0,0)[Action(TEXT("Wczytaj"),[this,Slot]{Confirm(TEXT("Wczytać zapis?"),TEXT("Niezapisany postęp bieżącej sesji zostanie utracony."),[this,Slot]{if(!Flow->Load(Slot))Refresh();});},Valid)])];
    }
}
void SMenuShell::BuildAbout()
{
    Content->AddSlot().AutoHeight()[WTGMenu::Text(TEXT("Wrocław The Game"),38,WTGMenu::Paper,true)];
    Content->AddSlot().AutoHeight().Padding(0,28)[WTGMenu::Card(WTGMenu::Text(TEXT("Autorska gra akcji osadzona we Wrocławiu.\nProjekt: chmajster / WroclawTheGame.\n\nŚwiat: © OpenStreetMap contributors (ODbL), Copernicus EU-DEM / USGS przez Mapzen.\nModele i tekstury: lokalny katalog CC0; autorzy i licencje w dokumentacji zasobów projektu.\n\nInterfejs jest oryginalny. Nie używa grafik, logotypów ani plików Grand Theft Auto.\n\nWersja rozwojowa — Nadodrze / Odra."),24))];
}
void SMenuShell::BuildMap()
{
    Content->AddSlot().AutoHeight()[WTGMenu::Text(TEXT("Nadodrze / Odra"),32,WTGMenu::Paper,true)];
    auto Map=SNew(SMenuMap).Flow(Flow.Get());MapView=Map;FocusOrder.Add(Map);
    Content->AddSlot().AutoHeight().Padding(0,18)[SNew(SBox).HeightOverride(520)[Map]];
    Content->AddSlot().AutoHeight()[WTGMenu::Text(TEXT("Przeciągnij mapę. Kółko / + −: zoom. Kliknij: znacznik. Strzałki / D-pad: przesuwanie. TAB: następna kontrolka."),18,WTGMenu::Muted)];
    auto Clear=Action(TEXT("Usuń własny znacznik"),[this]{Flow->MapWaypoint.Reset();Refresh();});
    Clear->SetEnabled(TAttribute<bool>::CreateLambda([Weak=Flow]{return Weak.IsValid() && Weak->MapWaypoint.IsSet();}));
    Content->AddSlot().AutoHeight().Padding(0,8)[Clear];
}
void SMenuShell::BuildCharacter()
{
    auto* W=Flow->MenuWorld();auto* P=Cast<ASliceCharacter>(Flow->Controller()->GetPawn());
    if(auto* V=Cast<ADriveableVehicle>(Flow->Controller()->GetPawn()))P=V->Driver;
    const auto& Appearance=W->GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>()->Committed.PlayerAppearanceData;
    Content->AddSlot().AutoHeight()[WTGMenu::Text(Appearance.Name,42,WTGMenu::Paper,true)];
    if(P)Content->AddSlot().AutoHeight().Padding(0,24)[WTGMenu::Card(WTGMenu::Text(FString::Printf(TEXT("Zdrowie   %.0f / %.0f\nWytrzymałość   %.0f / %.0f\nWzrost   %.0f cm\nLokalizacja   %s\nPortfel   %.2f zł"),
        P->HealthState->Value,P->HealthState->Maximum,P->StaminaState->Value,P->StaminaState->Maximum,Appearance.Height,
        W->GetSubsystem<UDistrictGameplaySubsystem>()->IsActive()?TEXT("Nadodrze / Odra"):TEXT("Bieżąca mapa"),W->GetGameInstance()->GetSubsystem<UWTGEconomySubsystem>()->BalanceCents/100.0),26))];
    Content->AddSlot().AutoHeight().Padding(0,10)[WTGMenu::Text(W->GetGameInstance()->GetSubsystem<USliceMission>()->InventoryText(),20,WTGMenu::Muted)];
    Content->AddSlot().AutoHeight()[Action(TEXT("Nowa postać i rozgrywka"),[this]{Confirm(TEXT("Utworzyć nową postać?"),TEXT("Po zatwierdzeniu kreatora rozpoczniesz nową grę. Zapis zostanie zastąpiony."),[this]{Flow->Controller()->NewGame();});})];
}
void SMenuShell::BuildStats()
{
    auto* W=Flow->MenuWorld();auto* D=W->GetSubsystem<UDistrictDeliverySubsystem>();
    Content->AddSlot().AutoHeight()[WTGMenu::Text(TEXT("Twoja historia w liczbach"),38,WTGMenu::Paper,true)];
    Content->AddSlot().AutoHeight().Padding(0,28)[WTGMenu::Card(WTGMenu::Text(FString::Printf(TEXT("Czas gry   %02d:%02d:%02d\nDystans pieszo   %.2f km\nDystans samochodem   %.2f km\nPróby dostawy   %u\nDostawa ukończona   %s\nTransakcje w portfelu   %d"),
        int(Flow->PlayedSeconds)/3600,int(Flow->PlayedSeconds)/60%60,int(Flow->PlayedSeconds)%60,Flow->WalkedCm/100000,Flow->DrivenCm/100000,
        D->State.Attempts,D->State.Stage==Wroclaw::DeliveryStage::Complete?TEXT("Tak"):TEXT("Nie"),W->GetGameInstance()->GetSubsystem<UWTGEconomySubsystem>()->Ledger.Num()),26))];
    Content->AddSlot().AutoHeight().Padding(0,14)[WTGMenu::Text(TEXT("Czas i dystans są mierzone od rozpoczęcia tej rozgrywki i przechowywane w jej zapisie. Starsze zapisy zaczynają te liczniki od zera."),18,WTGMenu::Muted)];
}
void SMenuShell::Confirm(const FString& Title,const FString& Body,TFunction<void()> A)
{
    DialogTitle=Title;DialogBody=Body;Pending=MoveTemp(A);DrawDialog();
}
void SMenuShell::DrawDialog()
{
    FocusOrder.Reset();
    Layers->AddSlot()[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor(0,0,0,.88f)).HAlign(HAlign_Center).VAlign(VAlign_Center)
        [SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1920).HeightOverride(1080).HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(SBox).WidthOverride(650)
            [WTGMenu::Card(SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight()[WTGMenu::Text(DialogTitle,32,WTGMenu::Paper,true)]
                +SVerticalBox::Slot().AutoHeight().Padding(0,20,0,30)[WTGMenu::Text(DialogBody,22,WTGMenu::Muted)]
                +SVerticalBox::Slot().AutoHeight()[Action(TEXT("Anuluj"),[this]{RevertVideoChange();Refresh();})]
                +SVerticalBox::Slot().AutoHeight()[SNew(SBox).IsEnabled_Lambda([this]{return !VideoPending || VideoApplyDeadline==0;})[Action(TEXT("Potwierdź"),[this]{auto A=MoveTemp(Pending);if(A)A();})]])]]]]];
    FocusFirst();
}
FReply SMenuShell::OnPreviewKeyDown(const FGeometry& G,const FKeyEvent& E)
{
    const FKey K=E.GetKey();
    if(Flow.IsValid() && Flow->IsTransitioning())return FReply::Handled();
    if(VideoApplyDeadline>0 && !VideoPending)return FReply::Handled();
    if(!CapturingBinding.IsNone()){
        if(K==EKeys::Escape || K==EKeys::Gamepad_Special_Right || (!CapturingGamepad && K==EKeys::Gamepad_FaceButton_Right)){CapturingBinding=NAME_None;Refresh(true);return FReply::Handled();}
        auto* Input=Flow->GetGameInstance()->GetSubsystem<UWTGInputProfileSubsystem>();
        const bool OK=CapturingGamepad?Input->RebindGamepad(CapturingBinding,K):Input->RebindKeyboard(CapturingBinding,K);
        if(OK){CapturingBinding=NAME_None;Flow->Notice=TEXT("Przypisanie zapisane.");Refresh(true);}
        else {Flow->Notice=TEXT("Ten klawisz jest zarezerwowany lub zaj\u0119ty w tym kontek\u015bcie.");Refresh(true);}
        return FReply::Handled();
    }
    if(K==EKeys::Escape || K==EKeys::Gamepad_FaceButton_Right || K==EKeys::Gamepad_Special_Right){
        if(VideoPending){RevertVideoChange();Refresh();}else if(Pending)Refresh();else if(Flow.IsValid() && !Flow->IsFrontEnd())Flow->Resume();else if(Page!=0)SelectPage(0);return FReply::Handled();}
    if(MapView && MapView->HasKeyboardFocus() && K!=EKeys::Tab)return FReply::Unhandled();
    const int Step=(K==EKeys::Up || K==EKeys::Gamepad_DPad_Up)?-1:(K==EKeys::Down || K==EKeys::Gamepad_DPad_Down)?1:0;
    if((Step || K==EKeys::Tab) && !FocusOrder.IsEmpty()){
        for(int32 I=0;I<FocusOrder.Num();++I)if(FocusOrder[I]->HasKeyboardFocus() || FocusOrder[I]->HasFocusedDescendants())Focused=I;
        const int Delta=Step?Step:(E.IsShiftDown()?-1:1);
        for(int32 I=0;I<FocusOrder.Num();++I){Focused=(Focused+Delta+FocusOrder.Num())%FocusOrder.Num();if(FocusOrder[Focused]->IsEnabled())break;}
        NavigationSound(.09f);FSlateApplication::Get().SetKeyboardFocus(FocusOrder[Focused],EFocusCause::Navigation);return FReply::Handled();}
    return FReply::Unhandled();
}
FReply SMenuShell::OnPreviewMouseButtonDown(const FGeometry&,const FPointerEvent& E)
{
    if(CapturingBinding.IsNone() || CapturingGamepad)return FReply::Unhandled();
    const bool OK=Flow->GetGameInstance()->GetSubsystem<UWTGInputProfileSubsystem>()->RebindKeyboard(CapturingBinding,E.GetEffectingButton());
    if(OK){CapturingBinding=NAME_None;Flow->Notice=TEXT("Przypisanie zapisane.");}
    else Flow->Notice=TEXT("Przycisk myszy jest już zajęty w tym kontekście.");Refresh(true);return FReply::Handled();
}
