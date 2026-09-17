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
    Body = CreateDefaultSubobject<UBoxComponent>(TEXT("Body")); SetRootComponent(Body);
    Body->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual")); Visual->SetupAttachment(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Visual->SetStaticMesh(Cube.Object); Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void ACityAmbientAgent::Configure(const FCityPopulationRoute &Definition, int32 StartIndex)
{
    Route = Definition.Points; bVehicle = Definition.Vehicle;
    const FVector Size = bVehicle ? FVector(180,80,50) : FVector(25,25,85);
    Body->SetBoxExtent(Size); Visual->SetRelativeScale3D(Size/50);
    if (Route.Num() < 2) return;
    StartIndex = FMath::Clamp(StartIndex,0,Route.Num()-2);
    Target = StartIndex+1;
    SetActorLocation(Route[StartIndex]+FVector(0,0,Size.Z+12));
    SetActorHiddenInGame(false); SetActorEnableCollision(true); SetActorTickEnabled(true);
}
void ACityAmbientAgent::Tick(float Dt)
{
    Super::Tick(Dt);
    if (Route.Num()<2 || UGameplayStatics::IsGamePaused(this)) return;
    if (Target>=Route.Num()) Target=1;
    const float Height=bVehicle?62:97, Speed=bVehicle?700:125;
    FVector Delta=Route[Target]+FVector(0,0,Height)-GetActorLocation();
    if (Delta.Size2D()<45) { ++Target; return; }
    const FVector Step=Delta.GetSafeNormal2D()*FMath::Min(static_cast<double>(Speed*Dt),Delta.Size2D());
    FVector Next=GetActorLocation()+Step;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CityAmbientFloor),false,this);
    FHitResult Floor;
    // No movement onto missing streamed collision. Stop for a blocking actor rather than ghosting through it.
    if (!GetWorld()->LineTraceSingleByChannel(Floor,Next+FVector(0,0,200),Next-FVector(0,0,400),ECC_Visibility,Params)) return;
    if (Floor.ImpactNormal.Z<.65 || FMath::Abs(Floor.ImpactPoint.Z+Height-Next.Z)>120) return;
    Next.Z=Floor.ImpactPoint.Z+Height;
    FHitResult Obstacle;
    const FVector Extent=Body->GetScaledBoxExtent()*.9;
    if (GetWorld()->SweepSingleByChannel(Obstacle,GetActorLocation(),Next+Delta.GetSafeNormal2D()*(bVehicle?250:45),
                                         GetActorQuat(),ECC_Pawn,FCollisionShape::MakeBox(Extent),Params)) return;
    SetActorRotation(Delta.Rotation());
    SetActorLocation(Next,true);
}
ACityPopulation::ACityPopulation()
{
    PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.TickInterval=1;
}
void ACityPopulation::Tick(float Dt)
{
    Super::Tick(Dt);
    auto *Player=UGameplayStatics::GetPlayerPawn(this,0);
    if (!Player || UGameplayStatics::IsGamePaused(this)) return;
    // Fixed pool: at most 24 ambient agents; no per-NPC persistence or far-away physics simulation.
    while (Pool.Num()<24)
    {
        auto *Agent=GetWorld()->SpawnActor<ACityAmbientAgent>();
        if (!Agent) break;
        Agent->SetActorHiddenInGame(true); Agent->SetActorEnableCollision(false); Agent->SetActorTickEnabled(false);
        Pool.Add(Agent); AssignedRoutes.Add(INDEX_NONE);
    }
    TArray<int32> Near;
    for (int32 R=0;R<Routes.Num();++R)
        for (const auto &Point:Routes[R].Points)
            if (FVector::DistSquared(Point,Player->GetActorLocation())<FMath::Square(22000.)) { Near.Add(R);break; }
    for (int32 I=0;I<Pool.Num();++I)
    {
        if (!Pool[I]) continue;
        const int32 RouteIndex=Near.IsEmpty()?INDEX_NONE:Near[(I/3)%Near.Num()];
        if (I>=Near.Num()*3 || RouteIndex==INDEX_NONE)
        {
            Pool[I]->SetActorHiddenInGame(true);Pool[I]->SetActorEnableCollision(false);Pool[I]->SetActorTickEnabled(false);
            AssignedRoutes[I]=INDEX_NONE;continue;
        }
        if (AssignedRoutes[I]!=RouteIndex)
        {
            const auto &Route=Routes[RouteIndex];
            if (Route.Points.Num()<2) continue;
            const int32 Start=(I%3)*(Route.Points.Num()-1)/3;
            if (FVector::DistSquared(Route.Points[Start],Player->GetActorLocation())<FMath::Square(600.)) continue;
            Pool[I]->Configure(Route,Start);AssignedRoutes[I]=RouteIndex;
        }
    }
}
