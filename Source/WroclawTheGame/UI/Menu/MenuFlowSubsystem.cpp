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
#include "Vehicles/DriveableVehicle.h"
#include "Systems/CityGameplaySubsystem.h"
#include "World/SurfaceQualityDirector.h"
#include "Audio/WTGAudioSettings.h"
#include "Components/AudioComponent.h"
#include "UI/PerformanceSettings.h"
#include "UI/Menu/MenuLoadingScreen.h"
#include "Framework/Application/SlateApplication.h"
#include "UObject/UObjectGlobals.h"
#include "MoviePlayer.h"
#include "Engine/Engine.h"
#include "Camera/PlayerCameraManager.h"

void UMenuFlowSubsystem::Initialize(FSubsystemCollectionBase& C)
{
    Super::Initialize(C);
    const bool Skip=FParse::Param(FCommandLine::Get(),TEXT("WTGSkipFrontEnd")) ||
        (FParse::Param(FCommandLine::Get(),TEXT("WTGNoDistrictAutoLoad")) && !FParse::Param(FCommandLine::Get(),TEXT("WTGMenuTest")));
    Screen=Skip?EWTGMenuScreen::Playing:EWTGMenuScreen::Boot;bHasSession=Skip;
    Ticker=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this,&UMenuFlowSubsystem::Tick));
    LoadHandle=FCoreUObjectDelegates::PreLoadMap.AddUObject(this,&UMenuFlowSubsystem::PrepareLoadingScreen);
    if(FSlateApplication::IsInitialized())ActivationHandle=FSlateApplication::Get().OnApplicationActivationStateChanged().AddUObject(this,&UMenuFlowSubsystem::ApplicationActivationChanged);
}
void UMenuFlowSubsystem::Deinitialize()
{
    FTSTicker::GetCoreTicker().RemoveTicker(Ticker);if(Widget)Widget->RemoveFromParent();Widget=nullptr;
    FCoreUObjectDelegates::PreLoadMap.Remove(LoadHandle);
    if(FSlateApplication::IsInitialized())FSlateApplication::Get().OnApplicationActivationStateChanged().Remove(ActivationHandle);
    DeferredOperation=nullptr;
    if(Camera)Camera->Destroy();Camera=nullptr;PC.Reset();Super::Deinitialize();
}
UWorld* UMenuFlowSubsystem::MenuWorld() const { return PC.IsValid()?PC->GetWorld():nullptr; }
TSharedPtr<SMenuShell> UMenuFlowSubsystem::SlateMenu() const { return Widget?Widget->SlateMenu():nullptr; }
void UMenuFlowSubsystem::Attach(ASliceController* C)
{
    if(Widget)Widget->RemoveFromParent();Widget=nullptr;Camera=nullptr;HiddenCharacters.Reset();PC=C;
    if(IsFrontEnd()){
        Screen=EWTGMenuScreen::Boot;
        C->SetIgnoreMoveInput(true);C->SetIgnoreLookInput(true);C->SetInputMode(FInputModeUIOnly());
        Widget=CreateWidget<UFrontEndMenuWidget>(C);Widget->AddToViewport(500);
    }
}
bool UMenuFlowSubsystem::Tick(float Dt)
{
    if(!PC.IsValid())return true;
    const auto* Audio=UWTGAudioSettings::Get();
    const auto* Performance=UWTGPerformanceSettings::Get();
    const float Cap=IsMenuOpen()?30.f:float(Performance->FPSLimit);
    if(Cap!=AppliedFrameCap && GEngine){GEngine->SetMaxFPS(Cap);AppliedFrameCap=Cap;}
    if(DeferredOperation){TransitionSeconds+=Dt;TransitionOpacity=1-FMath::Clamp(TransitionSeconds/.14f,0.f,1.f);
        if(TransitionSeconds>=.14f){auto Operation=MoveTemp(DeferredOperation);ExecutingTransition=true;Operation();ExecutingTransition=false;TransitionOpacity=1;}}
    if(Screen==EWTGMenuScreen::Main && Camera && !Performance->bReduceUIMotion){
        CameraSeconds+=Dt;const FVector Offset(120*FMath::Sin(CameraSeconds*.055),80*FMath::Sin(CameraSeconds*.038),20*FMath::Sin(CameraSeconds*.07));
        const FVector Position=CameraBase+Offset;Camera->SetActorLocationAndRotation(Position,(CameraLook-Position).Rotation());
    }
    AudioElapsed+=Dt;
    if(IsMenuOpen() && AudioElapsed>=.2){AudioElapsed=0;
        for(TActorIterator<ASurfaceQualityDirector> It(MenuWorld());It;++It)
            if(It->Ambience)It->Ambience->SetVolumeMultiplier(.18f*Audio->MasterVolume*Audio->AmbientVolume);
        for(TActorIterator<ADriveableVehicle> It(MenuWorld());It;++It)
            if(It->EngineAudio)It->EngineAudio->SetVolumeMultiplier((It->bSiren?.45f:.25f)*Audio->MasterVolume*Audio->SFXVolume);
    }
    if(Screen==EWTGMenuScreen::Boot){
        auto* P=Cast<ASliceCharacter>(PC->GetPawn());
        if(P && P->GetCharacterMovement()->MovementMode==MOVE_Walking && MenuWorld()->GetRealTimeSeconds()>1)ShowMain();
    }
    if(Screen==EWTGMenuScreen::Playing && !MenuWorld()->IsPaused() && bHasSession){
        PlayedSeconds+=Dt;APawn* P=PC->GetPawn();
        if(P){const FVector Position=P->GetActorLocation();
            if(LastTrackedPawn.Get()==P){const double Distance=FVector::Dist2D(Position,LastPosition);
                if(Distance<1000){if(Cast<ADriveableVehicle>(P))DrivenCm+=Distance;else WalkedCm+=Distance;}}
            LastTrackedPawn=P;LastPosition=Position;}
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
    if(Screen==EWTGMenuScreen::Pause && Transition([this]{ShowMain();}))return;
    if(R){CameraBase=R->PlayerStart+FVector(-1100,-900,850);CameraLook=R->MeetingPoint+FVector(500,1600,300);CameraSeconds=0;
        Camera->SetActorLocationAndRotation(CameraBase,(CameraLook-CameraBase).Rotation());}
    else {CameraBase=PC->GetPawn()?PC->GetPawn()->GetActorLocation()+FVector(-1000,-500,700):FVector::ZeroVector;CameraLook=CameraBase+FVector(1000,500,-300);Camera->SetActorLocationAndRotation(CameraBase,(CameraLook-CameraBase).Rotation());}
    auto& PP=Camera->GetCameraComponent()->PostProcessSettings;
    PP.bOverride_VignetteIntensity=true;PP.VignetteIntensity=.4f;
    PP.bOverride_ColorSaturation=true;PP.ColorSaturation=FVector4(.85,.85,.85,1);
    for(TActorIterator<ACharacter> It(MenuWorld());It;++It)if(!It->IsHidden()){It->SetActorHiddenInGame(true);HiddenCharacters.Add(*It);}
    PC->SetViewTarget(Camera);Open(EWTGMenuScreen::Main);
}
void UMenuFlowSubsystem::ShowPause(){Open(EWTGMenuScreen::Pause);}
void UMenuFlowSubsystem::Resume()
{
    if(!PC.IsValid() || !bHasSession)return;
    if(Transition([this]{Resume();}))return;
    Screen=EWTGMenuScreen::Playing;Notice.Empty();GetGameInstance()->GetSubsystem<USliceMission>()->bShowMenu=false;
    if(Widget)Widget->RemoveFromParent();Widget=nullptr;
    for(auto& Character:HiddenCharacters)if(Character.IsValid())Character->SetActorHiddenInGame(false);HiddenCharacters.Reset();
    if(APawn* P=PC->GetPawn())P->SetActorHiddenInGame(false);
    ResetTracking();PC->SetViewTarget(PC->GetPawn());PC->ResetIgnoreMoveInput();PC->ResetIgnoreLookInput();
    PC->SetPause(false);PC->SetInputMode(FInputModeGameOnly());PC->bShowMouseCursor=false;
    if(PC->PlayerCameraManager && !UWTGPerformanceSettings::Get()->bReduceUIMotion)PC->PlayerCameraManager->StartCameraFade(1,0,.18f,FLinearColor::Black,false);
    UE_LOG(LogTemp,Display,TEXT("WTG_MENU_SCREEN playing"));
}
bool UMenuFlowSubsystem::StartNew()
{
    if(!MenuWorld())return false;
    if(Transition([this]{if(!StartNew() && Widget)Widget->Refresh();}))return true;
    if(!MenuWorld()->GetSubsystem<UDistrictGameplaySubsystem>()->IsActive()){PC->NewGame();return true;}
    auto* D=MenuWorld()->GetSubsystem<UDistrictSaveSubsystem>();
    bool OK=MenuWorld()->GetSubsystem<UDistrictGameplaySubsystem>()->IsActive()?D->NewRun():false;
    if(OK){bHasSession=true;Resume();}else {PC->SetPause(true);Notice=TEXT("Nie można rozpocząć gry. Sprawdź gotowość świata i możliwość zapisu.");}return OK;
}
bool UMenuFlowSubsystem::Load(const FString& Slot)
{
    if(!MenuWorld())return false;
    bool SaveValid=false;DescribeSave(Slot,SaveValid);if(!SaveValid){Notice=TEXT("Zapis niedostępny lub niezgodny.");return false;}
    if(Transition([this,Slot]{if(!Load(Slot) && Widget)Widget->Refresh();}))return true;
    if(Slot==TEXT("campaign") || Slot==TEXT("city")){
        auto* M=GetGameInstance()->GetSubsystem<USliceMission>();
        const bool OK=Slot==TEXT("city")?MenuWorld()->GetSubsystem<UCityGameplaySubsystem>()->LoadSaved():M->ContinueGame();
        if(OK){bHasSession=true;Resume();UGameplayStatics::OpenLevel(PC.Get(),Slot==TEXT("city")?TEXT("Nadodrze_GIS"):TEXT("Przebudzenie_Source"));}
        else Notice=TEXT("Nie można wczytać tego postępu.");return OK;
    }
    bool Valid=false;DescribeSave(Slot,Valid);if(!Valid){Notice=TEXT("Ten zapis jest niedostępny lub niezgodny.");return false;}
    auto* D=MenuWorld()->GetSubsystem<UDistrictSaveSubsystem>();const FString Previous=D->Slot;D->Slot=Slot;
    const bool OK=D->LoadSaved();D->Slot=Previous;
    if(OK){bHasSession=true;Resume();}else Notice=D->Status;return OK;
}
bool UMenuFlowSubsystem::Save()
{
    if(!MenuWorld() || !bHasSession)return false;
    if(MenuWorld()->GetSubsystem<UDistrictGameplaySubsystem>()->IsActive()){
        auto* D=MenuWorld()->GetSubsystem<UDistrictSaveSubsystem>();const bool OK=D->Save();Notice=D->Status;return OK;
    }
    auto* City=MenuWorld()->GetSubsystem<UCityGameplaySubsystem>();
    const bool OK=City->IsActive()?City->SaveAppearance():GetGameInstance()->GetSubsystem<USliceMission>()->SaveCheckpoint();
    Notice=OK?TEXT("Postęp zapisany"):TEXT("Nie można zapisać postępu.");return OK;
}
void UMenuFlowSubsystem::Quit(){if(Transition([this]{Quit();}))return;if(PC.IsValid())UKismetSystemLibrary::QuitGame(PC.Get(),PC.Get(),EQuitPreference::Quit,false);}
bool UMenuFlowSubsystem::Transition(TFunction<void()> Operation)
{
    if(ExecutingTransition || UWTGPerformanceSettings::Get()->bReduceUIMotion)return false;
    if(!DeferredOperation){DeferredOperation=MoveTemp(Operation);TransitionSeconds=0;}return true;
}
void UMenuFlowSubsystem::ApplicationActivationChanged(bool Active)
{
    if(!Active && Screen==EWTGMenuScreen::Playing && PC.IsValid() && bHasSession)ShowPause();
}
void UMenuFlowSubsystem::PrepareLoadingScreen(const FString& Map)
{
    if(IsRunningDedicatedServer() || !IsMoviePlayerEnabled())return;
    FLoadingScreenAttributes Loading;Loading.MinimumLoadingScreenDisplayTime=0;Loading.bAutoCompleteWhenLoadingCompletes=true;
    Loading.WidgetLoadingScreen=WTGMenu::LoadingScreen();GetMoviePlayer()->SetupLoadingScreen(Loading);
    UE_LOG(LogTemp,Display,TEXT("WTG_MENU_LOADING %s"),*Map);
}
void UMenuFlowSubsystem::HideForCreator(){if(Widget)Widget->RemoveFromParent();Widget=nullptr;}
void UMenuFlowSubsystem::RestoreAfterCreator(){if(IsFrontEnd())ShowMain();else ShowPause();}
void UMenuFlowSubsystem::CreatedSession(){bHasSession=true;ExecutingTransition=true;Resume();ExecutingTransition=false;}
TArray<FString> UMenuFlowSubsystem::SaveSlots() const
{
    if(MenuWorld() && !MenuWorld()->GetSubsystem<UDistrictGameplaySubsystem>()->IsActive()){
        if(MenuWorld()->GetSubsystem<UCityGameplaySubsystem>()->IsActive())return UGameplayStatics::DoesSaveGameExist(TEXT("WroclawCity_v1"),0)?TArray<FString>{TEXT("city")}:TArray<FString>{};
        return GetGameInstance()->GetSubsystem<USliceMission>()->HasSave()?TArray<FString>{TEXT("campaign")}:TArray<FString>{};
    }
    TArray<FString> Files;IFileManager::Get().FindFiles(Files,*(FPaths::ProjectSavedDir()/TEXT("SaveGames/WroclawDistrict*.sav")),true,false);
    Files.RemoveAll([](const FString& F){return F.Contains(TEXT("Smoke"));});
    Files.Sort([](const FString& A,const FString& B){const FString P=FPaths::ProjectSavedDir()/TEXT("SaveGames");return IFileManager::Get().GetTimeStamp(*(P/A))>IFileManager::Get().GetTimeStamp(*(P/B));});
    for(auto& File:Files)File=FPaths::GetBaseFilename(File);return Files;
}
FString UMenuFlowSubsystem::DescribeSave(const FString& Slot,bool& Valid) const
{
    Valid=false;if(!SaveSlots().Contains(Slot))return TEXT("Niedostępny zapis");
    if(Slot==TEXT("campaign") || Slot==TEXT("city")){Valid=true;return Slot==TEXT("city")?TEXT("Miasto GIS — ostatni zapis"):TEXT("Przebudzenie — ostatni checkpoint");}
    auto* Data=Cast<UDistrictSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot,0));
    auto* R=MenuWorld()?MenuWorld()->GetSubsystem<UDistrictGameplaySubsystem>()->GetRegistry():nullptr;
    if(!Data || !R || !Data->Validate(R->BoundsMin,R->BoundsMax,R->DistrictId))return TEXT("Zapis uszkodzony lub niezgodny — zachowano plik");
    Valid=true;const auto Date=IFileManager::Get().GetTimeStamp(*(FPaths::ProjectSavedDir()/TEXT("SaveGames")/(Slot+TEXT(".sav"))));
    return FString::Printf(TEXT("Nadodrze  /  %s  /  %.2f zł  /  %s"),*Date.ToString(TEXT("%d.%m.%Y  %H:%M")),Data->Economy.BalanceCents/100.0,Data->DeliveryStage==4?TEXT("Dostawa ukończona"):TEXT("Dostawa w toku"));
}
