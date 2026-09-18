#include "Vehicles/RaceSession.h"
#include "Engine/World.h"
#include "Components/WorldPartitionStreamingSourceComponent.h"
#include "Vehicles/DriveableVehicle.h"
#include "Vehicles/VehicleDefinition.h"
#include "Vehicles/CityTrafficVehicle.h"
#include "Vehicles/VehicleAIDriverComponent.h"
#include "Vehicles/VehiclePursuitSubsystem.h"
#include "Character/SliceCharacter.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

namespace
{
Wroclaw::RaceMode ModeFor(const URaceDefinition *Race)
{
    if (!Race)
        return Wroclaw::RaceMode::TimeTrial;
    if (Race->Mode == TEXT("Escape"))
        return Wroclaw::RaceMode::Escape;
    if (Race->Mode == TEXT("Follow"))
        return Wroclaw::RaceMode::Follow;
    if (Race->Mode == TEXT("Navigation"))
        return Wroclaw::RaceMode::Navigation;
    if (Race->Mode == TEXT("Delivery") || Race->bDelivery)
        return Wroclaw::RaceMode::Delivery;
    return Wroclaw::RaceMode::TimeTrial;
}

const TCHAR *PursuitLabel(EVehiclePursuitState State)
{
    switch (State)
    {
    case EVehiclePursuitState::Locate: return TEXT("LOKALIZUJĄ");
    case EVehiclePursuitState::Chase: return TEXT("POŚCIG");
    case EVehiclePursuitState::Search: return TEXT("SZUKAJĄ");
    case EVehiclePursuitState::Lost: return TEXT("ZGUBIENI");
    default: return TEXT("BRAK");
    }
}
}

URaceSession::URaceSession()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void URaceSession::BeginPlay()
{
    Super::BeginPlay();
    Records = Cast<URaceRecords>(UGameplayStatics::LoadGameFromSlot(TEXT("NadodrzeRace_v1"), 0));
    if (!Records || Records->Version != 1)
        Records = NewObject<URaceRecords>(this);
    for (auto It = Records->BestTimes.CreateIterator(); It; ++It)
        if (!FMath::IsFinite(It.Value()) || It.Value() <= 0)
            It.RemoveCurrent();
}

void URaceSession::EndPlay(const EEndPlayReason::Type Reason)
{
    CleanupScenario(true);
    Super::EndPlay(Reason);
}

void URaceSession::CleanupScenario(bool bResetPursuit)
{
    if (FollowTarget)
    {
        FollowTarget->DeactivateTraffic();
        FollowTarget->Destroy();
        FollowTarget = nullptr;
    }
    if (bResetPursuit)
        if (auto *Pursuit = GetWorld()->GetSubsystem<UVehiclePursuitSubsystem>())
            Pursuit->StopPursuit();
    bScenarioStarted = false;
}

bool URaceSession::Start(int32 Index)
{
    auto *Car = Cast<ADriveableVehicle>(GetOwner());
    if (!Car || !Car->Driver || !Definitions.IsValidIndex(Index) || !Definitions[Index])
        return false;

    auto *Race = Definitions[Index].Get();
    std::vector<Wroclaw::RoadPoint> Points;
    for (const auto &P : Race->Checkpoints)
        Points.push_back({P.X, P.Y, P.Z});
    const FVector StartLocation = Race->Start.GetLocation();

    Wroclaw::RaceProgress Candidate;
    if (!Candidate.Start(Points, {StartLocation.X, StartLocation.Y, StartLocation.Z},
                         Race->TimeLimit, ModeFor(Race)))
        return false;

    CleanupScenario(true);
    if (!Car->TeleportTo(StartLocation, Race->Start.Rotator(), false, false))
        return false;

    Car->Chassis->SetPhysicsLinearVelocity(FVector::ZeroVector);
    Car->Chassis->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
    Car->Chassis->SetSimulatePhysics(false);
    bAwaitingStreaming = true;
    Car->Health = Car->Definition->MaxHealth;
    Car->bEngine = true;
    LastHealth = Car->Health;
    Selected = Index;
    Progress = Candidate;
    bRecordSaved = true;
    bScenarioStarted = false;
    return true;
}

