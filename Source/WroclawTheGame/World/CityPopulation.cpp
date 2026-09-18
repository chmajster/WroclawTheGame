#include "World/CityPopulation.h"
#include "Vehicles/CityTrafficVehicle.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ACityAmbientAgent::ACityAmbientAgent()
{
    PrimaryActorTick.bCanEverTick = true;
    Body = CreateDefaultSubobject<UBoxComponent>(TEXT("Body"));
    SetRootComponent(Body);
    Body->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
    Visual->SetupAttachment(Body);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual->SetCanEverAffectNavigation(false);
    PedestrianVisual = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PedestrianVisual"));
    PedestrianVisual->SetupAttachment(Body);
    PedestrianVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PedestrianVisual->SetCanEverAffectNavigation(false);
    PedestrianVisual->SetVisibility(false);
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

void ACityAmbientAgent::Configure(const FCityPopulationRoute &Definition, int32 StartIndex, UStaticMesh *VehicleMesh, USkeletalMesh *PedestrianMesh)
{
    Route = Definition.Points;
    bVehicle = Definition.Vehicle;
    CurrentSpeed = 0.0f;
    const FVector Size = bVehicle ? FVector(180,80,50) : FVector(25,25,85);
    Body->SetBoxExtent(Size);
    if (bVehicle)
    {
        Visual->SetStaticMesh(VehicleMesh);
        Visual->SetVisibility(VehicleMesh != nullptr);
        Visual->SetRelativeScale3D(FVector(1));
        PedestrianVisual->SetVisibility(false);
    }
    else
    {
        PedestrianVisual->SetSkeletalMesh(PedestrianMesh);
        PedestrianVisual->SetVisibility(PedestrianMesh != nullptr);
        PedestrianVisual->SetRelativeScale3D(FVector(1));
        PedestrianVisual->SetRelativeLocation(FVector(0,0,-97));
        PedestrianVisual->SetRelativeRotation(FRotator(0,-90,0));
        Visual->SetVisibility(false);
    }
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

    const FVector NextDirection = Delta.GetSafeNormal2D();
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
    MaxTrafficVehicles = FMath::Clamp(MaxTrafficVehicles, 0, 24);
    SimplifiedSimulationRadius = FMath::Max(SimplifiedSimulationRadius, 1000.0f);
    FullSimulationRadius = FMath::Clamp(FullSimulationRadius, 1000.0f, SimplifiedSimulationRadius);
    TrafficActivationRadius = FMath::Max(TrafficActivationRadius, 1000.0f);

    while (Pool.Num() < MaxAgents)
    {
        auto *Agent = GetWorld()->SpawnActor<ACityAmbientAgent>();
        if (!Agent)
            break;
        Pool.Add(Agent);
        AssignedRoutes.Add(INDEX_NONE);
    }
    while (TrafficPool.Num() < MaxTrafficVehicles)
    {
        auto *Traffic = GetWorld()->SpawnActor<ACityTrafficVehicle>();
        if (!Traffic)
            break;
        Traffic->SetVisualMeshes(TrafficBodyMesh, TrafficWheelMesh);
        Traffic->DeactivateTraffic();
        TrafficPool.Add(Traffic);
        AssignedTrafficRoutes.Add(INDEX_NONE);
    }

    TArray<int32> NearPedestrianRoutes;
    TArray<int32> NearTrafficRoutes;
    const FVector PlayerPosition = Player->GetActorLocation();
    const double SimplifiedRadiusSq = FMath::Square(static_cast<double>(SimplifiedSimulationRadius));
    const double TrafficRadiusSq = FMath::Square(static_cast<double>(TrafficActivationRadius));
    for (int32 R = 0; R < Routes.Num(); ++R)
    {
        const double RadiusSq = Routes[R].Vehicle ? TrafficRadiusSq : SimplifiedRadiusSq;
        for (const FVector &Point : Routes[R].Points)
            if (FVector::DistSquared(Point, PlayerPosition) < RadiusSq)
            {
                (Routes[R].Vehicle ? NearTrafficRoutes : NearPedestrianRoutes).Add(R);
                break;
            }
    }

    const double FullRadiusSq = FMath::Square(static_cast<double>(FullSimulationRadius));
    for (int32 I = 0; I < Pool.Num(); ++I)
    {
        if (!Pool[I])
            continue;

        const int32 RouteIndex = NearPedestrianRoutes.IsEmpty()
                                     ? INDEX_NONE
                                     : NearPedestrianRoutes[(I / 3) % NearPedestrianRoutes.Num()];
        if (I >= MaxAgents || I >= NearPedestrianRoutes.Num() * 3 || RouteIndex == INDEX_NONE)
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
            if (FVector::DistSquared(Route.Points[Start], PlayerPosition) < FMath::Square(600.0))
                continue;
            Pool[I]->Configure(Route, Start, VehicleMesh, PedestrianMesh);
            AssignedRoutes[I] = RouteIndex;
        }

        const double DistanceSq = FVector::DistSquared(Pool[I]->GetActorLocation(), PlayerPosition);
        Pool[I]->SetSimulationLevel(
            DistanceSq <= FullRadiusSq
                ? ECityAgentSimulationLevel::Full
                : ECityAgentSimulationLevel::Simplified);
    }

    for (int32 I = 0; I < TrafficPool.Num(); ++I)
    {
        auto *Traffic = TrafficPool[I].Get();
        if (!Traffic)
            continue;

        const int32 RouteIndex = NearTrafficRoutes.IsEmpty()
                                     ? INDEX_NONE
                                     : NearTrafficRoutes[(I / 2) % NearTrafficRoutes.Num()];
        if (I >= MaxTrafficVehicles || I >= NearTrafficRoutes.Num() * 2 || RouteIndex == INDEX_NONE)
        {
            Traffic->DeactivateTraffic();
            AssignedTrafficRoutes[I] = INDEX_NONE;
            continue;
        }

        if (AssignedTrafficRoutes[I] != RouteIndex)
        {
            const auto &Route = Routes[RouteIndex];
            if (Route.Points.Num() < 2)
            {
                Traffic->DeactivateTraffic();
                AssignedTrafficRoutes[I] = INDEX_NONE;
                continue;
            }
            const int32 Start = (I % 2) * (Route.Points.Num() - 1) / 2;
            if (FVector::DistSquared(Route.Points[Start], PlayerPosition) < FMath::Square(1200.0))
                continue;
            if (!Traffic->ConfigureTraffic(Route.Points, Start))
            {
                Traffic->DeactivateTraffic();
                AssignedTrafficRoutes[I] = INDEX_NONE;
                continue;
            }
            AssignedTrafficRoutes[I] = RouteIndex;
        }

        if (FVector::DistSquared(Traffic->GetActorLocation(), PlayerPosition) >
            FMath::Square(static_cast<double>(TrafficActivationRadius * 1.35f)))
        {
            Traffic->DeactivateTraffic();
            AssignedTrafficRoutes[I] = INDEX_NONE;
        }
    }
}
