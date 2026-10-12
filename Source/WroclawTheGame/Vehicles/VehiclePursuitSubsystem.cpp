#include "Vehicles/VehiclePursuitSubsystem.h"
#include "Vehicles/CityTrafficVehicle.h"
#include "Vehicles/DriveableVehicle.h"
#include "Vehicles/VehicleAIDriverComponent.h"
#include "World/CityPopulation.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"

int32 UVehiclePursuitSubsystem::ActivePursuerCount() const
{
    int32 Count = 0;
    for (const auto &Pursuer : Pursuers)
        if (Pursuer && !Pursuer->IsActorBeingDestroyed())
            ++Count;
    return Count;
}

bool UVehiclePursuitSubsystem::StartPursuit(int32 PursuerCount)
{
    auto *PlayerVehicle = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!PlayerVehicle || PursuerCount <= 0)
        return false;

    ACityPopulation *Population = nullptr;
    for (TActorIterator<ACityPopulation> It(GetWorld()); It; ++It)
    {
        Population = *It;
        break;
    }
    if (!Population)
        return false;

    const FCityPopulationRoute *BestRoute = nullptr;
    int32 NearestIndex = INDEX_NONE;
    double BestDistanceSq = TNumericLimits<double>::Max();
    for (const auto &Route : Population->Routes)
    {
        if (!Route.Vehicle || Route.Points.Num() < 4)
            continue;
        const int32 CycleLength = Route.Points.Num() - 1;
        for (int32 I = 0; I < CycleLength; ++I)
        {
            const double DistanceSq = FVector::DistSquared2D(
                Route.Points[I], PlayerVehicle->GetActorLocation());
            if (DistanceSq < BestDistanceSq)
            {
                BestDistanceSq = DistanceSq;
                BestRoute = &Route;
                NearestIndex = I;
            }
        }
    }
    if (!BestRoute || NearestIndex == INDEX_NONE || BestDistanceSq > FMath::Square(8000.0))
        return false;

    StopPursuit();
    PursuerCount = FMath::Clamp(PursuerCount, 1, 3);
    const int32 CycleLength = BestRoute->Points.Num() - 1;
    for (int32 I = 0; I < PursuerCount; ++I)
    {
        int32 StartIndex=INDEX_NONE;
        auto* PC=UGameplayStatics::GetPlayerController(this,0);
        FVector View; FRotator Rotation;
        if (!PC) continue;
        PC->GetPlayerViewPoint(View,Rotation);
        const float HalfFov=PC->PlayerCameraManager?PC->PlayerCameraManager->GetFOVAngle()*.5f:50.f;
        for (int32 Offset=5+I*7;Offset<CycleLength+5+I*7;++Offset)
        {
            const int32 Candidate=((NearestIndex-Offset)%CycleLength+CycleLength)%CycleLength;
            const FVector Point=BestRoute->Points[Candidate]+FVector(0,0,85);
            if (FVector::DistSquared2D(Point,PlayerVehicle->GetActorLocation())<FMath::Square(3500.0)) continue;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(PoliceSpawnVisibility),false,PlayerVehicle);
            FHitResult Occlusion;
            const bool InView=FVector::DotProduct((Point-View).GetSafeNormal(),Rotation.Vector()) > FMath::Cos(FMath::DegreesToRadians(HalfFov+20));
            const bool Occluded=GetWorld()->LineTraceSingleByChannel(Occlusion,View,Point,ECC_Visibility,Params);
            if (InView && !Occluded) continue;
            FHitResult Ground;
            if (!GetWorld()->LineTraceSingleByChannel(Ground,Point+FVector(0,0,100),Point-FVector(0,0,250),ECC_Visibility,Params)) continue;
            if (GetWorld()->OverlapBlockingTestByChannel(Point,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeBox(FVector(210,90,28)),Params)) continue;
            StartIndex=Candidate;break;
        }
        if (StartIndex==INDEX_NONE) continue;

        auto *Pursuer = GetWorld()->SpawnActor<ACityTrafficVehicle>();
        if (!Pursuer)
            continue;
        Pursuer->bPersistentPlayerVehicle = false;
        Pursuer->bSiren = true;
        Pursuer->SetVisualMeshes(Population->TrafficBodyMesh,Population->TrafficWheelMesh);
        for (int32 Lamp=0;Lamp<2;++Lamp)
        {
            auto* Light=NewObject<UPointLightComponent>(Pursuer);
            Light->SetupAttachment(Pursuer->GetRootComponent());
            Light->SetRelativeLocation(FVector(0,Lamp?40:-40,110));
            Light->SetLightColor(Lamp?FLinearColor::Blue:FLinearColor::Red);
            Light->SetIntensity(1800);Light->SetAttenuationRadius(500);
            Light->RegisterComponent();
        }
        Pursuer->TrafficDriver->CruiseSpeed = 1650.0f + I * 120.0f;
        if (!Pursuer->ConfigureTraffic(BestRoute->Points, StartIndex))
        {
            Pursuer->Destroy();
            continue;
        }
        Pursuers.Add(Pursuer);
    }

    if (Pursuers.IsEmpty())
        return false;

    State = EVehiclePursuitState::Locate;
    LastKnown = PlayerVehicle->GetActorLocation();
    LastSeenAt = GetWorld()->GetTimeSeconds();
    Accumulator = 0.0f;
    return true;
}

