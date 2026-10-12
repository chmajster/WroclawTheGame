#include "UI/Menu/MenuFlowSubsystem.h"
#include "UI/Menu/FrontEndMenuWidget.h"
#include "UI/SliceController.h"
#include "Mission/SliceMission.h"
#include "Save/DistrictSaveSubsystem.h"
#include "Save/DistrictSaveGame.h"
#include "Systems/DistrictGameplaySubsystem.h"
#include "Geography/DistrictRegistry.h"
#include "Character/SliceCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void UMenuFlowSubsystem::Initialize(FSubsystemCollectionBase& C)
{
    Super::Initialize(C);
    const bool Skip=FParse::Param(FCommandLine::Get(),TEXT("WTGSkipFrontEnd")) ||
        (FParse::Param(FCommandLine::Get(),TEXT("WTGNoDistrictAutoLoad")) && !FParse::Param(FCommandLine::Get(),TEXT("WTGMenuTest")));
    Screen=Skip?EWTGMenuScreen::Playing:EWTGMenuScreen::Boot;bHasSession=Skip;
    Ticker=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this,&UMenuFlowSubsystem::Tick));
}
void UMenuFlowSubsystem::Deinitialize()
{
    FTSTicker::GetCoreTicker().RemoveTicker(Ticker);if(Widget)Widget->RemoveFromParent();Widget=nullptr;
    if(Camera)Camera->Destroy();Camera=nullptr;PC.Reset();Super::Deinitialize();
}
UWorld* UMenuFlowSubsystem::MenuWorld() const { return PC.IsValid()?PC->GetWorld():nullptr; }
void UMenuFlowSubsystem::Attach(ASliceController* C)
{
    if(Widget)Widget->RemoveFromParent();Widget=nullptr;Camera=nullptr;PC=C;
    if(IsFrontEnd()){
        Screen=EWTGMenuScreen::Boot;
        C->SetIgnoreMoveInput(true);C->SetIgnoreLookInput(true);C->SetInputMode(FInputModeUIOnly());
        Widget=CreateWidget<UFrontEndMenuWidget>(C);Widget->AddToViewport(500);
    }
}
bool UMenuFlowSubsystem::Tick(float Dt)
{
    if(!PC.IsValid())return true;
    if(Screen==EWTGMenuScreen::Boot){
        auto* P=Cast<ASliceCharacter>(PC->GetPawn());
        if(P && P->GetCharacterMovement()->MovementMode==MOVE_Walking && MenuWorld()->GetRealTimeSeconds()>1)ShowMain();
    }
    return true;
}
void UMenuFlowSubsystem::Open(EWTGMenuScreen Target)
{
    if(!PC.IsValid())return;Screen=Target;
    GetGameInstance()->GetSubsystem<USliceMission>()->bShowMenu=true;
    PC->SetPause(true);PC->ResetIgnoreMoveInput();PC->ResetIgnoreLookInput();
    PC->SetIgnoreMoveInput(true);PC->SetIgnoreLookInput(true);
    if(auto* P=Cast<ASliceCharacter>(PC->GetPawn())){P->SetSprint(false);P->SetBlock(false);P->GetCharacterMovement()->StopMovementImmediately();}
    if(!Widget){Widget=CreateWidget<UFrontEndMenuWidget>(PC.Get());Widget->AddToViewport(500);}
    Widget->Refresh();FInputModeUIOnly Mode;Mode.SetWidgetToFocus(Widget->TakeWidget());PC->SetInputMode(Mode);PC->bShowMouseCursor=true;
    UE_LOG(LogTemp,Display,TEXT("WTG_MENU_SCREEN %s"),Target==EWTGMenuScreen::Main?TEXT("main"):TEXT("pause"));
}
void UMenuFlowSubsystem::ShowMain()
{
    if(!PC.IsValid())return;
    auto* R=MenuWorld()->GetSubsystem<UDistrictGameplaySubsystem>()->GetRegistry();
    if(!Camera){Camera=MenuWorld()->SpawnActor<ACameraActor>();Camera->GetCameraComponent()->SetFieldOfView(60);}
    if(R){const FVector P=R->PlayerStart+FVector(-1100,-900,650);Camera->SetActorLocationAndRotation(P,(R->MeetingPoint+FVector(500,1200,150)-P).Rotation());}
    PC->SetViewTarget(Camera);Open(EWTGMenuScreen::Main);
}
void UMenuFlowSubsystem::ShowPause(){Open(EWTGMenuScreen::Pause);}
void UMenuFlowSubsystem::Resume()
{
    if(!PC.IsValid() || !bHasSession)return;
    Screen=EWTGMenuScreen::Playing;GetGameInstance()->GetSubsystem<USliceMission>()->bShowMenu=false;
    if(Widget)Widget->RemoveFromParent();Widget=nullptr;
    PC->SetViewTarget(PC->GetPawn());PC->ResetIgnoreMoveInput();PC->ResetIgnoreLookInput();
    PC->SetPause(false);PC->SetInputMode(FInputModeGameOnly());PC->bShowMouseCursor=false;
}
bool UMenuFlowSubsystem::StartNew()
{
    if(!MenuWorld())return false;
    auto* D=MenuWorld()->GetSubsystem<UDistrictSaveSubsystem>();
    bool OK=MenuWorld()->GetSubsystem<UDistrictGameplaySubsystem>()->IsActive()?D->NewRun():false;
    if(OK){bHasSession=true;Resume();}else {PC->SetPause(true);Notice=TEXT("Nie można rozpocząć gry. Sprawdź gotowość świata i możliwość zapisu.");}return OK;
}
bool UMenuFlowSubsystem::Load(const FString& Slot)
{
    if(!MenuWorld())return false;
    bool Valid=false;DescribeSave(Slot,Valid);if(!Valid){Notice=TEXT("Ten zapis jest niedostępny lub niezgodny.");return false;}
    auto* D=MenuWorld()->GetSubsystem<UDistrictSaveSubsystem>();const FString Previous=D->Slot;D->Slot=Slot;
    const bool OK=D->LoadSaved();D->Slot=Previous;
    if(OK){bHasSession=true;Resume();}else Notice=D->Status;return OK;
}
bool UMenuFlowSubsystem::Save()
{
    if(!MenuWorld() || !bHasSession)return false;
    auto* D=MenuWorld()->GetSubsystem<UDistrictSaveSubsystem>();const bool OK=D->Save();Notice=D->Status;return OK;
}
void UMenuFlowSubsystem::Quit(){if(PC.IsValid())UKismetSystemLibrary::QuitGame(PC.Get(),PC.Get(),EQuitPreference::Quit,false);}
TArray<FString> UMenuFlowSubsystem::SaveSlots() const
{
    TArray<FString> Files;IFileManager::Get().FindFiles(Files,*(FPaths::ProjectSavedDir()/TEXT("SaveGames/WroclawDistrict*.sav")),true,false);
    Files.RemoveAll([](const FString& F){return F.Contains(TEXT("Smoke"));});
    Files.Sort([](const FString& A,const FString& B){const FString P=FPaths::ProjectSavedDir()/TEXT("SaveGames");return IFileManager::Get().GetTimeStamp(*(P/A))>IFileManager::Get().GetTimeStamp(*(P/B));});
    for(auto& File:Files)File=FPaths::GetBaseFilename(File);return Files;
}
FString UMenuFlowSubsystem::DescribeSave(const FString& Slot,bool& Valid) const
{
    Valid=false;if(!SaveSlots().Contains(Slot))return TEXT("Niedostępny zapis");
    auto* Data=Cast<UDistrictSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot,0));
    auto* R=MenuWorld()?MenuWorld()->GetSubsystem<UDistrictGameplaySubsystem>()->GetRegistry():nullptr;
    if(!Data || !R || !Data->Validate(R->BoundsMin,R->BoundsMax,R->DistrictId))return TEXT("Zapis uszkodzony lub niezgodny — zachowano plik");
    Valid=true;const auto Date=IFileManager::Get().GetTimeStamp(*(FPaths::ProjectSavedDir()/TEXT("SaveGames")/(Slot+TEXT(".sav"))));
    return FString::Printf(TEXT("Nadodrze  /  %s  /  %.2f zł  /  %s"),*Date.ToString(TEXT("%d.%m.%Y  %H:%M")),Data->Economy.BalanceCents/100.0,Data->DeliveryStage==4?TEXT("Dostawa ukończona"):TEXT("Dostawa w toku"));
}