bool URaceSession::StartScenario()
{
    if (!Definitions.IsValidIndex(Selected))
        return false;
    auto *Race = Definitions[Selected].Get();
    if (!Race)
        return false;

    if (Progress.mode == Wroclaw::RaceMode::Escape)
    {
        auto *Pursuit = GetWorld()->GetSubsystem<UVehiclePursuitSubsystem>();
        if (!Pursuit->StartPursuit(FMath::Clamp(Race->PursuerCount, 1, 3)))
            return false;
    }
    else if (Progress.mode == Wroclaw::RaceMode::Follow)
    {
        TArray<FVector> Route;
        Route.Add(Race->Start.GetLocation());
        Route.Append(Race->Checkpoints);
        if (Route.Num() < 4)
            return false;

        FollowTarget = GetWorld()->SpawnActor<ACityTrafficVehicle>();
        if (!FollowTarget)
            return false;
        FollowTarget->bPersistentPlayerVehicle = false;
        FollowTarget->TrafficDriver->CruiseSpeed = 950.0f;
        if (!FollowTarget->ConfigureTraffic(Route, 1, false))
        {
            FollowTarget->Destroy();
            FollowTarget = nullptr;
            return false;
        }
    }

    bScenarioStarted = true;
    return true;
}

void URaceSession::TickComponent(float Dt, ELevelTick Tick, FActorComponentTickFunction *Function)
{
    Super::TickComponent(Dt, Tick, Function);
    auto *Car = Cast<ADriveableVehicle>(GetOwner());
    if (!Car)
        return;

    if (auto *PC = Cast<APlayerController>(Car->GetController()))
    {
        if (PC->WasInputKeyJustPressed(EKeys::F6) && Progress.status != Wroclaw::RaceStatus::Running &&
            !Definitions.IsEmpty())
            Selected = (Selected + 1) % Definitions.Num();
        if (PC->WasInputKeyJustPressed(EKeys::F5))
            Start(Selected);
    }

    if (bAwaitingStreaming)
    {
        FHitResult Floor;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(RaceFloor), false, Car);
        if (Car->Driver)
            Params.AddIgnoredActor(Car->Driver.Get());
        if (!Car->StreamingSource->IsStreamingCompleted() ||
            !GetWorld()->LineTraceSingleByChannel(Floor, Car->GetActorLocation(),
                                                  Car->GetActorLocation() - FVector(0, 0, 250),
                                                  ECC_Visibility, Params))
            return;
        Car->Chassis->SetSimulatePhysics(true);
        bAwaitingStreaming = false;
        if (!StartScenario())
        {
            Progress.status = Wroclaw::RaceStatus::Failed;
            return;
        }
    }

    if (Progress.status != Wroclaw::RaceStatus::Running)
        return;
    if (!Car->Driver || Car->Health <= 0)
    {
        Progress.status = Wroclaw::RaceStatus::Failed;
        CleanupScenario(true);
        return;
    }

    auto *Race = Definitions.IsValidIndex(Selected) ? Definitions[Selected].Get() : nullptr;
    if (!Race)
    {
        Progress.status = Wroclaw::RaceStatus::Failed;
        CleanupScenario(true);
        return;
    }

    const FVector Position = Car->GetActorLocation();
    if (Progress.mode == Wroclaw::RaceMode::Follow)
    {
        if (!FollowTarget)
        {
            Progress.status = Wroclaw::RaceStatus::Failed;
        }
        else
        {
            const float Distance = FVector::Dist2D(Position, FollowTarget->GetActorLocation());
            FCollisionQueryParams SightParams(SCENE_QUERY_STAT(FollowVehicleSight), false, Car);
            SightParams.AddIgnoredActor(Car);
            FHitResult Hit;
            const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
                Hit, Position + FVector(0,0,80),
                FollowTarget->GetActorLocation() + FVector(0,0,80),
                ECC_Visibility, SightParams);
            const bool bVisible = !bBlocked || Hit.GetActor() == FollowTarget;
            const bool bTargetFinished = !FollowTarget->TrafficDriver->IsActive();
            Progress.TickFollow(Dt, Distance, bVisible, bTargetFinished,
                                Race->MinFollowDistance, Race->MaxFollowDistance,
                                Race->LostTargetTime);
        }
    }
    else
    {
        Progress.Tick(Dt, {Position.X, Position.Y, Position.Z},
                      FMath::Max(0.f, LastHealth - Car->Health));
        if (Progress.mode == Wroclaw::RaceMode::Escape &&
            GetWorld()->GetSubsystem<UVehiclePursuitSubsystem>()->GetState() ==
                EVehiclePursuitState::Lost)
            Progress.MarkPursuitLost();
    }
    LastHealth = Car->Health;

    if (Progress.status == Wroclaw::RaceStatus::Finished)
    {
        const FName Id = Race->RaceId;
        float *Best = Records->BestTimes.Find(Id);
        if (!Best || Progress.elapsed < *Best)
        {
            Records->BestTimes.Add(Id, Progress.elapsed);
            bRecordSaved = UGameplayStatics::SaveGameToSlot(Records, TEXT("NadodrzeRace_v1"), 0);
        }
        CleanupScenario(false);
    }
    else if (Progress.status == Wroclaw::RaceStatus::Failed)
        CleanupScenario(true);
}

