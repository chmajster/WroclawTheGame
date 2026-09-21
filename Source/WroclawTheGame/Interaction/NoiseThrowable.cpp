#include "Interaction/NoiseThrowable.h"
#include "Systems/NoiseSystem.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/StaticMesh.h"
#include "Perception/AISense_Hearing.h"
#include "Audio/SliceAudio.h"
ANoiseThrowable::ANoiseThrowable()
{
    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("PhysicsCollision"));
    RootComponent = Collision;
    Collision->SetSphereRadius(7.0f);
    Collision->SetCollisionProfileName(TEXT("PhysicsActor"));
    Collision->SetSimulatePhysics(true);
    Collision->SetNotifyRigidBodyCollision(true);
    Collision->SetCanEverAffectNavigation(false);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ThrownObject"));
    Mesh->SetupAttachment(Collision);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetCanEverAffectNavigation(false);
}
void ANoiseThrowable::BeginPlay()
{
    Super::BeginPlay();
    UStaticMesh *CanMesh = LoadObject<UStaticMesh>(
        nullptr, TEXT("/Game/FreeModels/PolyHaven/can_rusted/can_rusted_1k.can_rusted_1k"));
    if (CanMesh)
        Mesh->SetStaticMesh(CanMesh);
    else
        UE_LOG(LogTemp, Error, TEXT("NoiseThrowable: imported CC0 can mesh is unavailable"));
    Collision->OnComponentHit.AddDynamic(this, &ANoiseThrowable::Hit);
    SetLifeSpan(12);
}
void ANoiseThrowable::Hit(UPrimitiveComponent *Component, AActor *Other, UPrimitiveComponent *OtherComponent,
                          FVector Impulse, const FHitResult &Result)
{
    if (bReported || Other == GetInstigator())
        return;
    bReported = true;
    GetWorld()->GetSubsystem<UNoiseSystem>()->Report(TEXT("Throw"), GetActorLocation(), GetInstigator());
    USliceAudio::Play(this, TEXT("Hit"), GetActorLocation());
}
