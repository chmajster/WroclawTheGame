#include "UI/Menu/MenuShell.h"
#include "UI/Menu/MenuStyle.h"
#include "UI/Menu/MenuFlowSubsystem.h"
#include "UI/SliceController.h"
#include "Audio/SliceAudio.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
using namespace WTGMenu;

void SMenuShell::Construct(const FArguments& Args){Flow=Args._Flow;Refresh();}
TSharedRef<SWidget> SMenuShell::Action(const FString& Label,TFunction<void()> Click,bool Enabled,const FString& Hint)
{
    auto B=SNew(SWTGMenuButton).Label(Label).Hint(Hint).OnClicked_Lambda([this,Click]{
        if(Flow.IsValid())USliceAudio::PlayUI(Flow.Get(),TEXT("Switch"),.3f);Click();return FReply::Handled();});
    B->SetEnabled(Enabled);if(Enabled)FocusOrder.Add(B->FocusTarget());return B;
}
void SMenuShell::FocusFirst()
{
    Focused=0;
    RegisterActiveTimer(0,FWidgetActiveTimerDelegate::CreateLambda([this](double,float){
        if(!FocusOrder.IsEmpty())FSlateApplication::Get().SetKeyboardFocus(FocusOrder[0],EFocusCause::SetDirectly);
        return EActiveTimerReturnType::Stop;
    }));
}
void SMenuShell::SelectPage(int32 Index){Page=Index;Refresh();}
void SMenuShell::Refresh()
{
    FocusOrder.Reset();Pending=nullptr;DialogTitle.Empty();
    if(!Flow.IsValid())return;
    if(Flow->Screen==EWTGMenuScreen::Boot){
        ChildSlot[Card(SNew(SVerticalBox)+SVerticalBox::Slot().VAlign(VAlign_Center)
            [Text(TEXT("WROCŁAW THE GAME\n\nPrzygotowywanie świata…"),42,Paper,true)],FMargin(100))];return;
    }
    ChildSlot[SAssignNew(Layers,SOverlay)
        +SOverlay::Slot()[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(FLinearColor(0,0,0,Flow->IsFrontEnd()?.28f:.76f))]
        +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
            [SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
                [SNew(SBox).WidthOverride(1920).HeightOverride(1080)
                    [SNew(SVerticalBox)
                        +SVerticalBox::Slot().AutoHeight().Padding(100,60,100,12)
                            [Text(Flow->IsFrontEnd()?TEXT("W R O C Ł A W"):TEXT("WROCŁAW THE GAME"),Flow->IsFrontEnd()?62:32,Paper,true)]
                        +SVerticalBox::Slot().AutoHeight().Padding(100,0,100,45)
                            [Text(Flow->IsFrontEnd()?TEXT("T H E  G A M E  /  NADODRZE"):TEXT("PAUZA  /  NADODRZE"),18,Accent,true)]
                        +SVerticalBox::Slot().FillHeight(1).Padding(100,0,100,36)
                            [SNew(SHorizontalBox)
                                +SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(430)
                                    [Card(SAssignNew(Navigation,SVerticalBox),FMargin(16,24))]]
                                +SHorizontalBox::Slot().FillWidth(1).Padding(40,0,0,0)
                                    [SNew(SScrollBox)+SScrollBox::Slot()[SAssignNew(Content,SVerticalBox)]]]
                        +SVerticalBox::Slot().AutoHeight().Padding(100,0,100,50)
                            [Text(TEXT("↑ ↓  NAWIGACJA     ENTER / A  WYBIERZ     ESC / B  WSTECZ"),16,Paper)]
                    ]]]];
    if(Flow->IsFrontEnd())BuildMain();else{
        Navigation->AddSlot().AutoHeight()[Action(TEXT("GRA"),[this]{SelectPage(0);})];
        Navigation->AddSlot().AutoHeight()[Action(TEXT("USTAWIENIA"),[this]{SelectPage(2);})];
        Navigation->AddSlot().AutoHeight()[Action(TEXT("STEROWANIE"),[this]{SelectPage(3);})];
    }
    switch(Page){case 0:if(!Flow->IsFrontEnd())BuildGame();break;case 1:BuildSaves();break;case 2:BuildSettings();break;case 3:BuildControls();break;case 4:BuildAbout();break;}
    Content->AddSlot().AutoHeight().Padding(0,22)[Text(Flow->Notice,18,Accent)];
    FocusFirst();
}
void SMenuShell::BuildMain()
{
    const auto Slots=Flow->SaveSlots();bool Valid=false;if(!Slots.IsEmpty())Flow->DescribeSave(Slots[0],Valid);
    Navigation->AddSlot().AutoHeight()[Action(TEXT("Kontynuuj"),[this,Slots]{
        auto Load=[this,Slots]{if(!Flow->Load(Slots[0]))Refresh();};
        if(Flow->bHasSession)Confirm(TEXT("Wczytać ostatni zapis?"),TEXT("Niezapisany postęp bieżącej sesji zostanie utracony."),Load);else Load();
    },Valid,TEXT("Ostatni poprawny zapis dzielnicy"))];
    Navigation->AddSlot().AutoHeight()[Action(TEXT("Nowa gra"),[this]{Confirm(TEXT("Rozpocząć nową grę?"),TEXT("Bieżący zapis Nadodrza zostanie zastąpiony. Postać zachowa wybrany wygląd."),[this]{if(!Flow->StartNew())Refresh();});})];
    Navigation->AddSlot().AutoHeight()[Action(TEXT("Wczytaj grę"),[this]{SelectPage(1);})];
    Navigation->AddSlot().AutoHeight()[Action(TEXT("Ustawienia"),[this]{SelectPage(2);})];
    Navigation->AddSlot().AutoHeight()[Action(TEXT("Sterowanie"),[this]{SelectPage(3);})];
    Navigation->AddSlot().AutoHeight()[Action(TEXT("O grze"),[this]{SelectPage(4);})];
    Navigation->AddSlot().AutoHeight()[Action(TEXT("Wyjście"),[this]{Confirm(TEXT("Wyjść z gry?"),TEXT("Niezapisany postęp nie zostanie zachowany."),[this]{Flow->Quit();});})];
    if(Page==0){
        Content->AddSlot().FillHeight(1).VAlign(VAlign_Bottom)
            [SNew(SBox).WidthOverride(630).HAlign(HAlign_Left)
                [Card(SNew(SVerticalBox)
                    +SVerticalBox::Slot().AutoHeight()[Text(TEXT("01  /  NADODRZE"),18,Accent,true)]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,20)[Text(TEXT("Miasto ma\nswoje historie."),48,Paper,true)]
                    +SVerticalBox::Slot().AutoHeight()[Text(TEXT("Kilka ulic, jedna przesyłka i droga nad Odrę.\nTwoja pierwsza historia zaczyna się przy pracowni Mai."),22,Paper)])]];
    }
}
void SMenuShell::BuildGame()
{
    Content->AddSlot().AutoHeight()[Text(TEXT("Twoja rozgrywka"),38,Paper,true)];
    Content->AddSlot().AutoHeight().Padding(0,24)[Action(TEXT("Wznów"),[this]{Flow->Resume();})];
    Content->AddSlot().AutoHeight()[Action(TEXT("Zapisz grę"),[this]{Flow->Save();Refresh();})];
    Content->AddSlot().AutoHeight()[Action(TEXT("Wczytaj zapis"),[this]{SelectPage(1);})];
    Content->AddSlot().AutoHeight()[Action(TEXT("Rozpocznij od nowa"),[this]{Confirm(TEXT("Rozpocząć od nowa?"),TEXT("Nadpiszesz bieżący zapis i postęp misji."),[this]{if(!Flow->StartNew())Refresh();});})];
    Content->AddSlot().AutoHeight()[Action(TEXT("Menu główne"),[this]{Confirm(TEXT("Wrócić do menu głównego?"),TEXT("Zapisz grę, aby zachować ostatni postęp."),[this]{Page=0;Flow->ShowMain();});})];
    Content->AddSlot().AutoHeight()[Action(TEXT("Wyjdź z gry"),[this]{Confirm(TEXT("Wyjść z gry?"),TEXT("Niezapisany postęp zostanie utracony."),[this]{Flow->Quit();});})];
}
void SMenuShell::BuildSaves()
{
    Content->AddSlot().AutoHeight()[Text(TEXT("Zapisane historie"),38,Paper,true)];
    auto Slots=Flow->SaveSlots();if(Slots.IsEmpty())Content->AddSlot().AutoHeight().Padding(0,30)[Text(TEXT("Nie masz jeszcze zapisanej gry."),22,Muted)];
    for(const auto& Slot:Slots){bool Valid=false;const FString Description=Flow->DescribeSave(Slot,Valid);
        Content->AddSlot().AutoHeight().Padding(0,14)[Card(SNew(SVerticalBox)
            +SVerticalBox::Slot().AutoHeight()[Text(Description,20,Valid?Paper:Muted)]
            +SVerticalBox::Slot().AutoHeight().Padding(0,16,0,0)[Action(TEXT("Wczytaj"),[this,Slot]{Confirm(TEXT("Wczytać zapis?"),TEXT("Niezapisany postęp bieżącej sesji zostanie utracony."),[this,Slot]{if(!Flow->Load(Slot))Refresh();});},Valid)])];
    }
}
void SMenuShell::BuildSettings()
{
    Content->AddSlot().AutoHeight()[Text(TEXT("Ustawienia obrazu"),38,Paper,true)];
    for(int32 Q=0;Q<4;++Q){const TCHAR* Labels[]={TEXT("Niska"),TEXT("Średnia"),TEXT("Wysoka"),TEXT("Ultra")};
        Content->AddSlot().AutoHeight().Padding(0,10)[Action(Labels[Q],[Q]{auto* S=GEngine->GetGameUserSettings();S->SetOverallScalabilityLevel(Q);S->ApplyNonResolutionSettings();S->SaveSettings();})];}
}
void SMenuShell::BuildControls()
{
    Content->AddSlot().AutoHeight()[Text(TEXT("Sterowanie"),38,Paper,true)];
    Content->AddSlot().AutoHeight().Padding(0,30)[Card(Text(TEXT("WASD — ruch / jazda\nMysz — kamera\nE — interakcja / wejście / wyjście\nShift — sprint\nCtrl — kucanie\nSpacja — skok / hamulec ręczny\nF5 / F9 — zapis / odczyt\nEsc — pauza"),24))];
}
void SMenuShell::BuildAbout()
{
    Content->AddSlot().AutoHeight()[Text(TEXT("Wrocław The Game"),38,Paper,true)];
    Content->AddSlot().AutoHeight().Padding(0,28)[Card(Text(TEXT("Autorska gra akcji osadzona we Wrocławiu.\nProjekt: chmajster / WroclawTheGame.\n\nŚwiat: © OpenStreetMap contributors (ODbL), Copernicus EU-DEM / USGS przez Mapzen.\nModele i tekstury: lokalny katalog CC0; autorzy i licencje w dokumentacji zasobów projektu.\n\nInterfejs jest oryginalny. Nie używa grafik, logotypów ani plików Grand Theft Auto.\n\nWersja rozwojowa — Nadodrze / Odra."),24))];
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
        [SNew(SBox).WidthOverride(650)
            [Card(SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight()[Text(DialogTitle,32,Paper,true)]
                +SVerticalBox::Slot().AutoHeight().Padding(0,20,0,30)[Text(DialogBody,22,Muted)]
                +SVerticalBox::Slot().AutoHeight()[Action(TEXT("Anuluj"),[this]{Refresh();})]
                +SVerticalBox::Slot().AutoHeight()[Action(TEXT("Potwierdź"),[this]{auto A=MoveTemp(Pending);if(A)A();})])]]];
    FocusFirst();
}
FReply SMenuShell::OnPreviewKeyDown(const FGeometry& G,const FKeyEvent& E)
{
    const FKey K=E.GetKey();
    if(K==EKeys::Escape || K==EKeys::Gamepad_FaceButton_Right){
        if(Pending)Refresh();else if(Page!=0)SelectPage(0);else if(Flow.IsValid() && !Flow->IsFrontEnd())Flow->Resume();return FReply::Handled();}
    const int Step=(K==EKeys::Up || K==EKeys::Gamepad_DPad_Up)?-1:(K==EKeys::Down || K==EKeys::Gamepad_DPad_Down)?1:0;
    if((Step || K==EKeys::Tab) && !FocusOrder.IsEmpty()){
        for(int32 I=0;I<FocusOrder.Num();++I)if(FocusOrder[I]->HasKeyboardFocus())Focused=I;
        const int Delta=Step?Step:(E.IsShiftDown()?-1:1);
        Focused=(Focused+Delta+FocusOrder.Num())%FocusOrder.Num();FSlateApplication::Get().SetKeyboardFocus(FocusOrder[Focused],EFocusCause::Navigation);return FReply::Handled();}
    return FReply::Unhandled();
}
