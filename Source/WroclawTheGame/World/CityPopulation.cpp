#include "World/CityPopulation.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ACityAmbientAgent::ACityAmbientAgent()
{
    PrimaryActorTick.bCanEverTick = true;
    Body = CreateDefaultSubobject<UBoxComponent>(TEXT("Body"));
    SetRootComponent(Body);
    Body->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
    Visual->SetupAttachment(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Visual->SetStaticMesh(Cube.Object);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetSimulationLevel(ECityAgentSimulationLevel::Dormant);
}

void ACityAmbientAgent::SetSimulationLevel(ECityAgentSimulationLevel Level)
{
    SimulationLevel = Level;
    switch (Level)
    {
    case ECityAgentSimulationLevel::Full:
        SetActorHiddenInGame(false);
        SetActorEnableCollision(true);
        SetActorTickEnabled(true);
        SetActorTickInterval(0.0f);
        break;
    case ECityAgentSimulationLevel::Simplified:
        SetActorHiddenInGame(false);
        SetActorEnableCollision(false);
        SetActorTickEnabled(true);
        SetActorTickInterval(0.10f);
        break;
    case ECityAgentSimulationLevel::Dormant:
    default:
        SetActorHiddenInGame(true);
        SetActorEnableCollision(false);
        SetActorTickEnabled(false);
        CurrentSpeed = 0.0f;
        break;
    }
}

void ACityAmbientAgent::Configure(const FCityPopulationRoute &Definition, int32 StartIndex)
{
    Route = Definition.Points;
    bVehicle = Definition.Vehicle;
    CurrentSpeed = 0.0f;
    const FVector Size = bVehicle ? FVector(180,80,50) : FVector(25,25,85);
    Body->SetBoxExtent(Size);
    Visual->SetRelativeScale3D(Size / 50);
    if (Route.Num() < 2)
    {
        SetSimulationLevel(ECityAgentSimulationLevel::Dormant);
        return;
    }
    StartIndex = FMath::Clamp(StartIndex, 0, Route.Num() - 2);
    Target = StartIndex + 1;
    SetActorLocation(Route[StartIndex] + FVector(0,0,Size.Z + 12));
}

void ACityAmbientAgent::Tick(float Dt)
{
    Super::Tick(Dt);
    if (SimulationLevel == ECityAgentSimulationLevel::Dormant ||
        Route.Num() < 2 || UGameplayStatics::IsGamePaused(this))
        return;

    if (Target >= Route.Num())
        Target = 1;

    const float Height = bVehicle ? 62.0f : 97.0f;
    const float CruiseSpeed = bVehicle ? 700.0f : 125.0f;
    FVector Delta = Route[Target] + FVector(0,0,Height) - GetActorLocation();
    if (Delta.Size2D() < 45.0)
    {
        ++Target;
        return;
    }

    FVector NextDirection = Delta.GetSafeNormal2D();
    float DesiredSpeed = CruiseSpeed;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CityAmbientFloor), false, this);

    if (SimulationLevel == ECityAgentSimulationLevel::Full)
    {
        FHitResult Obstacle;
        const FVector Extent = Body->GetScaledBoxExtent() * 0.9;
        const float LookAhead = bVehicle ? 350.0f : 65.0f;
        if (GetWorld()->SweepSingleByChannel(
                Obstacle, GetActorLocation(),
                GetActorLocation() + NextDirection * LookAhead,
                GetActorQuat(), ECC_Pawn, FCollisionShape::MakeBox(Extent), Params))
        {
            const float ObstacleDistance = FVector::Dist2D(GetActorLocation(), Obstacle.ImpactPoint);
            const float StopDistance = bVehicle ? 220.0f : 45.0f;
            DesiredSpeed = ObstacleDistance <= StopDistance
                               ? 0.0f
                               : CruiseSpeed * FMath::Clamp((ObstacleDistance - StopDistance) / LookAhead, 0.15f, 1.0f);
        }
    }

    const float SpeedResponse = bVehicle ? 2.5f : 6.0f;
    CurrentSpeed = FMath::FInterpTo(CurrentSpeed, DesiredSpeed, Dt, SpeedResponse);
    if (CurrentSpeed < 1.0f)
        return;

    const FVector Step = NextDirection * FMath::Min(static_cast<double>(CurrentSpeed * Dt), Delta.Size2D());
    FVector Next = GetActorLocation() + Step;

    FHitResult Floor;
    if (!GetWorld()->LineTraceSingleByChannel(
            Floor, Next + FVector(0,0,200), Next - FVector(0,0,400),
            ECC_Visibility, Params))
        return;
    if (Floor.ImpactNormal.Z < 0.65 || FMath::Abs(Floor.ImpactPoint.Z + Height - Next.Z) > 120.0)
        return;

    Next.Z = Floor.ImpactPoint.Z + Height;
    const float TurnResponse = bVehicle ? 3.0f : 8.0f;
    SetActorRotation(FMath::RInterpTo(GetActorRotation(), Delta.Rotation(), Dt, TurnResponse));
    SetActorLocation(Next, SimulationLevel == ECityAgentSimulationLevel::Full);
}

