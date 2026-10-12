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
    EntryPoint=Vehicle->GetActorLocation();
    bLoopRoute = bLoop;
    TargetIndex = FMath::Clamp(StartIndex, 0, Route.Num() - 1);
    if (FVector::Dist2D(Vehicle->GetActorLocation(), Route[TargetIndex]) < WaypointRadius)
        TargetIndex = (TargetIndex + 1) % Route.Num();
    bActive = true;
    ObstacleWait=YieldReverse=0;
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

    const FVector Position=Vehicle->GetActorLocation();
    FVector ToTarget = Route[TargetIndex] - Position;
    bool PassedCorner=false;
    if(TargetIndex+1<Route.Num())
    {
        FVector A=TargetIndex>0?Route[TargetIndex-1]:EntryPoint,B=Route[TargetIndex],C=Route[TargetIndex+1],P=Position;
        A.Z=B.Z=C.Z=P.Z=0;
        const FVector Next=C-B;const float Along=FVector::DotProduct(P-B,Next)/FMath::Max(1.f,Next.SizeSquared());
        PassedCorner=Along>.02f && FVector::DistSquared(P,FMath::ClosestPointOnSegment(P,B,C))+2500<FVector::DistSquared(P,FMath::ClosestPointOnSegment(P,A,B));
    }
    if (ToTarget.Size2D() <= WaypointRadius || PassedCorner)
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

    // Follow a point ahead along the polyline. Waiting until the intersection
    // centre to turn leaves a physical car facing the opposite pavement.
    const FVector Previous=TargetIndex>0?Route[TargetIndex-1]:EntryPoint;
    FVector FlatPosition=Position,FlatPrevious=Previous,FlatTarget=Route[TargetIndex];
    FlatPosition.Z=FlatPrevious.Z=FlatTarget.Z=0;
    const FVector Nearest=FMath::ClosestPointOnSegment(FlatPosition,FlatPrevious,FlatTarget);
    float Ahead=FMath::Clamp(250.f+FMath::Abs(Vehicle->Speed)*.65f,250.f,650.f);
    const float Remaining=FVector::Dist2D(Nearest,FlatTarget);
    FVector Aim=Nearest+(FlatTarget-Nearest).GetSafeNormal2D()*FMath::Min(Ahead,Remaining);
    Ahead-=Remaining;int32 LookIndex=TargetIndex;
    for(int32 Step=0;Ahead>0 && Step<Route.Num()-1;++Step)
    {
        const int32 NextIndex=LookIndex+1<Route.Num()?LookIndex+1:(bLoopRoute?0:-1);
        if(NextIndex<0)break;
        const FVector Segment=Route[NextIndex]-Aim;const float Length=Segment.Size2D();
        if(Length>Ahead){Aim+=Segment*(Ahead/Length);break;}
        Ahead-=Length;Aim=Route[NextIndex];LookIndex=NextIndex;
    }
    ToTarget=Aim-Position;

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
    const bool Reverse=ForwardDot<-.35f || (Vehicle->Speed< -50 && ForwardDot<.35f);
    const float Steering = FMath::Clamp((Reverse?-FMath::Atan2(-RightDot,-ForwardDot):Angle) / FMath::DegreesToRadians(48.0f), -1.0f, 1.0f);

    const float TurnPenalty = FMath::Clamp(FMath::Abs(Angle) / PI, 0.0f, 0.75f);
    const float DesiredSpeed = CruiseSpeed * (1.0f - TurnPenalty);
    const float ForwardSpeed = Vehicle->Speed;

    bool bBrake = FMath::Abs(ForwardSpeed) > DesiredSpeed * 1.15f;
    FString BrakeReason;
    float Throttle = bBrake ? 0.0f : FMath::Clamp((DesiredSpeed - ForwardSpeed) / FMath::Max(CruiseSpeed, 1.0f), 0.18f, 1.0f);
    if (Reverse) { bBrake=ForwardSpeed< -400;Throttle=bBrake?0.f:-.4f; }
    if (bPursuit && FVector::Dist2D(Vehicle->GetActorLocation(),PursuitGoal)<450) {bBrake=true;Throttle=0;}
    // Deterministic right of way: lowest stable actor ID owns a shared junction
    // while within its crossing radius. A car already inside always clears it.
    for (TActorIterator<AWTGDistrictRegistry> Registry(GetWorld());Registry;++Registry)
        for (const FVector& Junction:Registry->Intersections)
        {
            const float Distance=FVector::Dist2D(Vehicle->GetActorLocation(),Junction);
            const FVector ToJunction=Junction-Vehicle->GetActorLocation();
            const float Along=FVector::DotProduct(ToJunction,SteeringDirection);
            if (Distance<500 || Along<0 || Along>1000 || (ToJunction-SteeringDirection*Along).Size2D()>300) continue;
            for (TActorIterator<ADriveableVehicle> Other(GetWorld());Other;++Other)
            {
                if (*Other==Vehicle || Other->IsHidden() || !Other->IsAIControlled()) continue;
                const float TheirDistance=FVector::Dist2D(Other->GetActorLocation(),Junction);
                if (TheirDistance<500 || (TheirDistance<1500 && Other->GetUniqueID()<Vehicle->GetUniqueID())) {bBrake=true;Throttle=0;BrakeReason=TEXT("junction:")+Other->GetName();break;}
            }
        }

    FCollisionQueryParams Params(SCENE_QUERY_STAT(VehicleAIObstacle), false, Vehicle);
    FHitResult Hit;
    const FVector Start = Vehicle->GetActorLocation() + FVector(0, 0, 45);
    const FVector End = Start + SteeringDirection * ObstacleLookAhead;
    ADriveableVehicle* BlockingCar=nullptr;
    if (GetWorld()->SweepSingleByChannel(
            Hit, Start, End, SteeringDirection.Rotation().Quaternion(), ECC_Pawn,
            FCollisionShape::MakeBox(FVector(170, 70, 55)), Params))
    {
        const float Distance = FVector::Dist2D(Start, Hit.ImpactPoint);
        BrakeReason=TEXT("obstacle:")+(Hit.GetActor()?Hit.GetActor()->GetActorNameOrLabel():TEXT("none"))+FString::Printf(TEXT(" %.1f"),Distance);
        if (Distance < 350.0f)
        {
            bBrake = true;
            Throttle = 0.0f;
            BlockingCar=Cast<ADriveableVehicle>(Hit.GetActor());
        }
        else
            Throttle *= FMath::Clamp((Distance - 350.0f) / FMath::Max(ObstacleLookAhead - 350.0f, 1.0f), 0.15f, 1.0f);
    }
    // Resolve mutual stops without deleting visible traffic or disabling collisions.
    // The later actor yields by backing into a checked clear space, then retries.
    if(BlockingCar && FMath::Abs(ForwardSpeed)<30)ObstacleWait+=Dt;else ObstacleWait=0;
    if(ObstacleWait>2 && Vehicle->GetUniqueID()>BlockingCar->GetUniqueID()){
        YieldReverse=1.5f;ObstacleWait=0;
    }
    if(YieldReverse>0){
        YieldReverse-=Dt;FHitResult Rear;
        const FVector BackEnd=Start-Vehicle->GetActorForwardVector()*450;
        const bool Blocked=GetWorld()->SweepSingleByChannel(Rear,Start,BackEnd,Vehicle->GetActorQuat(),ECC_Pawn,FCollisionShape::MakeBox(FVector(170,70,45)),Params);
        Throttle=Blocked?0.f:-.3f;bBrake=Blocked || ForwardSpeed< -220;
        Vehicle->SetAIControl(true,Throttle,0,bBrake);return;
    }

#if !UE_BUILD_SHIPPING
    Diagnostic=FString::Printf(TEXT("node=%d dist=%.1f dot=%.2f throttle=%.2f steer=%.2f brake=%d reason=%s"),TargetIndex,ToTarget.Size2D(),ForwardDot,Throttle,Steering,bBrake,*BrakeReason);
#endif
    Vehicle->SetAIControl(true, Throttle, Steering, bBrake);
}
