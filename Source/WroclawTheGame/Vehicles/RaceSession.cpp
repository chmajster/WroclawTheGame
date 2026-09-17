#include "Vehicles/RaceSession.h"
#include "Engine/World.h"
#include "WorldPartition/WorldPartitionStreamingSourceComponent.h"
#include "Vehicles/DriveableVehicle.h"
#include "Vehicles/VehicleDefinition.h"
#include "Character/SliceCharacter.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
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
    if (!Candidate.Start(Points, {StartLocation.X, StartLocation.Y, StartLocation.Z}, Race->TimeLimit,
                         Race->bDelivery))
        return false;
    // Do not reset through another actor or a wall. TeleportTo checks encroachment.
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
    }
    if (Progress.status != Wroclaw::RaceStatus::Running)
        return;
    if (!Car->Driver || Car->Health <= 0)
    {
        Progress.status = Wroclaw::RaceStatus::Failed;
        return;
    }
    FVector P = Car->GetActorLocation();
    Progress.Tick(Dt, {P.X, P.Y, P.Z}, FMath::Max(0.f, LastHealth - Car->Health));
    LastHealth = Car->Health;
    if (Progress.status == Wroclaw::RaceStatus::Finished && Definitions.IsValidIndex(Selected))
    {
        FName Id = Definitions[Selected]->RaceId;
        float *Best = Records->BestTimes.Find(Id);
        if (!Best || Progress.elapsed < *Best)
        {
            Records->BestTimes.Add(Id, Progress.elapsed);
            bRecordSaved = UGameplayStatics::SaveGameToSlot(Records, TEXT("NadodrzeRace_v1"), 0);
        }
    }
}
FString URaceSession::StatusText() const
{
    if (!Definitions.IsValidIndex(Selected))
        return TEXT("");
    const TCHAR *State = Progress.status == Wroclaw::RaceStatus::Running
                             ? TEXT("TRWA")
                             : (Progress.status == Wroclaw::RaceStatus::Finished
                                    ? TEXT("META")
                                    : (Progress.status == Wroclaw::RaceStatus::Failed ? TEXT("NIEPOWODZENIE")
                                                                                      : TEXT("GOTOWY")));
    return FString::Printf(
        TEXT("%s | %s | %.1f s | punkt %u/%u | ładunek %.0f%%\nF5 start/restart | F6 wybierz próbę%s"),
        *Definitions[Selected]->Title.ToString(), State, Progress.elapsed, Progress.next,
        unsigned(Progress.checkpoints.size()), Progress.cargo,
        bRecordSaved ? TEXT("") : TEXT(" | zapis rekordu nie powiódł się"));
}