FString URaceSession::StatusText() const
{
    if (!Definitions.IsValidIndex(Selected) || !Definitions[Selected])
        return TEXT("");
    const auto *Race = Definitions[Selected].Get();
    const TCHAR *State = Progress.status == Wroclaw::RaceStatus::Running
                             ? TEXT("TRWA")
                             : (Progress.status == Wroclaw::RaceStatus::Finished
                                    ? TEXT("META")
                                    : (Progress.status == Wroclaw::RaceStatus::Failed
                                           ? TEXT("NIEPOWODZENIE")
                                           : TEXT("GOTOWY")));

    if (Progress.mode == Wroclaw::RaceMode::Escape)
    {
        const auto PursuitState = GetWorld()->GetSubsystem<UVehiclePursuitSubsystem>()->GetState();
        return FString::Printf(
            TEXT("%s | %s | %.1f s | pościg: %s\nCel: zgub wszystkie auta przeciwników.\nF5 start/restart | F6 wybierz próbę%s"),
            *Race->Title.ToString(), State, Progress.elapsed, PursuitLabel(PursuitState),
            bRecordSaved ? TEXT("") : TEXT(" | zapis rekordu nie powiódł się"));
    }

    if (Progress.mode == Wroclaw::RaceMode::Follow)
    {
        return FString::Printf(
            TEXT("%s | %s | %.1f s | podejrzenie %.0f%% | utrata %.1f s\nTrzymaj dystans %.0f–%.0f m.\nF5 start/restart | F6 wybierz próbę%s"),
            *Race->Title.ToString(), State, Progress.elapsed, Progress.followSuspicion,
            Progress.lostTarget, Race->MinFollowDistance / 100.0f,
            Race->MaxFollowDistance / 100.0f,
            bRecordSaved ? TEXT("") : TEXT(" | zapis rekordu nie powiódł się"));
    }

    if (Progress.mode == Wroclaw::RaceMode::Navigation)
    {
        FString Hint = TEXT("Jedź według wskazówek, bez pełnej trasy GPS.");
        if (!Race->NavigationHints.IsEmpty())
        {
            const int32 HintIndex = FMath::Clamp(
                static_cast<int32>(Progress.next * Race->NavigationHints.Num() /
                                   FMath::Max<size_t>(Progress.checkpoints.size(), 1)),
                0, Race->NavigationHints.Num() - 1);
            Hint = Race->NavigationHints[HintIndex].ToString();
        }
        return FString::Printf(
            TEXT("%s | %s | %.1f s\nWskazówka: %s\nF5 start/restart | F6 wybierz próbę%s"),
            *Race->Title.ToString(), State, Progress.elapsed, *Hint,
            bRecordSaved ? TEXT("") : TEXT(" | zapis rekordu nie powiódł się"));
    }

    return FString::Printf(
        TEXT("%s | %s | %.1f s | punkt %u/%u | ładunek %.0f%%\nF5 start/restart | F6 wybierz próbę%s"),
        *Race->Title.ToString(), State, Progress.elapsed, Progress.next,
        unsigned(Progress.checkpoints.size()), Progress.cargo,
        bRecordSaved ? TEXT("") : TEXT(" | zapis rekordu nie powiódł się"));
}
