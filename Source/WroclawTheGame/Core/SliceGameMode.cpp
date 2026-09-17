#include "Core/SliceGameMode.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Character/SliceCharacter.h"
#include "World/SliceWorld.h"
#include "UI/SliceController.h"
#include "UI/SliceHUD.h"
#include "Mission/SliceMission.h"
#include "AI/SliceEnemy.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WorldPartitionStreamingSourceComponent.h"
ASliceGameMode::ASliceGameMode()
{
    DefaultPawnClass = ASliceCharacter::StaticClass();
    PlayerControllerClass = ASliceController::StaticClass();
    HUDClass = ASliceHUD::StaticClass();
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bTickEvenWhenPaused = true;
    PrimaryActorTick.TickInterval = .1;
}
void ASliceGameMode::BeginPlay()
{
    Super::BeginPlay();
    Started = FPlatformTime::Seconds();
    auto *M = GetGameInstance()->GetSubsystem<USliceMission>();
    GetWorld()->SpawnActor<ASliceWorld>();
    auto *PC = Cast<ASliceController>(UGameplayStatics::GetPlayerController(this, 0));
    if (auto *P = Cast<ASliceCharacter>(PC ? PC->GetPawn() : nullptr))
    {
        P->GetCharacterMovement()->DisableMovement();
        P->SetActorEnableCollision(false);
        P->SetActorLocation(M->SpawnPoint(), false);
        P->StreamingSource->EnableStreamingSource();
        PC->SetControlRotation(FRotator(-10, 0, 0));
    }
    if (PC && (M->bShowMenu || M->State.Finished()))
        PC->SetPause(true);
}
bool ASliceGameMode::HasThreat() const
{
    for (TActorIterator<ASliceEnemy> It(GetWorld()); It; ++It)
        if (It->IsThreat())
            return true;
    return false;
}
FString ASliceGameMode::ThreatText() const
{
    for (TActorIterator<ASliceEnemy> It(GetWorld()); It; ++It)
        if (It->IsThreat())
            return It->StatusText();
    return TEXT("");
}
void ASliceGameMode::Tick(float Dt)
{
    Super::Tick(Dt);
    auto *M = GetGameInstance()->GetSubsystem<USliceMission>();
    auto *P = Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
    if (!P)
        return;
    if (!bReady)
    {
        if (!P->StreamingSource->IsStreamingCompleted())
            return;
        FHitResult Floor;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(SpawnFloor), false, P);
        GetWorld()->LineTraceSingleByChannel(
            Floor, P->GetActorLocation(), P->GetActorLocation() - FVector(0, 0, 250), ECC_Visibility, Params);
        if (!Floor.bBlockingHit)
        {
            if (FPlatformTime::Seconds() - Started > 30)
                M->Notify(TEXT("Brak podłoża przy checkpointcie. Sprawdź przygotowanie mapy."));
            return;
        }
        P->SetActorEnableCollision(true);
        if (!P->TeleportTo(M->SpawnPoint(), FRotator::ZeroRotator, false, false))
        {
            P->SetActorEnableCollision(false);
            M->Notify(TEXT("Checkpoint zajęty. Wybierz nową grę z menu."));
            return;
        }
        P->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        bReady = true;
    }
    if (!M->bInGame || M->bDead || M->State.Finished() || UGameplayStatics::IsGamePaused(this))
        return;
    M->State.Tick(Dt);
    const FVector L = P->GetActorLocation();
    for (const auto &A : Wroclaw::Catalog())
        if (!M->State.Done(A.id) && M->State.CanDo(A))
        {
            const bool Zone = A.kind == "zone" && FVector::Dist2D(L, FVector(A.x, A.y, A.z)) < 180 &&
                              FMath::Abs(L.Z - A.z) < 100;
            const bool Calm =
                std::find(A.tags.begin(), A.tags.end(), "Trigger.Auto.NoThreat") != A.tags.end() &&
                !HasThreat();
            if (Zone || Calm)
                if (M->Act(UTF8_TO_TCHAR(A.id.c_str())) == Wroclaw::Result::Applied && !A.body.empty())
                    M->Notify(UTF8_TO_TCHAR(A.body.c_str()));
        }
    if (M->State.Finished())
        if (auto *PC = Cast<ASliceController>(P->GetController()))
            PC->SetPause(true);
}

void ASliceGameMode::Relocate(const FVector &Location)
{
    auto *M = GetGameInstance()->GetSubsystem<USliceMission>();
    M->Anchor = Location;
    if (auto *P = Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
    {
        P->GetCharacterMovement()->DisableMovement();
        P->SetActorEnableCollision(false);
        P->SetActorLocation(Location, false);
        bReady = false;
        Started = FPlatformTime::Seconds();
    }
}
