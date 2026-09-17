#include "Geography/GeoPreviewGameMode.h"
#include "Systems/CityGameplaySubsystem.h"
#include "Data/CityDefinition.h"
#include "EngineUtils.h"
#include "Vehicles/DriveableVehicle.h"
#include "Character/SliceCharacter.h"
#include "UI/SliceController.h"
#include "UI/SliceHUD.h"
#include "Mission/SliceMission.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WorldPartitionStreamingSourceComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
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
    auto *M = GetGameInstance()->GetSubsystem<USliceMission>();
    M->State = Wroclaw::Progress{};
    M->bDebugSession = true;
    M->bInGame = true;
    M->bShowMenu = false;
    M->bDead = false;
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
    M->Notify(TEXT("Wrocław — świat GIS. B: dziennik. Postęp miasta zapisuje się oddzielnie od kampanii."));
}
void AGeoPreviewGameMode::Tick(float Dt)
{
    Super::Tick(Dt);
    if (bReady)
        return;
    auto *P = Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
    if (!P || !P->StreamingSource->IsStreamingCompleted())
        return;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(GeoFloor), false, P);
    if (!GetWorld()->LineTraceSingleByChannel(Hit, Start + FVector(0, 0, 100), Start - FVector(0, 0, 400),
                                              ECC_Visibility, Params))
        return;
    P->SetActorEnableCollision(true);
    if (!P->TeleportTo(Hit.ImpactPoint + FVector(0, 0, 94), FRotator::ZeroRotator, false, false))
    {
        P->SetActorEnableCollision(false);
        return;
    }
    P->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    bReady = true;
}
