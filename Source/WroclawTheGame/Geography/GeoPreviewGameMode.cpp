#include "Geography/GeoPreviewGameMode.h"
#include "World/SurfaceQualityDirector.h"
#include "Systems/CityGameplaySubsystem.h"
#include "Data/CityDefinition.h"
#include "EngineUtils.h"
#include "Vehicles/DriveableVehicle.h"
#include "Character/SliceCharacter.h"
#include "Components/CapsuleComponent.h"
#include "UI/SliceController.h"
#include "UI/SliceHUD.h"
#include "Mission/SliceMission.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WorldPartitionStreamingSourceComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Geography/DistrictRegistry.h"
AGeoPreviewGameMode::AGeoPreviewGameMode()
{
    DefaultPawnClass = ASliceCharacter::StaticClass();
    PlayerControllerClass = ASliceController::StaticClass();
    HUDClass = ASliceHUD::StaticClass();
    PrimaryActorTick.bCanEverTick = true;
}
void AGeoPreviewGameMode::BeginPlay()
{
    Super::BeginPlay();
    bool HasSurfaces=false;
    for(TActorIterator<ASurfaceQualityDirector> It(GetWorld());It;++It){HasSurfaces=true;break;}
    if(!HasSurfaces)GetWorld()->SpawnActor<ASurfaceQualityDirector>();
    auto *M = GetGameInstance()->GetSubsystem<USliceMission>();
    M->State = Wroclaw::Progress{};
    M->bDebugSession = true;
    M->bInGame = true;
    M->bShowMenu = false;
    M->bDead = false;
    // Controllers can open the campaign menu during actor BeginPlay before the
    // geographic mode marks the session active. Clear that pause/input state.
    if (auto* PC=Cast<ASliceController>(UGameplayStatics::GetPlayerController(this,0))) PC->ResumeGame();
    if (auto *P = Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
    {
        Start = P->GetActorLocation();
        for (TActorIterator<ACityRegistry> It(GetWorld()); It; ++It)
        {
            auto *City = GetWorld()->GetSubsystem<UCityGameplaySubsystem>();
            City->Activate(Start);
            Start = City->SpawnPoint();
            P->SetActorLocation(Start);
            for (TActorIterator<ADriveableVehicle> Vehicle(GetWorld());Vehicle;++Vehicle) City->RestoreVehicle(*Vehicle);
            break;
        }
        P->GetCharacterMovement()->DisableMovement();
        P->SetActorEnableCollision(false);
        P->StreamingSource->EnableStreamingSource();
    }
    if (!GetWorld()->GetSubsystem<UCityGameplaySubsystem>()->IsWriteBlocked())
    {
        bool District=false;for(TActorIterator<AWTGDistrictRegistry> It(GetWorld());It;++It){District=true;break;}
        M->Notify(District?TEXT("Nadodrze — porozmawiaj z Mają. E: interakcja; F5/F9: zapis/odczyt; Esc: pauza."):TEXT("Wrocław — świat GIS. B: dziennik. Postęp miasta zapisuje się oddzielnie od kampanii."));
    }
}
void AGeoPreviewGameMode::InitGame(const FString& MapName,const FString& Options,FString& ErrorMessage)
{
    Super::InitGame(MapName,Options,ErrorMessage);
    if (auto* Class=LoadClass<ASliceCharacter>(nullptr,TEXT("/Game/CharacterCreator/BP_WTG_PlayerCharacter.BP_WTG_PlayerCharacter_C"))) DefaultPawnClass=Class;
}
void AGeoPreviewGameMode::Tick(float Dt)
{
    Super::Tick(Dt);
    if (bReady)
        return;
    auto *P = Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
    if (!P || !P->StreamingSource->IsStreamingCompleted())
        return;
    // Default pawn creation can happen after BeginPlay. Resolve its real spawn
    // here instead of tracing from an unset cached anchor near world origin.
    Start=P->GetActorLocation();
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(GeoFloor), false, P);
    if (!GetWorld()->LineTraceSingleByChannel(Hit, Start + FVector(0, 0, 100), Start - FVector(0, 0, 400),
                                              ECC_Visibility, Params))
        return;
    P->SetActorEnableCollision(true);
    if (!P->TeleportTo(Hit.ImpactPoint + FVector(0, 0, P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+4), P->GetActorRotation(), false, false))
    {
        P->SetActorEnableCollision(false);
        return;
    }
    P->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    bReady = true;
}