void UVehiclePursuitSubsystem::StopPursuit()
{
    for (auto &Pursuer : Pursuers)
        if (Pursuer)
        {
            Pursuer->DeactivateTraffic();
            Pursuer->Destroy();
        }
    Pursuers.Reset();
    State = EVehiclePursuitState::Inactive;
    LastKnown = FVector::ZeroVector;
    LastSeenAt = -1000.0;
    Accumulator = 0.0f;
}

bool UVehiclePursuitSubsystem::HasVisualContact(
    ACityTrafficVehicle *Pursuer, AActor *PlayerVehicle) const
{
    if (!Pursuer || !PlayerVehicle ||
        FVector::DistSquared(Pursuer->GetActorLocation(), PlayerVehicle->GetActorLocation()) >
            FMath::Square(static_cast<double>(MaxVisualDistance)))
        return false;

    FCollisionQueryParams Params(SCENE_QUERY_STAT(VehiclePursuitSight), false, Pursuer);
    Params.AddIgnoredActor(Pursuer);
    FHitResult Hit;
    const FVector Start = Pursuer->GetActorLocation() + FVector(0,0,80);
    const FVector End = PlayerVehicle->GetActorLocation() + FVector(0,0,80);
    if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
        return true;
    return Hit.GetActor() == PlayerVehicle;
}

void UVehiclePursuitSubsystem::EnterLost()
{
    for (auto &Pursuer : Pursuers)
        if (Pursuer)
        {
            Pursuer->DeactivateTraffic();
            Pursuer->Destroy();
        }
    Pursuers.Reset();
    State = EVehiclePursuitState::Lost;
}

void UVehiclePursuitSubsystem::Tick(float DeltaTime)
{
    if (State == EVehiclePursuitState::Inactive || State == EVehiclePursuitState::Lost ||
        UGameplayStatics::IsGamePaused(this))
        return;

    Accumulator += DeltaTime;
    if (Accumulator < 0.25f)
        return;
    Accumulator = 0.0f;

    auto *PlayerVehicle = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!PlayerVehicle)
    {
        const double SinceSeen = GetWorld()->GetTimeSeconds() - LastSeenAt;
        State = SinceSeen > SearchSeconds ? EVehiclePursuitState::Lost : EVehiclePursuitState::Search;
        if (State == EVehiclePursuitState::Lost)
            EnterLost();
        return;
    }

    bool bContact = false;
    for (int32 I = Pursuers.Num() - 1; I >= 0; --I)
    {
        if (!Pursuers[I] || Pursuers[I]->IsActorBeingDestroyed() || Pursuers[I]->Health <= 0)
        {
            Pursuers.RemoveAtSwap(I);
            continue;
        }
        bContact |= HasVisualContact(Pursuers[I], PlayerVehicle);
    }
    if (Pursuers.IsEmpty())
    {
        EnterLost();
        return;
    }

    const double Now = GetWorld()->GetTimeSeconds();
    if (bContact)
    {
        LastKnown = PlayerVehicle->GetActorLocation();
        LastSeenAt = Now;
        State = EVehiclePursuitState::Chase;
    }
    for (const auto& Pursuer:Pursuers)
        if (Pursuer)
        {
            Pursuer->TrafficDriver->SetPursuitGoal(LastKnown);
            TArray<UPointLightComponent*> Beacons;Pursuer->GetComponents(Beacons);
            for (int32 I=0;I<Beacons.Num();++I) Beacons[I]->SetVisibility((int32(Now*4)+I)%2==0);
        }
    if (bContact) return;

    const double SinceSeen = Now - LastSeenAt;
    if (SinceSeen > SearchSeconds)
    {
        EnterLost();
        return;
    }
    State = SinceSeen > LoseSightSeconds
                ? EVehiclePursuitState::Search
                : EVehiclePursuitState::Locate;
}
