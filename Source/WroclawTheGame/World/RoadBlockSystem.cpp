#include "World/RoadBlockSystem.h"
#include "World/CityPopulation.h"
#include "Vehicles/DriveableVehicle.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

ACityRoadBlock::ACityRoadBlock()
{
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    for (int32 I = 0; I < 3; ++I)
    {
        const FVector Position(0, (I - 1) * 145.0f, 45.0f);
        const FName CollisionName(*FString::Printf(TEXT("BarrierCollision%d"), I));
        auto *Collision = CreateDefaultSubobject<UBoxComponent>(CollisionName);
        Collision->SetupAttachment(Root);
        Collision->SetRelativeLocation(Position);
        Collision->SetBoxExtent(FVector(60.0f, 62.5f, 45.0f));
        Collision->SetCollisionProfileName(TEXT("BlockAll"));
        Collision->SetMobility(EComponentMobility::Movable);
        BarrierColliders.Add(Collision);

        const FName Name(*FString::Printf(TEXT("Barrier%d"), I));
        auto *Barrier = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Barrier->SetupAttachment(Root);
        Barrier->SetRelativeLocation(Position);
        Barrier->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Barrier->SetCanEverAffectNavigation(false);
        Barrier->SetMobility(EComponentMobility::Movable);
        Barriers.Add(Barrier);
    }
}
void ACityRoadBlock::BeginPlay()
{
    Super::BeginPlay();
    UStaticMesh *BarrierMesh = LoadObject<UStaticMesh>(
        nullptr,
        TEXT("/Game/FreeModels/PolyHaven/concrete_road_barrier_02/concrete_road_barrier_02_1k.concrete_road_barrier_02_1k"));
    if (!BarrierMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("RoadBlock: imported CC0 barrier mesh is unavailable"));
        return;
    }
    for (UStaticMeshComponent *Barrier : Barriers)
        if (Barrier)
            Barrier->SetStaticMesh(BarrierMesh);
}

void URoadBlockSubsystem::ClearRoadBlock()
{
    if (ActiveRoadBlock)
        ActiveRoadBlock->Destroy();
    ActiveRoadBlock = nullptr;
    LastSpawnPoint = FVector::ZeroVector;
}

bool URoadBlockSubsystem::UpdateForHeat(int32 HeatLevel, ADriveableVehicle *PlayerVehicle,
                                        ACityPopulation *Population)
{
    if (HeatLevel < 4 || !PlayerVehicle || !Population)
    {
        if (HeatLevel < 3)
            ClearRoadBlock();
        return false;
    }

    if (ActiveRoadBlock)
    {
        const float Distance = FVector::Dist2D(
            ActiveRoadBlock->GetActorLocation(), PlayerVehicle->GetActorLocation());
        if (Distance < 25000.0f)
            return true;
        ClearRoadBlock();
    }

    const FVector PlayerPosition = PlayerVehicle->GetActorLocation();
    const FVector Forward = PlayerVehicle->GetVelocity().Size2D() > 200.0f
                                ? PlayerVehicle->GetVelocity().GetSafeNormal2D()
                                : PlayerVehicle->GetActorForwardVector().GetSafeNormal2D();

    const FVector *BestPoint = nullptr;
    FVector BestTangent = FVector::ForwardVector;
    double BestScore = TNumericLimits<double>::Max();
    for (const auto &Route : Population->Routes)
    {
        if (!Route.Vehicle || Route.Points.Num() < 3)
            continue;
        for (int32 I = 1; I < Route.Points.Num() - 1; ++I)
        {
            const FVector Delta = Route.Points[I] - PlayerPosition;
            const double Distance = Delta.Size2D();
            if (Distance < 5000.0 || Distance > 12000.0 ||
                FVector::DotProduct(Forward, Delta.GetSafeNormal2D()) < 0.45f)
                continue;
            if (!LastSpawnPoint.IsNearlyZero() &&
                FVector::DistSquared2D(Route.Points[I], LastSpawnPoint) < FMath::Square(4000.0))
                continue;

            const double Score = FMath::Abs(Distance - 8000.0);
            if (Score < BestScore)
            {
                BestScore = Score;
                BestPoint = &Route.Points[I];
                BestTangent = (Route.Points[I + 1] - Route.Points[I - 1]).GetSafeNormal2D();
            }
        }
    }
    if (!BestPoint)
        return false;

    FHitResult Floor;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(RoadBlockFloor), false);
    if (!GetWorld()->LineTraceSingleByChannel(
            Floor, *BestPoint + FVector(0,0,300), *BestPoint - FVector(0,0,500),
            ECC_Visibility, Params))
        return false;

    FTransform SpawnTransform(BestTangent.Rotation(), Floor.ImpactPoint);
    ActiveRoadBlock = GetWorld()->SpawnActorDeferred<ACityRoadBlock>(
        ACityRoadBlock::StaticClass(), SpawnTransform);
    if (!ActiveRoadBlock)
        return false;
    ActiveRoadBlock->FinishSpawning(SpawnTransform);
    LastSpawnPoint = Floor.ImpactPoint;
    return true;
}
