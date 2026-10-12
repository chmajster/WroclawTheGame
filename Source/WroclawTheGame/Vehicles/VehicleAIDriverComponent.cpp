#include "Vehicles/VehicleAIDriverComponent.h"
#include "Vehicles/DriveableVehicle.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Geography/DistrictRegistry.h"
#include "EngineUtils.h"

UVehicleAIDriverComponent::UVehicleAIDriverComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.05f;
}

void UVehicleAIDriverComponent::BeginPlay()
{
    Super::BeginPlay();
    Vehicle = Cast<ADriveableVehicle>(GetOwner());
    if (!Vehicle)
        SetComponentTickEnabled(false);
}

void UVehicleAIDriverComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    StopAI();
    Super::EndPlay(Reason);
}

bool UVehicleAIDriverComponent::ConfigureRoute(const TArray<FVector> &Points, int32 StartIndex, bool bLoop)
{
    if (!Vehicle || Points.Num() < 2)
        return false;
    Route = Points;
    bLoopRoute = bLoop;
    TargetIndex = FMath::Clamp(StartIndex, 0, Route.Num() - 1);
    if (FVector::Dist2D(Vehicle->GetActorLocation(), Route[TargetIndex]) < WaypointRadius)
        TargetIndex = (TargetIndex + 1) % Route.Num();
    bActive = true;
    SetComponentTickEnabled(true);
    Vehicle->SetAIControl(true, 0.0f, 0.0f, true);
    return true;
}

void UVehicleAIDriverComponent::StopAI()
{
    bActive = false;
    bPursuit = false;
    Route.Reset();
    TargetIndex = 0;
    if (Vehicle)
        Vehicle->SetAIControl(false);
    SetComponentTickEnabled(false);
}

void UVehicleAIDriverComponent::TickComponent(float Dt, ELevelTick TickType,
                                              FActorComponentTickFunction *ThisTickFunction)
{
    Super::TickComponent(Dt, TickType, ThisTickFunction);
    if (!bActive || !Vehicle || Vehicle->Health <= 0 || Route.Num() < 2)
    {
        StopAI();
        return;
    }

    FVector ToTarget = Route[TargetIndex] - Vehicle->GetActorLocation();
    if (ToTarget.Size2D() <= WaypointRadius)
    {
        if (TargetIndex + 1 < Route.Num())
            ++TargetIndex;
        else if (bLoopRoute)
            TargetIndex = 0;
        else
        {
            StopAI();
            return;
        }
        ToTarget = Route[TargetIndex] - Vehicle->GetActorLocation();
    }

    if (bPursuit && FVector::Dist2D(Vehicle->GetActorLocation(),PursuitGoal)<2000)
    {
        FHitResult Wall;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(PursuitSteering),false,Vehicle);
        // Keep to the source street circuit until the last short, clear approach.
        if (!GetWorld()->LineTraceSingleByChannel(Wall,Vehicle->GetActorLocation()+FVector(0,0,60),PursuitGoal+FVector(0,0,60),ECC_WorldStatic,Params))
            ToTarget=PursuitGoal-Vehicle->GetActorLocation();
    }
    const FVector SteeringDirection=ToTarget.GetSafeNormal2D();
    const float ForwardDot = FVector::DotProduct(Vehicle->GetActorForwardVector(), SteeringDirection);
    const float RightDot = FVector::DotProduct(Vehicle->GetActorRightVector(), SteeringDirection);
    const float Angle = FMath::Atan2(RightDot, ForwardDot);
    const float Steering = FMath::Clamp(Angle / FMath::DegreesToRadians(48.0f), -1.0f, 1.0f);

    const float TurnPenalty = FMath::Clamp(FMath::Abs(Angle) / PI, 0.0f, 0.75f);
    const float DesiredSpeed = CruiseSpeed * (1.0f - TurnPenalty);
    const float ForwardSpeed = Vehicle->Speed;

    bool bBrake = ForwardDot < -0.1f || FMath::Abs(ForwardSpeed) > DesiredSpeed * 1.15f;
    float Throttle = bBrake ? 0.0f : FMath::Clamp((DesiredSpeed - ForwardSpeed) / FMath::Max(CruiseSpeed, 1.0f), 0.18f, 1.0f);
    if (bPursuit && FVector::Dist2D(Vehicle->GetActorLocation(),PursuitGoal)<450) {bBrake=true;Throttle=0;}
    // Deterministic right of way: lowest stable actor ID owns a shared junction
    // while within its crossing radius. A car already inside always clears it.
    for (TActorIterator<AWTGDistrictRegistry> Registry(GetWorld());Registry;++Registry)
        for (const FVector& Junction:Registry->Intersections)
        {
            const float Distance=FVector::Dist2D(Vehicle->GetActorLocation(),Junction);
            if (Distance<500 || Distance>1600 || FVector::DotProduct((Junction-Vehicle->GetActorLocation()).GetSafeNormal2D(),Vehicle->GetActorForwardVector())<.3) continue;
            for (TActorIterator<ADriveableVehicle> Other(GetWorld());Other;++Other)
            {
                if (*Other==Vehicle || Other->IsHidden() || !Other->IsAIControlled()) continue;
                const float TheirDistance=FVector::Dist2D(Other->GetActorLocation(),Junction);
                if (TheirDistance<500 || (TheirDistance<1500 && Other->GetUniqueID()<Vehicle->GetUniqueID())) {bBrake=true;Throttle=0;break;}
            }
        }

    FCollisionQueryParams Params(SCENE_QUERY_STAT(VehicleAIObstacle), false, Vehicle);
    FHitResult Hit;
    const FVector Start = Vehicle->GetActorLocation() + FVector(0, 0, 45);
    const FVector End = Start + Vehicle->GetActorForwardVector() * ObstacleLookAhead;
    if (GetWorld()->SweepSingleByChannel(
            Hit, Start, End, Vehicle->GetActorQuat(), ECC_Pawn,
            FCollisionShape::MakeBox(FVector(170, 70, 55)), Params))
    {
        const float Distance = FVector::Dist2D(Start, Hit.ImpactPoint);
        if (Distance < 350.0f)
        {
            bBrake = true;
            Throttle = 0.0f;
        }
        else
            Throttle *= FMath::Clamp((Distance - 350.0f) / FMath::Max(ObstacleLookAhead - 350.0f, 1.0f), 0.15f, 1.0f);
    }

    Vehicle->SetAIControl(true, Throttle, Steering, bBrake);
}