ACityPopulation::ACityPopulation()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 1.0f;
}

void ACityPopulation::Tick(float Dt)
{
    Super::Tick(Dt);
    auto *Player = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!Player || UGameplayStatics::IsGamePaused(this))
        return;

    MaxAgents = FMath::Clamp(MaxAgents, 1, 128);
    SimplifiedSimulationRadius = FMath::Max(SimplifiedSimulationRadius, 1000.0f);
    FullSimulationRadius = FMath::Clamp(FullSimulationRadius, 1000.0f, SimplifiedSimulationRadius);

    while (Pool.Num() < MaxAgents)
    {
        auto *Agent = GetWorld()->SpawnActor<ACityAmbientAgent>();
        if (!Agent)
            break;
        Pool.Add(Agent);
        AssignedRoutes.Add(INDEX_NONE);
    }

    TArray<int32> Near;
    const double SimplifiedRadiusSq = FMath::Square(static_cast<double>(SimplifiedSimulationRadius));
    for (int32 R = 0; R < Routes.Num(); ++R)
        for (const auto &Point : Routes[R].Points)
            if (FVector::DistSquared(Point, Player->GetActorLocation()) < SimplifiedRadiusSq)
            {
                Near.Add(R);
                break;
            }

    const double FullRadiusSq = FMath::Square(static_cast<double>(FullSimulationRadius));
    for (int32 I = 0; I < Pool.Num(); ++I)
    {
        if (!Pool[I])
            continue;

        if (I >= MaxAgents)
        {
            Pool[I]->SetSimulationLevel(ECityAgentSimulationLevel::Dormant);
            AssignedRoutes[I] = INDEX_NONE;
            continue;
        }

        const int32 RouteIndex = Near.IsEmpty() ? INDEX_NONE : Near[(I / 3) % Near.Num()];
        if (I >= Near.Num() * 3 || RouteIndex == INDEX_NONE)
        {
            Pool[I]->SetSimulationLevel(ECityAgentSimulationLevel::Dormant);
            AssignedRoutes[I] = INDEX_NONE;
            continue;
        }

        if (AssignedRoutes[I] != RouteIndex)
        {
            const auto &Route = Routes[RouteIndex];
            if (Route.Points.Num() < 2)
            {
                Pool[I]->SetSimulationLevel(ECityAgentSimulationLevel::Dormant);
                AssignedRoutes[I] = INDEX_NONE;
                continue;
            }
            const int32 Start = (I % 3) * (Route.Points.Num() - 1) / 3;
            if (FVector::DistSquared(Route.Points[Start], Player->GetActorLocation()) < FMath::Square(600.0))
                continue;
            Pool[I]->Configure(Route, Start);
            AssignedRoutes[I] = RouteIndex;
        }

        const double DistanceSq = FVector::DistSquared(Pool[I]->GetActorLocation(), Player->GetActorLocation());
        Pool[I]->SetSimulationLevel(
            DistanceSq <= FullRadiusSq
                ? ECityAgentSimulationLevel::Full
                : ECityAgentSimulationLevel::Simplified);
    }
}
