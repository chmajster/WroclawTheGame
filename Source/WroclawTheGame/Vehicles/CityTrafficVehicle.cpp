#include "Vehicles/CityTrafficVehicle.h"
#include "Vehicles/VehicleAIDriverComponent.h"
#include "Components/BoxComponent.h"

ACityTrafficVehicle::ACityTrafficVehicle()
{
    bPersistentPlayerVehicle = false;
    TrafficDriver = CreateDefaultSubobject<UVehicleAIDriverComponent>(TEXT("TrafficDriver"));
}

bool ACityTrafficVehicle::ConfigureTraffic(const TArray<FVector> &Route, int32 StartIndex)
{
    if (Route.Num() < 2 || !TrafficDriver)
        return false;

    StartIndex = FMath::Clamp(StartIndex, 0, Route.Num() - 2);
    const FVector Start = Route[StartIndex] + FVector(0, 0, 85);
    const FVector Next = Route[StartIndex + 1] + FVector(0, 0, 85);
    SetActorLocation(Start, false, nullptr, ETeleportType::TeleportPhysics);
    SetActorRotation((Next - Start).Rotation());
    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);
    SetActorTickEnabled(true);
    Health = Definition ? Definition->MaxHealth : 100.0f;
    return TrafficDriver->ConfigureRoute(Route, StartIndex + 1, true);
}

void ACityTrafficVehicle::DeactivateTraffic()
{
    if (TrafficDriver)
        TrafficDriver->StopAI();
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);
    Chassis->SetSimulatePhysics(false);
}
